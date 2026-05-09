#ifndef UTILS_H
#define UTILS_H

#include <bits/stdc++.h>
using namespace std;

// ─────────────────────────────────────────────────────────
//  ISA Definition
// ─────────────────────────────────────────────────────────

enum Opcode {
    OP_NOP,     // No operation (pipeline bubble)
    OP_ADD,     // ADD  Rd, Rs1, Rs2       -> Rd = Rs1 + Rs2
    OP_SUB,     // SUB  Rd, Rs1, Rs2       -> Rd = Rs1 - Rs2
    OP_AND,     // AND  Rd, Rs1, Rs2       -> Rd = Rs1 & Rs2
    OP_OR,      // OR   Rd, Rs1, Rs2       -> Rd = Rs1 | Rs2
    OP_SLT,     // SLT  Rd, Rs1, Rs2       -> Rd = (Rs1 < Rs2) ? 1 : 0
    OP_ADDI,    // ADDI Rd, Rs1, Imm       -> Rd = Rs1 + Imm
    OP_LOAD,    // LOAD Rd, offset(Rs1)     -> Rd = Mem[Rs1 + offset]
    OP_STORE,   // STORE Rs2, offset(Rs1)   -> Mem[Rs1 + offset] = Rs2
    OP_BEQ,     // BEQ  Rs1, Rs2, offset   -> if (Rs1 == Rs2) PC += offset
    OP_BNE,     // BNE  Rs1, Rs2, offset   -> if (Rs1 != Rs2) PC += offset
    OP_JAL,     // JAL  Rd, offset         -> Rd = PC+1; PC += offset
    OP_HALT     // HALT                     -> stop fetching
};

// ─────────────────────────────────────────────────────────
//  Decoded instruction (lives in instruction memory)
// ─────────────────────────────────────────────────────────

struct Instruction {
    Opcode opcode   = OP_NOP;
    int    rd       = 0;    // destination register
    int    rs1      = 0;    // source register 1
    int    rs2      = 0;    // source register 2
    int    imm      = 0;    // immediate / offset
    string rawText  = "";   // original assembly text for display
};

// ─────────────────────────────────────────────────────────
//  Pipeline Registers (latches between stages)
//  Each latch carries EVERYTHING the downstream stage needs.
// ─────────────────────────────────────────────────────────

// IF/ID latch  ── carries fetched instruction + PC to Decode
struct IF_ID_Latch {
    Instruction inst;
    int         pc      = 0;    // PC of this instruction
    bool        valid   = false; // is this latch holding a real instruction?
};

// ID/EX latch  ── carries decoded operands to Execute
struct ID_EX_Latch {
    Opcode opcode   = OP_NOP;
    int    rd       = 0;
    int    rs1      = 0;    // register indices (kept for hazard detection later)
    int    rs2      = 0;
    int    readData1= 0;    // actual values read from register file
    int    readData2= 0;
    int    imm      = 0;    // sign-extended immediate
    int    pc       = 0;
    bool   valid    = false;
    string rawText  = "";
};

// EX/MEM latch ── carries ALU result + branch info to Memory
struct EX_MEM_Latch {
    Opcode opcode       = OP_NOP;
    int    rd           = 0;
    int    aluResult    = 0;    // output of ALU
    int    writeData    = 0;    // data to write to memory (for STORE)
    int    branchTarget = 0;    // computed branch target address
    bool   branchTaken  = false;// did the branch condition evaluate true?
    bool   valid        = false;
    string rawText      = "";
};

// MEM/WB latch ── carries memory read / ALU result to Write-Back
struct MEM_WB_Latch {
    Opcode opcode       = OP_NOP;
    int    rd           = 0;
    int    aluResult    = 0;
    int    memData      = 0;    // data read from memory (for LOAD)
    bool   valid        = false;
    string rawText      = "";
};

// ─────────────────────────────────────────────────────────
//  CPU State (architectural state)
// ─────────────────────────────────────────────────────────

const int NUM_REGISTERS  = 32;
const int MEMORY_SIZE    = 1024;

struct CPUState {
    int  pc                     = 0;
    int  registers[NUM_REGISTERS] = {0};    // R0 is always 0
    int  memory[MEMORY_SIZE]    = {0};
    bool halted                 = false;

    // Pipeline latches
    IF_ID_Latch  ifId,  ifId_next;
    ID_EX_Latch  idEx,  idEx_next;
    EX_MEM_Latch exMem, exMem_next;
    MEM_WB_Latch memWb, memWb_next;

    // WB tracking (for display — what WB just finished this cycle)
    struct {
        string rawText  = "";
        Opcode opcode   = OP_NOP;
        int    rd       = 0;
        int    writeVal = 0;
        bool   valid    = false;
    } lastWB;

    // Stats
    int  totalCycles        = 0;
    int  instructionsCompleted = 0;
};

#endif