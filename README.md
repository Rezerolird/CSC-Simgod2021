# CSC-Simgod2021
Building Fast and Compact Sketches for Approximately Multi-Set Multi-Membership Querying

Source code of CSC-BF is in ./src/code_bf, in which CSCBF.cpp refers to CSC-RAMBO in the paper.

Source code of CSC-CF is in ./src/code_cf, in which RAMCO.h refers to CSC-CF and CuckooFilter.h refers to CSC-NCF in the paper.

To run CSC-BF, one can compile it using

`g++ main.cpp BIGSI.cpp MurmurHash3.cpp BloomFilter.cpp Rambo.cpp CSCBF.cpp bitarray.cpp CSCBFBIGSI.cpp -o main -std=c++14`

and then run `./main`

To run CSC-CF, one can compile it using

`g++ main.cpp bitarray.cpp MurmurHash3.cpp -o main -std=c++14`

and then run `./main`
