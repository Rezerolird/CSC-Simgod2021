#include <iostream>
#include <cstring>
#include <vector>
#include <random>

#include "MurmurHash3.h"
#include "BloomFilter.h"

using namespace std;

vector<size_t> BF_Hash(string key, int k, size_t* seed, size_t single_capacity){

    vector<size_t> hash_value;
    size_t op;

    for(int i=0; i<k; i++){
        MurmurHash3_x86_128(key.c_str(), key.size(), seed[i], &op);
        hash_value.push_back(op%single_capacity);
    }
    return hash_value;
}

BloomFilter::BloomFilter(size_t size, size_t k){
    this->k = k;
    this->bits_ = new bitarray(size);
    this->capacity = size;
    this->time_ins = 0;
}

void BloomFilter::insert(vector<size_t> a){
    int N = a.size();
    for(int n=0; n<N; n++){
        this->bits_->setbit(a[n]);
        this->time_ins ++;
    }
}

bool BloomFilter::check(vector<size_t> a){
    int N = a.size();
    for (int n =0 ; n<N; n++){
        if (!this->bits_->checkbit(a[n])){
            return false;
        }
    }
    return true;
}
