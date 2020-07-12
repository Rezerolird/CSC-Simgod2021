#ifndef _RAMCO_H_
#define _RAMCO_H_
#include <set>
#include <iostream>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <random>
#include <assert.h>
#include <xmmintrin.h>
#include <immintrin.h>
#include <vector>

#include "MurmurHash3.h"
#include "HashTable.h"
#include "bitarray.h"

using namespace std;

bool IS_pow_of_2(uint64_t x){
    return !(x & (x - 1));
}

uint64_t NExt_power_of_2(uint64_t x){
	if (IS_pow_of_2(x))
		return x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16; 
	return x + 1;    
}

template<typename fp_t, int fp_len>
class RAMCO{

    public:
        int a;

        bool Insert(int set_ID, string key);
        bool Delete(int set_ID, string key);
        set<size_t> Query(string key);

        uint64_t IndexHash(uint32_t hv);
        uint64_t AltIndex(uint64_t index, fp_t fp);
        fp_t FpHash(uint32_t hv);
        void GenerateIndexFpHash(string key, uint64_t* index, fp_t* fp);
        void createPartition();
        vector<uint64_t> partitionHash(string key);

        typedef struct{
            uint64_t index;
            fp_t fp;
            bool used;
        } VictimCache;

        HashTable<fp_t, fp_len>** table_;
        VictimCache* victim_;
        uint64_t* seeds;
        uint64_t* partition_seeds;
        uint64_t num_items_;
        uint64_t num_sets_;
        uint64_t num_buckets_;
        uint64_t modulo_table_;

        uint64_t R, B;
        int max_kick_num_;
        double time;
        set<uint64_t>* RAMCO_partition;
        RAMCO(int m, int max_kick_num, uint64_t bucket_num, size_t R, size_t B);
        
};

template<typename fp_t, int fp_len>
RAMCO<fp_t, fp_len>::RAMCO(int m, int max_kick_num, uint64_t bucket_num, uint64_t R, uint64_t B){

    this->R = R;
    this->B = B;
    this->num_items_ = 0;
    this->seeds = new uint64_t[2];
    this->partition_seeds = new uint64_t[this->R];
    random_device cf;
    mt19937 seed_ge(cf());
    uniform_int_distribution<uint64_t> dis(0, 1000000);
    for(int i = 0; i < 2; i++){
        this->seeds[i] = dis(seed_ge);
    }
    for(int i = 0; i < this->R; i++){
        this->partition_seeds[i] = dis(seed_ge);
    }

    this->num_sets_ = m;
    this->max_kick_num_ = max_kick_num;
    this->num_buckets_ = NExt_power_of_2(bucket_num);
    this->table_ = new HashTable<fp_t, fp_len>*[this->R];
    for(int i=0; i<this->R; i++){
        this->table_[i] = new HashTable<fp_t, fp_len>(this->num_buckets_);
    }
    RAMCO_partition = new set<uint64_t>[this->R * this->B];
    createPartition();

    this->modulo_table_ = this->num_buckets_ - 1;
    this->time = 0.0;
    this->victim_ = new VictimCache[this->R];
}

template<typename fp_t, int fp_len>
vector<uint64_t> RAMCO<fp_t, fp_len>::partitionHash(string key){
    vector<uint64_t> Hash;
    uint64_t op;
    for(int i=0; i<this->R; i++){
        MurmurHash3_x86_32(key.c_str(), key.size(), this->partition_seeds[i], &op);
        Hash.push_back(op % this->B);
    }
    return Hash;
}

template<typename fp_t, int fp_len>
void RAMCO<fp_t, fp_len>::createPartition(){
    for(int i=0; i<this->num_sets_; i++){
        vector<uint64_t> partition = partitionHash(to_string(i));
        for(int r=0; r < this->R; r++){
            RAMCO_partition[partition[r] + this->B * r].insert(i);
        }
    }
}

template<typename fp_t, int fp_len>
uint64_t RAMCO<fp_t, fp_len>::IndexHash(uint32_t hv){
    return hv & (this->num_buckets_ - 1);
}

template<typename fp_t, int fp_len>
fp_t RAMCO<fp_t, fp_len>::FpHash(uint32_t hv){
    uint32_t tag = hv & ((1ULL << fp_len) - 1);
    tag += (tag == 0);
    return tag;
}

template<typename fp_t, int fp_len>
uint64_t RAMCO<fp_t, fp_len>::AltIndex(uint64_t index, fp_t fp){
    return IndexHash((uint64_t)(index ^ (fp * 0x5bd1e995))); 
}

template<typename fp_t, int fp_len>
void RAMCO<fp_t, fp_len>::GenerateIndexFpHash(string key, uint64_t* index, fp_t* fp){

    uint64_t hash_index[2];
    MurmurHash3_x64_128(key.c_str(), key.size(), this->seeds[0], (void *)hash_index);

    *index = IndexHash(hash_index[0] >> 32);
    *fp = FpHash(hash_index[0]);

}

template<typename fp_t, int fp_len>
bool RAMCO<fp_t, fp_len>::Insert(int set_ID, string key){

    uint64_t index;
    fp_t fp;
    GenerateIndexFpHash(key, &index, &fp);
    vector<uint64_t> hash_res = partitionHash(to_string(set_ID));
    for(int r=0; r<this->R; r++){
        uint64_t cur_index = (index + hash_res[r]) & (this->table_[r]->NumBuckets() - 1);

        fp_t cur_fp = fp;
        fp_t old_fp;

        for(int i = 0; i < this->max_kick_num_; i++){
            bool kickout = i > 0;
            old_fp = 0;
            if(this->table_[r]->InsertFpInBucket(cur_index, cur_fp, kickout, &old_fp)){
                this->num_items_++;
                break;
            }
            if(kickout){
                cur_fp = old_fp;
            }
            cur_index = AltIndex(cur_index, cur_fp);
        }
        this->victim_[r].index = cur_index;
        this->victim_[r].fp = cur_fp;
        this->victim_[r].used = true;
    }
    return 1;
}

template<typename fp_t, int fp_len>
set<size_t> RAMCO<fp_t, fp_len>::Query(string key){

    bitarray k0(this->num_sets_);
    set<uint64_t> res; res.clear();
    bool found = false;
    uint64_t i1, i2, i1_back, i1_back2;
    fp_t fp;
    uint64_t mask;
    int count; 

    chrono::time_point<chrono::high_resolution_clock> t0 = chrono::high_resolution_clock::now();
    GenerateIndexFpHash(key, &i1, &fp);
    i1_back2 = i1;
    for(int r=0; r<this->R; r++){
        i1 = i1_back2;

        bitarray k1(this->B);
        bitarray k2(this->num_sets_);

        found = victim_[r].used && (fp == victim_[r].fp) &&
            (i1 == victim_[r].index || i2 == victim_[r].index);
        i1_back = i1;
        i2 = AltIndex(i1, fp);
        for(int b=0; b<this->B; b++){
            if(found || this->table_[r]->FindFpInBuckets1(i1, fp)){
                k1.setbit(b);
            }
            i1 = (i1 + 1) & this->modulo_table_;
        }
        mask = 1ULL; count = 0;
        for(int b=0; b<this->B; b++){
            if(k1.bit_array[count] & mask){
                for(auto set_id : RAMCO_partition[b + this->B *r]){
                    k2.setbit(set_id);
                }
            }
            else{
                i2 = AltIndex(i1_back & this->modulo_table_, fp);
                if(found || this->table_[r]->FindFpInBuckets1(i2, fp)){
                    for(auto set_id : RAMCO_partition[b + this->B *r]){
                        k2.setbit(set_id);
                    }
                }
            }
            i1_back++;
            mask = (mask >> 63) | (mask << 1);
            if(mask & 1) {count++;}
        }
        if(r == 0){
            k0 = k2;
        }
        else{
            k0.andop(k2.bit_array);
        }
    }
    count = 0; mask = 1ULL;
    for(int i=0; i<this->num_sets_; i++){
        if(k0.bit_array[count] & mask){
            res.insert(uint64_t(i));
        }
        mask = (mask >> 63) | (mask << 1);
        if(mask & 1) {count++;}
    }

    chrono::time_point<chrono::high_resolution_clock> t1 = chrono::high_resolution_clock::now();
    this->time += ((t1-t0).count()/1000000000.0);

    return res;
}

template<typename fp_t, int fp_len>
bool RAMCO<fp_t, fp_len>::Delete(int set_ID, string key){
    uint64_t i1, i2;
    fp_t fp;

    GenerateIndexFpHash(key, &i1, &fp);
    vector<uint64_t> hash_res = partitionHash(to_string(set_ID));
    for(int r=0; r<this->R; r++){
        i1 += hash_res[r];
        i2 = AltIndex(i1, fp);
        if(this->table_[r]->DeleteFp(i1, fp)){
            this->num_items_--;
            goto TryEliminateVictim;
        }
        else if(this->table_[r]->DeleteFp(i2, fp)){
            this->num_items_--;
            goto TryEliminateVictim;
        }
        else if(this->victim_[r].used && fp = this->victim_[r].fp && (i1 == this->victim_[r].index || i2 == this->victim_[r].index)){
            this->victim_[r].used = false;
        }
        else{
            return false;
        }

    TryEliminateVictim:
        if (this->victim_[r].used) {
            this->victim_[r].used = false;
            uint64_t index = this->victim_[r].index;
            fp_t tag = this->victim_[r].fp;
            Insert(set_ID, tag);
        }

    }

    return true;
}



#endif
