#include "cache.h"

int Cache::read(int addr, int* mem) {
    int word = addr % 4;
    int slot = (addr / 4) % 4;
    int t    = addr / 16;

    if (valid[slot] && tag[slot] == t) {
        hits++;
        return data[slot][word];
    }

    misses++;
    valid[slot] = true;
    tag[slot] = t;
    int start = (addr / 4) * 4;
    for (int i = 0; i < 4; i++)
        data[slot][i] = mem[start + i];
    return data[slot][word];
}

void Cache::write(int addr, int val, int* mem) {
    int word = addr % 4;
    int slot = (addr / 4) % 4;
    int t    = addr / 16;

    mem[addr] = val;   // always write to memory

    if (valid[slot] && tag[slot] == t) {
        hits++;
        data[slot][word] = val;
    } else {
        misses++;
    }
}

void Cache::print() {
    cout << "\n=== Cache ===" << endl;
    cout << "Hits: " << hits << "  Misses: " << misses << endl;
    int total = hits + misses;
    if (total > 0)
        cout << "Hit Rate: " << (hits * 100 / total) << "%" << endl;
}
