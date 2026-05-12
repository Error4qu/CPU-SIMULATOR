#include "parser.h"

int reg(string s) {
    if (s.back() == ',') s.pop_back();
    return stoi(s.substr(1));
}

vector<Instruction> parse(string filename) {
    ifstream file(filename);
    vector<Instruction> prog;
    string line;

    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        stringstream ss(line);
        string op;
        ss >> op;

        Instruction i;
        i.text = line;

        if (op == "ADD" || op == "SUB") {
            i.op = (op == "ADD") ? ADD : SUB;
            string a, b, c;
            ss >> a >> b >> c;
            i.rd = reg(a); i.rs1 = reg(b); i.rs2 = reg(c);
        }
        else if (op == "MOV") {
            i.op = MOV;
            string a, b;
            ss >> a >> b;
            i.rd = reg(a); i.rs1 = reg(b);
        }
        else if (op == "LOAD") {
            i.op = LOAD;
            string a, b;
            ss >> a >> b;
            i.rd = reg(a); i.imm = stoi(b);
        }
        else if (op == "STORE") {
            i.op = STORE;
            string a, b;
            ss >> a >> b;
            i.rs1 = reg(a); i.imm = stoi(b);
        }
        else if (op == "HALT") {
            i.op = HALT;
        }

        prog.push_back(i);
    }
    return prog;
}