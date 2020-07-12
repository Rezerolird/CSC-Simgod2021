#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <set>

#include "bitarray.h"
#include "BloomFilter.h"
#include "MurmurHash3.h"
#include "Rambo.h"

using namespace std;

vector<size_t> RAMBO::Hash(string key){

    vector<size_t> Hash;
    size_t op;
    for(int i=0; i<this->r; i++){
        MurmurHash3_x86_32(key.c_str(), key.size(), this->partition_seed[i], &op);
        Hash.push_back(op%this->b);
    }
    return Hash;
}

RAMBO::RAMBO(size_t R, size_t B, size_t total_usage, int k, int m){

    this->r = R;
    this->b = B;
    this->capacity = total_usage;
    this->single_capacity = size_t(this->capacity / (this->r * this->b));
    this->k = k;
    this->num_sets = m;
    this->time = 0.0;

    Rambo_array = new BloomFilter*[this->r * this->b];
    for(int b = 0; b < this->b; b++){
        for(int r = 0; r < this->r; r++){
            Rambo_array[b + this->b * r] = new BloomFilter(this->single_capacity, this->k);
        }
    }

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
    Rambo_partition = new set<size_t>[this->r * this->b];
    RAMBO::createPartition();

}

void RAMBO::createPartition(){
    for(int i=0; i<this->num_sets; i++){
        vector<size_t> partition = RAMBO::Hash(to_string(i));
        for(int r=0; r<this->r; r++){
            Rambo_partition[partition[r] + this->b * r].insert(i);
        }
    }
}

void RAMBO::insertion(string setID, string keys){
    vector<size_t> hash_res = RAMBO::Hash(setID);
    vector<size_t> locations = BF_Hash(keys, this->k, this->seed, this->single_capacity);
    for(int r=0; r < this->r; r++){
        Rambo_array[hash_res[r] + this->b * r]->insert(locations);
    }
}

set<size_t> RAMBO::takeIntrsec(set<size_t>* setArray){
    set<size_t> s1 = setArray[0];
    for (int i=1; i<this->r; i++){
        set<size_t> res;
        set_intersection(s1.begin(), s1.end(), setArray[i].begin(), setArray[i].end(), inserter(res,res.begin()));
        s1 = res;
    }
    return s1;
}

set<size_t> RAMBO::query(string query_key){

    set<size_t> res; res.clear();
    int count = 0; size_t mask = 1ULL;

    bitarray k1(this->num_sets); 
    chrono::time_point<chrono::high_resolution_clock> t0 = chrono::high_resolution_clock::now();
    vector<size_t> check_locations = BF_Hash(query_key, this->k, this->seed, this->single_capacity);
    for(int r=0; r<this->r; r++){
        bitarray k2(this->num_sets);
        for(int b=0; b<this->b; b++){
            if(Rambo_array[b + this->b * r]->check(check_locations)){
                for(auto set_id : Rambo_partition[b+this->b*r])
                    k2.setbit(set_id);
            }
        }
        if(r == 0){
            k1 = k2;
        }
        else{
            k1.andop(k2.bit_array);
        }
    }
    for(int i=0; i<k1.array_size; i++){
        if(k1.bit_array[count] & mask){
            res.insert(i);
        }
        mask = (mask >> 63) | (mask << 1);
        if(mask & 1) {count++;}
    }    
    chrono::time_point<chrono::high_resolution_clock> t1 = chrono::high_resolution_clock::now();
    this->time += ((t1-t0).count()/1000000000.0);

    return res;
}