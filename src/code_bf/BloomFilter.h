#ifndef _BLOOMFILTER_
#define _BLOOMFILTER_
#include <vector>
#include "bitarray.h"

std::vector<size_t> BF_Hash(std::string key, int k, size_t* seed, size_t single_capacity);

class BloomFilter{
    public:
        BloomFilter(size_t size, size_t k);
        void insert(std::vector<size_t> a);
        bool check(std::vector<size_t> a);

        size_t k;
        size_t capacity;
        size_t time_ins;

        bitarray* bits_;
};

#endif