#ifndef _CUCKOOFILTER_H_
#define _CUCKOOFILTER_H_

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

bool is_pow_of_2(uint64_t x){
    return !(x & (x - 1));
}

uint64_t next_power_of_2(uint64_t x){
	if (is_pow_of_2(x))
		return x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16; 
	return x + 1;    
}

template<typename fp_t, int fp_len>
class CuckooFilter{

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

        HashTable<fp_t, fp_len>* table_;
        VictimCache victim_;
        uint64_t* seeds;
        uint64_t num_items_;
        uint64_t num_sets_;
        uint64_t num_buckets_;
        uint64_t modulo_table_;
        int max_kick_num_;
        double time;
        CuckooFilter(int m, int max_kick_num, uint64_t bucket_num);
        
};

template<typename fp_t, int fp_len>
CuckooFilter<fp_t, fp_len>::CuckooFilter(int m, int max_kick_num, uint64_t bucket_num){
    
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
    this->num_buckets_ = next_power_of_2(bucket_num);
    this->table_ = new HashTable<fp_t, fp_len>(this->num_buckets_);

    this->modulo_table_ = this->num_buckets_ - 1;
    this->time = 0.0;
}

template<typename fp_t, int fp_len>
uint64_t CuckooFilter<fp_t, fp_len>::IndexHash(uint32_t hv){
    return hv & (this->table_->NumBuckets() - 1);
}

template<typename fp_t, int fp_len>
fp_t CuckooFilter<fp_t, fp_len>::FpHash(uint32_t hv){
    uint32_t tag = hv & ((1ULL << fp_len) - 1);
    tag += (tag == 0);
    return tag;
}

template<typename fp_t, int fp_len>
uint64_t CuckooFilter<fp_t, fp_len>::AltIndex(uint64_t index, fp_t fp){
    return IndexHash((uint64_t)(index ^ (fp * 0x5bd1e995))); 
}

template<typename fp_t, int fp_len>
void CuckooFilter<fp_t, fp_len>::GenerateIndexFpHash(string key, uint64_t* index, fp_t* fp){

    uint64_t hash_index[2];
    MurmurHash3_x64_128(key.c_str(), key.size(), this->seeds[0], (void *)hash_index);

    *index = IndexHash(hash_index[0] >> 32);
    *fp = FpHash(hash_index[0]);

}

template<typename fp_t, int fp_len>
bool CuckooFilter<fp_t, fp_len>::Insert(int set_ID, string key){

    uint64_t index;
    fp_t fp;
    GenerateIndexFpHash(key, &index, &fp);
    
    uint64_t cur_index = (index + set_ID) & (this->table_->NumBuckets() - 1);
    fp_t cur_fp = fp;
    fp_t old_fp;

    for(int i = 0; i < this->max_kick_num_; i++){
        bool kickout = i > 0;
        old_fp = 0;
        if(this->table_->InsertFpInBucket(cur_index, cur_fp, kickout, &old_fp)){
            this->num_items_++;
            return 1;
        }
        if(kickout){
            cur_fp = old_fp;
        }
        cur_index = AltIndex(cur_index, cur_fp);
    }
    this->victim_.index = cur_index;
    this->victim_.fp = cur_fp;
    this->victim_.used = true;
    return 1;
}

template<typename fp_t, int fp_len>
set<size_t> CuckooFilter<fp_t, fp_len>::Query(string key){

    set<size_t> res; res.clear();
    bitarray k1(this->num_sets_);
    bool found = false;
    uint64_t i1, i2, i1_back;
    fp_t fp;

    chrono::time_point<chrono::high_resolution_clock> t5 = chrono::high_resolution_clock::now();
    GenerateIndexFpHash(key, &i1, &fp);
    found = victim_.used && (fp == victim_.fp) &&
        (i1 == victim_.index || i2 == victim_.index);
    i1_back = i1;
    i2 = AltIndex(i1, fp);
    for(int i=0; i<this->num_sets_; i++){
        if(found || this->table_->FindFpInBuckets1(i1, fp)){
            k1.setbit(i);
        }
        i1 = (i1 + 1) & this->modulo_table_;

    }

    uint64_t mask = 1ULL; int count = 0;
    for(int i=0; i<this->num_sets_; i++){
        if(k1.bit_array[count] & mask){
            res.insert(i);
        }
        else{
            i2 = AltIndex(i1_back & this->modulo_table_, fp);
            if(found || this->table_->FindFpInBuckets1(i2, fp)){
                res.insert(i);
            }
        }
        i1_back++;
        mask = (mask >> 63) | (mask << 1);
        if(mask & 1) {count++;}
    }

    chrono::time_point<chrono::high_resolution_clock> t6 = chrono::high_resolution_clock::now();
    this->time += ((t6-t5).count()/1000000000.0);

    return res;
}

template<typename fp_t, int fp_len>
bool CuckooFilter<fp_t, fp_len>::Delete(int set_ID, string key){
    uint64_t i1, i2;
    fp_t fp;

    GenerateIndexFpHash(key, &i1, &fp);
    i1 += set_ID;
    i2 = AltIndex(i1, fp);
    if(this->table_->DeleteFp(i1, fp)){
        this->num_items_--;
        goto TryEliminateVictim;
    }
    else if(this->table_->DeleteFp(i2, fp)){
        this->num_items_--;
        goto TryEliminateVictim;
    }
    else if(this->victim_.used && fp = this->victim_.fp && (i1 == this->victim_.index || i2 == this->victim_.index)){
        this->victim_.used = false;
    }
    else{
        return false;
    }

TryEliminateVictim:
    if (this->victim_.used) {
        this->victim_.used = false;
        uint64_t index = this->victim_.index;
        fp_t tag = this->victim_.fp;
        Insert(index, tag);
    }

    return true;
}


#endif