#include "parser.h"

int getRegisterNumber(string reg) {
    // R1 -> 1
    return stoi(reg.substr(1));
}

vector<Instruction> parseInput(const string &filename) {
    ifstream file(filename);
    vector<Instruction> program;
    string line;

    while (getline(file, line)) {
        stringstream ss(line);
        string op, a, b, c;

        ss >> op >> a >> b >> c;

        Instruction inst;
        inst.opcode = op;

        if (op == "ADD" || op == "SUB") {
            inst.dest = getRegisterNumber(a);
            inst.src1 = getRegisterNumber(b);
            inst.src2 = getRegisterNumber(c);
        }
        else if (op == "LOAD") {
            inst.dest = getRegisterNumber(a);
            inst.src1 = stoi(b); // memory address
        }
        else if (op == "STORE") {
            inst.dest = stoi(b); // memory address
            inst.src1 = getRegisterNumber(a);
        }

        program.push_back(inst);
    }

    return program;
}