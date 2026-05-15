#include "parser.h"
/**
 * Parses a register string and extracts its integer index.
 * Removes trailing commas if present (e.g., "R1," becomes 1).
 *
 * @param s The string representing the register.
 * @return The integer index of the register.
 */
int reg(string s) {
    if (s.back() == ',') s.pop_back();
    return stoi(s.substr(1));
}
/**
 * Reads an assembly file and parses it into a list of Instruction objects.
 * Ignores empty lines and comments starting with '#'.
 *
 * @param filename The path to the assembly text file.
 * @return A vector containing the parsed instructions.
 */
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
        else if (op == "BEQ") {
            i.op = BEQ;
            string a, b, c;
            ss >> a >> b >> c;
            i.rs1 = reg(a); i.rs2 = reg(b); i.imm = stoi(c);
        }
        else if (op == "HALT") {
            i.op = HALT;
        }
        prog.push_back(i);
    }
    return prog;
}