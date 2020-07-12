#ifndef _BITARRAY_
#define _BITARRAY_
#include <cstdint>

class bitarray{
    public:

        bitarray(size_t size);
        void setbit(size_t k);
        void clearbit(size_t k);
        bool checkbit(size_t k);
        void andop(size_t* B);

        size_t getcount();
        size_t array_size;
        size_t bf_size;
        size_t* bit_array;
};

#endif