#ifndef PIPELINE_H
#define PIPELINE_H

#include "utils.h"

class Pipeline {
public:
    CPUState cpu;
    vector<Instruction> instrMemory;    // instruction memory (loaded program)

    void loadProgram(const vector<Instruction> &program);
    void initMemory();                  // pre-load data memory with test values
    bool tick();                        // advance one clock cycle; returns false when done
    void printCycleState() const;
    void printFinalState() const;

private:
    // ── Each stage reads from its INPUT latch and writes to *_next ──
    void stageIF();     // Instruction Fetch
    void stageID();     // Instruction Decode / Register Read
    void stageEX();     // Execute / Address Calculation
    void stageMEM();    // Memory Access
    void stageWB();     // Write-Back

    void advanceLatches();  // copy _next latches into current (clock edge)

    // Helpers
    string opcodeToString(Opcode op) const;
};

#endif
