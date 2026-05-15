#include "cache.h"
/**
 * Reads a single word from memory, utilizing the direct-mapped cache.
 * If the data is not in the cache (miss), it loads a block of 4 words from main memory.
 *
 * @param addr The absolute memory address to read.
 * @param mem Pointer to the main memory array.
 * @return The integer value stored at the requested memory address.
 */
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
/**
 * Writes a value to main memory and updates the cache if the block is currently cached.
 * Utilizes a write-through, write-no-allocate policy.
 *
 * @param addr The absolute memory address to write to.
 * @param val The integer value to write.
 * @param mem Pointer to the main memory array.
 */
void Cache::write(int addr, int val, int* mem) {
    int word = addr % 4;
    int slot = (addr / 4) % 4;
    int t    = addr / 16;
    mem[addr] = val;
    if (valid[slot] && tag[slot] == t) {
        hits++;
        data[slot][word] = val;
    } else {
        misses++;
    }
}

/**
 * Prints the cache hit and miss statistics to standard output.
 */
void Cache::print() {
    cout << "\n=== Cache ===" << endl;
    cout << "Hits: " << hits << "  Misses: " << misses << endl;
    int total = hits + misses;
    if (total > 0)
        cout << "Hit Rate: " << (hits * 100 / total) << "%" << endl;
}
