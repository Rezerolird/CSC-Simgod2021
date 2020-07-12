#ifndef _NCF_H_
#define _NCF_H_

#include <set>
#include <iostream>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <random>
#include <assert.h>
#include <xmmintrin.h>
#include <immintrin.h>

#include "MurmurHash3.h"
#include "HashTable.h"
#include "bitarray.h"

using namespace std;

bool Is_pow_of_2(uint64_t x){
    return !(x & (x - 1));
}

uint64_t Next_power_of_2(uint64_t x){
	if (Is_pow_of_2(x))
		return x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16; 
	return x + 1;    
}

template<typename fp_t, int fp_len>
class NCF{

    public:
        int a;

        bool Insert(int set_ID, string key);
        bool Delete(int set_ID, string key);
        set<size_t> Query(string key);

        uint64_t IndexHash(uint32_t hv);
        uint64_t AltIndex(uint64_t index, fp_t fp);
        fp_t FpHash(uint32_t hv);
        void GenerateIndexFpHash(string key, uint64_t* index, fp_t* fp);

        typedef struct{
            uint64_t index;
            fp_t fp;
            bool used;
        } VictimCache;

        HashTable<fp_t, fp_len>** table_;
        VictimCache* victim_;
        uint64_t* seeds;
        uint64_t num_items_;
        uint64_t num_sets_;
        uint64_t num_buckets_;
        uint64_t modulo_table_;
        int max_kick_num_;
        double time;
        NCF(int m, int max_kick_num, uint64_t bucket_num);
        
};

template<typename fp_t, int fp_len>
NCF<fp_t, fp_len>::NCF(int m, int max_kick_num, uint64_t bucket_num){
    
    this->num_items_ = 0;
    this->seeds = new uint64_t[2];
    random_device cf;
    mt19937 seed_ge(cf());
    uniform_int_distribution<uint64_t> dis(0, 1000000);
    for(int i = 0; i < 2; i++){
        this->seeds[i] = dis(seed_ge);
    }

    this->num_sets_ = m;
    this->max_kick_num_ = max_kick_num;
    this->num_buckets_ = Next_power_of_2(bucket_num);
    
    this->table_ = new HashTable<fp_t, fp_len>*[this->num_sets_];
    for(int i=0; i<this->num_sets_; i++){
        this->table_[i] = new HashTable<fp_t, fp_len>(this->num_buckets_);
    }

    this->modulo_table_ = this->num_buckets_ - 1;
    this->time = 0.0;
    this->victim_ = new VictimCache[this->num_sets_];
}

template<typename fp_t, int fp_len>
uint64_t NCF<fp_t, fp_len>::IndexHash(uint32_t hv){
    return hv & (this->num_buckets_ - 1);
}

template<typename fp_t, int fp_len>
fp_t NCF<fp_t, fp_len>::FpHash(uint32_t hv){
    uint32_t tag = hv & ((1ULL << fp_len) - 1);
    tag += (tag == 0);
    return tag;
}

template<typename fp_t, int fp_len>
uint64_t NCF<fp_t, fp_len>::AltIndex(uint64_t index, fp_t fp){
    return IndexHash((uint64_t)(index ^ (fp * 0x5bd1e995))); 
}

template<typename fp_t, int fp_len>
void NCF<fp_t, fp_len>::GenerateIndexFpHash(string key, uint64_t* index, fp_t* fp){

    uint64_t hash_index[2];
    MurmurHash3_x64_128(key.c_str(), key.size(), this->seeds[0], (void *)hash_index);

    *index = IndexHash(hash_index[0] >> 32);
    *fp = FpHash(hash_index[0]);

}

template<typename fp_t, int fp_len>
bool NCF<fp_t, fp_len>::Insert(int set_ID, string key){

    uint64_t index;
    fp_t fp;
    GenerateIndexFpHash(key, &index, &fp);

    uint64_t cur_index = index;
    fp_t cur_fp = fp;
    fp_t old_fp;

    for(int i = 0; i < this->max_kick_num_; i++){
        bool kickout = i > 0;
        old_fp = 0;
        if(this->table_[set_ID]->InsertFpInBucket(cur_index, cur_fp, kickout, &old_fp)){
            this->num_items_++;
            return 1;
        }
        if(kickout){
            cur_fp = old_fp;
        }
        cur_index = AltIndex(cur_index, cur_fp);
    }
    this->victim_[set_ID].index = cur_index;
    this->victim_[set_ID].fp = cur_fp;
    this->victim_[set_ID].used = true;
    return 1;
}

template<typename fp_t, int fp_len>
set<size_t> NCF<fp_t, fp_len>::Query(string key){

    set<size_t> res; res.clear();
    bool found = false;
    uint64_t i1, i2;
    fp_t fp;

    chrono::time_point<chrono::high_resolution_clock> t5 = chrono::high_resolution_clock::now();
    GenerateIndexFpHash(key, &i1, &fp);
    i2 = AltIndex(i1, fp);

    for(int i=0; i<this->num_sets_; i++){
        found = this->victim_[i].used && (fp == this->victim_[i].fp) &&
            (i1 == this->victim_[i].index || i2 == this->victim_[i].index);

        if(found || this->table_[i]->FindFpInBuckets2(i1, i2, fp)){
            res.insert(i);
        }
    }
    chrono::time_point<chrono::high_resolution_clock> t6 = chrono::high_resolution_clock::now();
    this->time += ((t6-t5).count()/1000000000.0);

    return res;
}

template<typename fp_t, int fp_len>
bool NCF<fp_t, fp_len>::Delete(int set_ID, string key){
    uint64_t i1, i2;
    fp_t fp;

    GenerateIndexFpHash(key, &i1, &fp);
    i2 = AltIndex(i1, fp);
    if(this->table_[set_ID]->DeleteFp(i1, fp)){
        this->num_items_--;
        goto TryEliminateVictim;
    }
    else if(this->table_[set_ID]->DeleteFp(i2, fp)){
        this->num_items_--;
        goto TryEliminateVictim;
    }
    else if(this->victim_[set_ID].used && fp = this->victim_[set_ID].fp && (i1 == this->victim_[set_ID].index || i2 == this->victim_[set_ID].index)){
        this->victim_.used = false;
    }
    else{
        return false;
    }

TryEliminateVictim:
    if (this->victim_[set_ID].used) {
        this->victim_[set_ID].used = false;
        uint64_t index = this->victim_[set_ID].index;
        fp_t tag = this->victim_[set_ID].fp;
        Insert(index, tag);
    }

    return true;
}



#endif