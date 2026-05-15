#ifndef CACHE_H
#define CACHE_H
#include <iostream>
using namespace std;
struct Cache {
    bool valid[4] = {};
    int  tag[4]   = {-1,-1,-1,-1};
    int  data[4][4] = {};
    int  hits = 0, misses = 0;
    int  read(int addr, int* mem);
    void write(int addr, int val, int* mem);
    void print();
};

#endif
