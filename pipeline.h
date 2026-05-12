#ifndef PIPELINE_H
#define PIPELINE_H
#include "utils.h"
#include "cache.h"

// ── Global CPU state ──
extern int pc;
extern int R[8];           // 8 registers
extern int mem[256];        // 256 words of memory
extern bool halted;
extern int cycle_count;

// ── 4 latches (current) and 4 latches (next) ──
extern LatchA la, la_next;     // between Fetch-Decode
extern LatchB lb, lb_next;     // between Decode-Execute
extern LatchC lc, lc_next;     // between Execute-Memory
extern LatchD ld, ld_next;     // between Memory-Writeback

extern Cache cache;
extern vector<Instruction> prog;

void setup(vector<Instruction> &p);
bool tick();
void printCycle();
void printResult();

#endif
