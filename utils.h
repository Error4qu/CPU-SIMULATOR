#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
using namespace std;

enum Opcode { ADD, SUB, MOV, LOAD, STORE, BEQ, HALT };

struct Instruction {
    Opcode op;
    int rd  = 0;
    int rs1 = 0;
    int rs2 = 0;
    int imm = 0;
    string text;
};

struct LatchA {
    Instruction inst;
    int  pc = 0;
    bool valid = false;
};

struct LatchB {
    Opcode op;
    int rd = 0, rs1 = 0, rs2 = 0;
    int val1 = 0, val2 = 0;
    int imm = 0;
    int pc  = 0;
    bool valid = false;
    string text;
};

struct LatchC {
    Opcode op;
    int rd = 0;
    int result = 0;
    int storeVal = 0;
    bool valid = false;
    string text;
};

struct LatchD {
    Opcode op;
    int rd = 0;
    int result = 0;
    bool valid = false;
    string text;
};

#endif