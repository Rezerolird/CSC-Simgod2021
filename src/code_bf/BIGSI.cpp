#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <set>
#include <chrono>

#include "BloomFilter.h"
#include "MurmurHash3.h"
#include "BIGSI.h"

using namespace std;

BIGSI::BIGSI(int m, int k,  size_t total_usage){

    this->num_sets = m;
    this->capacity = total_usage;
    this->single_capacity = size_t(this->capacity / this->num_sets);
    this->k = k;
    this->time = 0.0;
    this->total_hash_num = 0;

    Bigsi_array = new BloomFilter*[this->num_sets];
    for(int i = 0; i < this->num_sets; i++){
        Bigsi_array[i] = new BloomFilter(this->single_capacity, this->k);
    }

    this->seed = new size_t[this->k];

    random_device bf;
    mt19937 seed_ge(bf());
    uniform_int_distribution<size_t> dis(0, 10000000);
    for(int i = 0; i < this->k; i++){
        this->seed[i] = dis(seed_ge);
    }
}

void BIGSI::insertion(int setID, string keys){
    vector<size_t> locations = BF_Hash(keys, this->k, this->seed, this->single_capacity);
    // Bigsi_array[stoi(setID)]->insert(locations);
    Bigsi_array[setID]->insert(locations);
    this->total_hash_num += locations.size();
}

set<size_t> BIGSI::query(string query_key){

    set<size_t> res_set;
    chrono::time_point<chrono::high_resolution_clock> t0 = chrono::high_resolution_clock::now();
    vector<size_t> check_locations = BF_Hash(query_key, this->k, this->seed, this->single_capacity);
    for(int i=0; i<this->num_sets; i++){
        if(Bigsi_array[i]->check(check_locations)){
            res_set.insert(i);
        }
    }
    chrono::time_point<chrono::high_resolution_clock> t1 = chrono::high_resolution_clock::now();
    this->time += ((t1-t0).count()/1000000000.0);

    return res_set;
}