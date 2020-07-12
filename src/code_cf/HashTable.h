#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include <iostream>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <random>
#include <xmmintrin.h>
#include <immintrin.h>

using namespace std;

template <typename index_len, int bits_per_tag>
class HashTable{

    static const int kTagsPerBucket = 4;
    static const int kBytesPerBucket = (sizeof(index_len) * kTagsPerBucket + 7) >> 3;
    static const uint32_t kTagMask = (1ULL << bits_per_tag) - 1;

    struct Bucket{
        index_len bits_[kTagsPerBucket];
    } __attribute__((__packed__));

    Bucket *buckets_;
    uint64_t num_buckets_;
    uint32_t *Buckets_;

    public:
        HashTable(uint64_t bucket_num){
            num_buckets_ = bucket_num;
            buckets_ = new Bucket[num_buckets_];
            memset(buckets_, 0, kBytesPerBucket * num_buckets_);
        }

        ~HashTable(){
            delete[] buckets_;
        }

        uint64_t NumBuckets(){
            return num_buckets_;
        }

        uint64_t SizeInBytes(){
            return kBytesPerBucket * num_buckets_;
        }

        uint64_t SizeInFps(){
            return kTagsPerBucket * num_buckets_;
        }

        uint32_t ReadFp(uint32_t index, int j){
            return buckets_[index].bits_[j];
        }

        void WriteFp(uint32_t index, int j, index_len fp){
            buckets_[index].bits_[j] = fp;
        }

        bool FindFpInBuckets1(uint64_t index, index_len fp){

            for(int i = 0; i < kTagsPerBucket; i++){
                if(ReadFp(index, i) == fp){ return true; }
            }
            return false;
        }

        bool FindFpInBuckets2(uint64_t index1, uint64_t index2, index_len fp){

            for(int i = 0; i < kTagsPerBucket; i++){
                if(ReadFp(index1, i) == fp || ReadFp(index2, i) == fp){ return true; }
            }
            return false;
        }

        bool InsertFpInBucket(uint32_t index, index_len fp, bool kickout, index_len *old_fp){
            for(int i=0; i < kTagsPerBucket; i++){
                if(ReadFp(index, i) == 0){
                    WriteFp(index, i, fp);
                    return true;
                }
            }
            if(kickout){
                index_len r = rand() & 3;
                *old_fp = ReadFp(index, r);
                WriteFp(index, r, fp);
            }
            return false;
        }

        uint64_t NumFpsInBucket(uint32_t index){
            uint64_t num = 0;
            for(int i=0; i<kBytesPerBucket; i++){
                if(ReadFp(index, i) != 0) { num++; }
            }
            return num;
        }
};

#endif