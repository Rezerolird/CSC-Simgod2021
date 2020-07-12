#include <iostream>
#include <cstring>
#include "bitarray.h"

using namespace std;

bitarray::bitarray(size_t size){
    this->array_size = size;
    this->bf_size = (this->array_size + 63) >> 6;
    this->bit_array = new size_t[this->bf_size];

    size_t allocate_size = this->bf_size * 8;
    for(int i = 0; i < this->bf_size; i++){
        this->bit_array[i] = 0;
    }
}

void bitarray::setbit(size_t k){
    this->bit_array[(k>>6)] |= (1ULL << (k&63));
}

void bitarray::clearbit(size_t k){
    this->bit_array[(k>>6)] &= ~(1ULL << (k&63));
}

bool bitarray::checkbit(size_t k){
    return (this->bit_array[(k>>6)] & (1ULL << (k&63)));
}

size_t bitarray::getcount(){
    size_t countx = 0;
    size_t x;
    for (size_t i = 0; i < this->bf_size; i++){
        x = this->bit_array[i];
        while(x){
            x = x & (x - 1);
            countx ++;
        }
    }
    return countx;
}

void bitarray::andop(size_t* B){
    for(size_t i = 0; i < this->bf_size; i++){
        this->bit_array[i] &= B[i];
    }
}
