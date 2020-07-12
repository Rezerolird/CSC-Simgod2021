#ifndef _RAMBO_
#define _RAMBO_
#include <vector>
#include <set>
#include <cstring>
#include <random>
#include "BloomFilter.h"

class RAMBO{
    public:

        std::vector<size_t> Hash(std::string key);

        RAMBO(size_t R, size_t B, size_t total_usage, int k, int m);
        std::set<size_t> takeIntrsec(std::set<size_t>* setArray);
        void createPartition();
        void insertion(std::string setID, std::string keys);
        std::set<size_t> query(std::string query_key);

        int k;
        int r;
        int b;
        int num_sets;
        size_t capacity;
        size_t single_capacity;
        float time;
        size_t* seed;
        size_t* partition_seed;

        BloomFilter** Rambo_array;
        std::set<size_t>* Rambo_partition;

};

#endif