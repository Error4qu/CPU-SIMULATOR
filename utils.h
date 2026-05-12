#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
using namespace std;

// ── 6 operations ──
enum Opcode { ADD, SUB, MOV, LOAD, STORE, HALT };

// ── One instruction ──
struct Instruction {
    Opcode op;
    int rd  = 0;
    int rs1 = 0;
    int rs2 = 0;
    int imm = 0;
    string text;
};

// ── Latch between FETCH and DECODE ──
struct LatchA {
    Instruction inst;
    bool valid = false;
};

// ── Latch between DECODE and EXECUTE ──
struct LatchB {
    Opcode op;
    int rd = 0, rs1 = 0, rs2 = 0;
    int val1 = 0, val2 = 0;
    int imm = 0;
    bool valid = false;
    string text;
};

// ── Latch between EXECUTE and MEMORY ──
struct LatchC {
    Opcode op;
    int rd = 0;
    int result = 0;
    int storeVal = 0;
    bool valid = false;
    string text;
};

// ── Latch between MEMORY and WRITEBACK ──
struct LatchD {
    Opcode op;
    int rd = 0;
    int result = 0;
    bool valid = false;
    string text;
};

#endif