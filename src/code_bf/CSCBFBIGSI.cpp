#include <iostream>
#include <cstring>
#include <algorithm>
#include <immintrin.h>
#include <cstdlib>
#include <chrono>

#include "bitarray.h"
#include "CSCBFBIGSI.h"
#include "MurmurHash3.h"

using namespace std;

vector<size_t> CSCBFBIGSI::locationHash(string key){
    vector<size_t> Locations;
    size_t op;
    for(int i=0; i<this->k; i++){
        MurmurHash3_x86_128(key.c_str(), key.size(), this->seed[i], &op);
        Locations.push_back(op % this->capacity);
    }
    return Locations;
}

CSCBFBIGSI::CSCBFBIGSI(size_t total_usage, int k, int m){
    this->capacity = total_usage;
    this->k = k;
    this->mask = 1ULL;
    this->set_num = m;

    this->seed = new size_t[this->k];
    random_device bf;
    mt19937 seed_ge(bf());
    uniform_int_distribution<size_t> dis(0, 10000000);
    for(int i = 0; i < this->k; i++){
        this->seed[i] = dis(seed_ge);
    }
    this->CSCBF_array = new bitarray(this->capacity);

    this->array_length = (this->set_num + 63) >> 6;
    this->copy_array = new size_t[this->array_length];
    this->mask_array = new size_t[this->array_length];
    this->result = new size_t[this->array_length];

    this->al_size = this->array_length * 8;
    memset(this->copy_array, 0, this->al_size);
    memset(this->mask_array, 255, this->al_size);
    memset(this->result, 0, this->al_size);
    this->time = 0.0;
}

void CSCBFBIGSI::insertion(string setID, string keys){

    size_t offset_location = 0;
    vector<size_t> locations = locationHash(keys);
    for(auto &location : locations){
        offset_location = location + atoi(setID.c_str());
        if(offset_location < this->capacity){
            this->CSCBF_array->setbit(offset_location);
        }
        else{
            this->CSCBF_array->setbit(offset_location - this->capacity);
        }
    }
}

set<size_t> CSCBFBIGSI::query(string query_key){

    set<size_t> res; res.clear();
    memset(this->copy_array, 0, this->al_size);
    memset(this->mask_array, 255, this->al_size);
    memset(this->result, 0, this->al_size);

    int count = 0;
    this->mask = 1ULL;
    size_t end_location = 0;
    chrono::time_point<chrono::high_resolution_clock> t0 = chrono::high_resolution_clock::now();
    vector<size_t> check_locations = locationHash(query_key);
    for(auto start_location : check_locations){
        end_location = start_location + this->set_num;
        if(end_location < this->capacity){
            CopyArray(this->CSCBF_array->bit_array, this->copy_array, start_location);
        }
        else{
            CopyArray2(this->CSCBF_array->bit_array, this->copy_array, start_location);
        }
        for(int i = 0; i < this->array_length; i++){
            this->mask_array[i] &= this->copy_array[i];
        }
    }
    for(int b=0; b<this->set_num; b++){
        if(this->mask_array[count] & this->mask){
            res.insert(b);
        }
        this->mask = (this->mask >> 63) | (this->mask << 1);
        if(this->mask & 1ULL){
            count++;
        }
    }
    chrono::time_point<chrono::high_resolution_clock> t1 = chrono::high_resolution_clock::now();
    this->time += ((t1-t0).count()/1000000000.0);

    return res;
}

void CSCBFBIGSI::CopyArray(size_t* src_array, size_t* tgt_array, size_t begin_location){

    size_t start = begin_location / 64;
    size_t start_offset = begin_location & 63;
    size_t end_offset = 64 - start_offset;

    int i = 0;
    while(i < this->array_length){
        tgt_array[i] = (src_array[start+1] << end_offset) | (src_array[start] >> start_offset);
        i++; start++;
    }

}

void CSCBFBIGSI::CopyArray2(size_t* src_array, size_t* tgt_array, size_t begin_location){

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
