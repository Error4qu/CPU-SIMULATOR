#ifndef PIPELINE_H
#define PIPELINE_H

#include "utils.h"
#include "cache.h"

class Pipeline {
public:
    CPUState cpu;
    vector<Instruction> instrMemory;
    Cache cache;                        // direct-mapped cache for MEM stage

    void loadProgram(const vector<Instruction> &program);
    void initMemory();
    bool tick();                        // advance one clock cycle
    void printCycleState() const;
    void printFinalState() const;

private:
    void stageIF();
    void stageID();
    void stageEX();
    void stageMEM();
    void stageWB();

    void advanceLatches();

    // ── Hazard Handling ──
    bool detectLoadUseHazard();                     // returns true if stall needed
    void applyForwarding(int &d1, int &d2);         // modifies EX operands in-place
    bool writesToRegister(Opcode op) const;          // does this opcode write to rd?

    string opcodeToString(Opcode op) const;
};

#endif
