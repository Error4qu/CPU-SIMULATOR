#include "cache.h"

// ─────────────────────────────────────────────────────────
//  Split address into tag, index, offset
//
//  Example: address=10, BLOCK_SIZE=4, NUM_LINES=16
//    offset = 10 % 4 = 2
//    index  = (10 / 4) % 16 = 2
//    tag    = 10 / (4 * 16) = 0
// ─────────────────────────────────────────────────────────
void Cache::getFields(int address, int &tag, int &index, int &offset) const {
    offset = address % BLOCK_SIZE;
    index  = (address / BLOCK_SIZE) % NUM_LINES;
    tag    = address / (BLOCK_SIZE * NUM_LINES);
}

// ─────────────────────────────────────────────────────────
//  READ from cache (used by LOAD)
//
//  1. Compute tag, index, offset from address
//  2. Check cache line at [index]:
//     - If valid AND tag matches → HIT, return cached data
//     - Else → MISS, load entire block from main memory
//       into cache, then return the word
// ─────────────────────────────────────────────────────────
int Cache::read(int address, int* mainMemory) {
    int tag, index, offset;
    getFields(address, tag, index, offset);
    totalAccesses++;

    CacheLine &line = lines[index];

    if (line.valid && line.tag == tag) {
        // ── CACHE HIT ──
        hits++;
        return line.data[offset];
    }

    // ── CACHE MISS — load entire block from main memory ──
    misses++;
    line.valid = true;
    line.tag   = tag;
    int blockStart = (address / BLOCK_SIZE) * BLOCK_SIZE; // align to block boundary
    for (int i = 0; i < BLOCK_SIZE; i++) {
        line.data[i] = mainMemory[blockStart + i];
    }
    return line.data[offset];
}

// ─────────────────────────────────────────────────────────
//  WRITE to cache (used by STORE)
//
//  Write-through: ALWAYS write to main memory.
//  If the block is already in cache (hit), update cache too.
//  If not in cache (miss), don't bring it in (no-allocate).
// ─────────────────────────────────────────────────────────
void Cache::write(int address, int data, int* mainMemory) {
    int tag, index, offset;
    getFields(address, tag, index, offset);
    totalAccesses++;

    // Always write to main memory (write-through)
    mainMemory[address] = data;

    CacheLine &line = lines[index];
    if (line.valid && line.tag == tag) {
        // HIT — update cache too
        hits++;
        line.data[offset] = data;
    } else {
        // MISS — don't allocate (write-no-allocate)
        misses++;
    }
}

// ─────────────────────────────────────────────────────────
//  Print cache statistics
// ─────────────────────────────────────────────────────────
void Cache::printStats() const {
    cout << "║  CACHE STATISTICS                                   ║" << endl;
    cout << "╠═══════════════════════════════════════════════════════╣" << endl;
    cout << "║  Config: " << NUM_LINES << " lines × " << BLOCK_SIZE
         << " words = " << NUM_LINES * BLOCK_SIZE << " words total" << endl;
    cout << "║  Policy: Write-through, No-allocate on write miss" << endl;
    cout << "║  Total Accesses : " << setw(6) << totalAccesses << endl;
    cout << "║  Hits           : " << setw(6) << hits << endl;
    cout << "║  Misses         : " << setw(6) << misses << endl;
    if (totalAccesses > 0) {
        double hitRate = (double)hits / totalAccesses * 100.0;
        cout << "║  Hit Rate       : " << fixed << setprecision(1)
             << setw(5) << hitRate << "%" << endl;
    }
}

// ─────────────────────────────────────────────────────────
//  Print cache contents (non-empty lines only)
// ─────────────────────────────────────────────────────────
void Cache::printContents() const {
    cout << "║  CACHE CONTENTS (valid lines)                       ║" << endl;
    cout << "╠═══════════════════════════════════════════════════════╣" << endl;
    for (int i = 0; i < NUM_LINES; i++) {
        if (lines[i].valid) {
            cout << "║  Line[" << setw(2) << i << "] tag=" << setw(3) << lines[i].tag
                 << "  data=[";
            for (int j = 0; j < BLOCK_SIZE; j++) {
                cout << setw(4) << lines[i].data[j];
                if (j < BLOCK_SIZE - 1) cout << ",";
            }
            cout << "]" << endl;
        }
    }
}
