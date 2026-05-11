#ifndef CACHE_H
#define CACHE_H

#include <bits/stdc++.h>
using namespace std;

// ─────────────────────────────────────────────────────────
//  Direct-Mapped Cache
//
//  Address breakdown:
//    offset = address % BLOCK_SIZE
//    index  = (address / BLOCK_SIZE) % NUM_LINES
//    tag    = address / (BLOCK_SIZE * NUM_LINES)
//
//  Policy: Write-through + No-allocate on write miss
// ─────────────────────────────────────────────────────────

const int BLOCK_SIZE = 4;    // 4 words per cache block
const int NUM_LINES  = 16;   // 16 cache lines
// Total cache = 16 × 4 = 64 words

struct CacheLine {
    bool valid = false;
    int  tag   = -1;
    int  data[BLOCK_SIZE] = {0};
};

struct Cache {
    CacheLine lines[NUM_LINES];
    int hits   = 0;
    int misses = 0;
    int totalAccesses = 0;

    // Break address into tag, index, offset
    void getFields(int address, int &tag, int &index, int &offset) const;

    // Read a word — on miss, loads entire block from main memory
    int read(int address, int* mainMemory);

    // Write a word — write-through (always writes to main memory)
    // On cache hit, updates cache too. On miss, doesn't allocate.
    void write(int address, int data, int* mainMemory);

    void printStats() const;
    void printContents() const;
};

#endif
