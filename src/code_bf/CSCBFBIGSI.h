#ifndef _CSCBFBIGSI_
#define _CSCBFBIGSI_
#include <vector>
#include <set>
#include <random>
#include "bitarray.h"

class CSCBFBIGSI{
    public:
        CSCBFBIGSI(size_t total_usage, int k, int m);
        void insertion(std::string setID, std::string keys);
        std::set<size_t> query(std::string query_key);
        std::vector<size_t> locationHash(std::string key);
        void CopyArray(size_t* src_array, size_t* tgt_array, size_t begin_location);
        void CopyArray2(size_t* src_array, size_t* tgt_array, size_t begin_location);

        int k;
        size_t capacity;
        int set_num;
        size_t mask;
        size_t* copy_array;
        size_t* mask_array;
        size_t* result;
        int array_length, al_size;
        float time;
        size_t* seed;

        bitarray* CSCBF_array;

};

#endif