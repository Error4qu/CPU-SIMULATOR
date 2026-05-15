#ifndef PIPELINE_H
#define PIPELINE_H
#include "utils.h"
#include "cache.h"

extern int pc;
extern int R[8];
extern int mem[256];
extern bool halted;
extern int cycle_count;

extern LatchA la, la_next;
extern LatchB lb, lb_next;
extern LatchC lc, lc_next;
extern LatchD ld, ld_next;

extern Cache cache;
extern vector<Instruction> prog;

void setup(vector<Instruction> &p);
bool tick();
void printCycle();
void printResult();

#endif
