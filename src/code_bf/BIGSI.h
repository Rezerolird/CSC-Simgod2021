#ifndef _BIGSI_
#define _BIGSI_
#include <vector>
#include <set>
#include <cstring>
#include <random>

#include "BloomFilter.h"

class BIGSI{
    public:

        BIGSI(int m, int k,  size_t total_usage);
        void insertion(int setID, std::string keys);
        std::set<size_t> query(std::string query_key);

        int num_sets;
        int k;
        size_t capacity;
        size_t single_capacity;
        float time;
        size_t* seed;
        int total_hash_num;

        BloomFilter** Bigsi_array;

};

#endif