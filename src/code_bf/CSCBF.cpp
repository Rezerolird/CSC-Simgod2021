#include <iostream>
#include <cstring>
#include <algorithm>
#include <immintrin.h>
#include <cstdlib>
#include <chrono>
#include <cmath>

#include "bitarray.h"
#include "CSCBF.h"
#include "MurmurHash3.h"

using namespace std;

vector<size_t> CSCBF::partitionHash(string key){
    vector<size_t> Hash;
    size_t op;
    for(int i=0; i<this->r; i++){
        MurmurHash3_x86_32(key.c_str(), key.size(), i, &op);
        Hash.push_back(op % this->b);
    }
    return Hash;
}

vector<size_t> CSCBF::locationHash(string key){ 
    vector<size_t> Locations;
    size_t op;
    for(int i=0; i<this->k; i++){
        MurmurHash3_x86_128(key.c_str(), key.size(), this->seed[i], &op);
        Locations.push_back(op % this->single_capacity);
    }
    return Locations;
}

CSCBF::CSCBF(size_t R, size_t B, size_t total_usage, int k, int m){
    this->r = R;
    this->b = B;
    this->capacity = total_usage;
    this->single_capacity = size_t(this->capacity / (this->r));
    this->k = k;
    this->mask = 1ULL;
    this->set_num = m;

    this->seed = new size_t[this->k];
    this->partition_seed = new size_t[this->r];
    random_device bf;
    mt19937 seed_ge(bf());
    uniform_int_distribution<size_t> dis(0, 1000000);
    for(int i = 0; i < this->k; i++){
        this->seed[i] = dis(seed_ge);
    }

    for(int i = 0; i < this->r; i++){
        this->partition_seed[i] = dis(seed_ge);
    }

    this->CSCBF_array = new bitarray*[this->r];
    for(int r=0; r < this->r; r++){
        this->CSCBF_array[r] = new bitarray(this->single_capacity);
    }
    CSCBF_partition = new set<size_t>[this->r * this->b];
    CSCBF::createPartition();

    this->array_length = (this->b + 63) >> 6;
    this->copy_array = new size_t[this->array_length];
    this->mask_array = new size_t[this->array_length];
    this->result = new size_t[this->array_length];

    this->al_size = this->array_length * 8;
    memset(this->copy_array, 0, this->al_size);
    memset(this->mask_array, 255, this->al_size);
    memset(this->result, 0, this->al_size);
    this->time = 0.0;

}

void CSCBF::createPartition(){
    for(int i=0; i<this->set_num; i++){
        vector<size_t> partition = partitionHash(to_string(i));
        for(int r=0; r < this->r; r++){
            CSCBF_partition[partition[r] + this->b * r].insert(i);
        }
    }
}

void CSCBF::insertion(string setID, string keys){

    size_t offset_location = 0;
    int set_id = atoi(setID.c_str());
    vector<size_t> hash_res = partitionHash(to_string(set_id));
    vector<size_t> locations = locationHash(keys);
    for(int r=0; r < this->r; r++){
        for(auto &location : locations){
            offset_location = location + hash_res[r];
            if(offset_location < this->single_capacity){
                this->CSCBF_array[r]->setbit(offset_location);
            }
            else{
                this->CSCBF_array[r]->setbit(offset_location - this->single_capacity);
            }
        }
    }
}


set<size_t> CSCBF::query(string query_key){

    set<size_t> res; res.clear();
    bitarray k1(this->set_num);
    int count;
    size_t end_location = 0;

    chrono::time_point<chrono::high_resolution_clock> t0 = chrono::high_resolution_clock::now();
    vector<size_t> check_locations = locationHash(query_key);
    for(int r=0; r<this->r; r++){
        bitarray k2(this->set_num);
        memset(this->mask_array, 255, this->al_size);
        for(auto start_location : check_locations){
            end_location = start_location + this->b;
            if(end_location < this->single_capacity){
                CopyArray(this->CSCBF_array[r]->bit_array, this->copy_array, start_location);
            }
            else{
                CopyArray2(this->CSCBF_array[r]->bit_array, this->copy_array, start_location);
            }
            for(int i = 0; i < this->array_length; i++){
                this->mask_array[i] &= this->copy_array[i];
            }
        }

        this->mask = 1ULL; count = 0;
        for(int b=0; b<this->b; b++){
            if(this->mask_array[count] & this->mask){
                for(auto set_id : CSCBF_partition[b + this->b*r]){
                    k2.setbit(set_id);
                }
            }
            this->mask = (this->mask >> 63) | (this->mask << 1);
            if(this->mask & 1){
                count++;
            }
        }
        if(r == 0){
            k1 = k2;
        }
        else{
            k1.andop(k2.bit_array);
        }
    }

    count = 0; this->mask = 1ULL;
    for(int i=0; i<this->set_num; i++){
        if(k1.bit_array[count] & this->mask){
            res.insert(size_t(i));
        }
        this->mask = (this->mask >> 63) | (this->mask << 1);
        if(this->mask & 1) {count++;}
    }

    chrono::time_point<chrono::high_resolution_clock> t1 = chrono::high_resolution_clock::now();
    this->time += ((t1-t0).count()/1000000000.0);

    return res;
}

void CSCBF::CopyArray(size_t* src_array, size_t* tgt_array, size_t begin_location){

    size_t start = begin_location / 64;
    size_t start_offset = begin_location & 63;
    size_t end_offset = 64 - start_offset;

    int i = 0;
    while(i < this->array_length){
        tgt_array[i] = (src_array[start+1] << end_offset) | (src_array[start] >> start_offset);
        i++; start++; 
    }

}

void CSCBF::CopyArray2(size_t* src_array, size_t* tgt_array, size_t begin_location){

    size_t start = begin_location / 64;
    size_t start_offset = begin_location & 63;
    size_t end_offset = 64 - start_offset;
    size_t dis = this->array_length - 1;

    int i = 0;
    while(i < this->array_length){

        if(start < dis){
            tgt_array[i] = (src_array[start+1] << end_offset) | (src_array[start] >> start_offset);
            start ++;   
        }
        else if(start == dis){
            tgt_array[i] = (src_array[0] << end_offset) | (src_array[start] >> start_offset);
            start = 0;
        }
        i++;
    }
}
