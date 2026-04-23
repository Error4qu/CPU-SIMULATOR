#ifndef UTILS_H
#define UTILS_H

#include <bits/stdc++.h>
using namespace std;

struct Instruction {
    string opcode;
    int dest = -1;
    int src1 = -1;
    int src2 = -1;
};

#endif