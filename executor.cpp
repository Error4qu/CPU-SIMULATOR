#include "executor.h"

void executeProgram(vector<Instruction> &program) {
    int reg[32] = {0};
    int mem[1024] = {0};

    // initialize memory (optional)
    mem[10] = 5;
    mem[20] = 10;

    for (auto &inst : program) {
        if (inst.opcode == "ADD") {
            reg[inst.dest] = reg[inst.src1] + reg[inst.src2];
        }
        else if (inst.opcode == "SUB") {
            reg[inst.dest] = reg[inst.src1] - reg[inst.src2];
        }
        else if (inst.opcode == "LOAD") {
            reg[inst.dest] = mem[inst.src1];
        }
        else if (inst.opcode == "STORE") {
            mem[inst.dest] = reg[inst.src1];
        }
    }

    // print registers
    cout << "Registers:\n";
    for (int i = 0; i < 8; i++) {
        cout << "R" << i << ": " << reg[i] << endl;
    }

    cout << "\nMemory[100]: " << mem[100] << endl;
}