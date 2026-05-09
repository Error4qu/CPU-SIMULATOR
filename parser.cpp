#include "parser.h"

// ─────────────────────────────────────────────────────────
//  Helper: extract register number from strings like "R5"
// ─────────────────────────────────────────────────────────
static int regNum(const string &tok) {
    // Accepts R0..R31 or r0..r31
    string s = tok;
    if (s.back() == ',') s.pop_back(); // strip trailing comma if any
    return stoi(s.substr(1));
}

// ─────────────────────────────────────────────────────────
//  Helper: map mnemonic string -> Opcode enum
// ─────────────────────────────────────────────────────────
static Opcode mnemonicToOpcode(const string &m) {
    if (m == "ADD")   return OP_ADD;
    if (m == "SUB")   return OP_SUB;
    if (m == "AND")   return OP_AND;
    if (m == "OR")    return OP_OR;
    if (m == "SLT")   return OP_SLT;
    if (m == "ADDI")  return OP_ADDI;
    if (m == "LOAD")  return OP_LOAD;
    if (m == "STORE") return OP_STORE;
    if (m == "BEQ")   return OP_BEQ;
    if (m == "BNE")   return OP_BNE;
    if (m == "JAL")   return OP_JAL;
    if (m == "HALT")  return OP_HALT;
    if (m == "NOP")   return OP_NOP;
    cerr << "[Parser] Unknown mnemonic: " << m << endl;
    exit(1);
}

// ─────────────────────────────────────────────────────────
//  Parse an assembly source file into instruction memory
//
//  Supported formats:
//    ADD  R3, R1, R2       (R-type: dest, src1, src2)
//    SUB  R4, R3, R1
//    AND  R5, R1, R2
//    OR   R6, R1, R2
//    SLT  R7, R1, R2
//    ADDI R1, R0, 5        (I-type: dest, src1, imm)
//    LOAD R1, 10(R0)       (Load:   dest, offset(base))
//    STORE R4, 100(R0)     (Store:  src,  offset(base))
//    BEQ  R1, R2, 2        (Branch: src1, src2, offset)
//    BNE  R1, R2, -3
//    JAL  R31, 4           (Jump:   link_reg, offset)
//    HALT
//    NOP
// ─────────────────────────────────────────────────────────
vector<Instruction> parseProgram(const string &filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "[Parser] Cannot open file: " << filename << endl;
        exit(1);
    }

    vector<Instruction> program;
    string line;

    while (getline(file, line)) {
        // Strip carriage return if present
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // Skip blank lines and comments
        if (line.empty()) continue;
        if (line[0] == '#' || line[0] == ';') continue;

        // Tokenize
        stringstream ss(line);
        string mnemonic;
        ss >> mnemonic;

        Instruction inst;
        inst.rawText = line;
        inst.opcode  = mnemonicToOpcode(mnemonic);

        switch (inst.opcode) {
            // R-type: OP Rd, Rs1, Rs2
            case OP_ADD:
            case OP_SUB:
            case OP_AND:
            case OP_OR:
            case OP_SLT: {
                string a, b, c;
                ss >> a >> b >> c;
                inst.rd  = regNum(a);
                inst.rs1 = regNum(b);
                inst.rs2 = regNum(c);
                break;
            }

            // I-type: OP Rd, Rs1, Imm
            case OP_ADDI: {
                string a, b, c;
                ss >> a >> b >> c;
                inst.rd  = regNum(a);
                inst.rs1 = regNum(b);
                inst.imm = stoi(c);
                break;
            }

            // LOAD Rd, offset(Rs1)   or   LOAD Rd, offset  (base=R0)
            case OP_LOAD: {
                string a, b;
                ss >> a >> b;
                inst.rd = regNum(a);
                // Check for offset(Rn) format
                size_t paren = b.find('(');
                if (paren != string::npos) {
                    inst.imm = stoi(b.substr(0, paren));
                    string base = b.substr(paren + 1);
                    base.pop_back(); // remove ')'
                    inst.rs1 = regNum(base);
                } else {
                    // Simple: LOAD R1, 10  means LOAD R1, 10(R0)
                    inst.imm = stoi(b);
                    inst.rs1 = 0;
                }
                break;
            }

            // STORE Rs2, offset(Rs1)   or   STORE Rs2, offset
            case OP_STORE: {
                string a, b;
                ss >> a >> b;
                inst.rs2 = regNum(a);  // the register to store FROM
                size_t paren = b.find('(');
                if (paren != string::npos) {
                    inst.imm = stoi(b.substr(0, paren));
                    string base = b.substr(paren + 1);
                    base.pop_back();
                    inst.rs1 = regNum(base);
                } else {
                    inst.imm = stoi(b);
                    inst.rs1 = 0;
                }
                break;
            }

            // Branch: OP Rs1, Rs2, offset
            case OP_BEQ:
            case OP_BNE: {
                string a, b, c;
                ss >> a >> b >> c;
                inst.rs1 = regNum(a);
                inst.rs2 = regNum(b);
                inst.imm = stoi(c);
                break;
            }

            // JAL Rd, offset
            case OP_JAL: {
                string a, b;
                ss >> a >> b;
                inst.rd  = regNum(a);
                inst.imm = stoi(b);
                break;
            }

            case OP_HALT:
            case OP_NOP:
                break;

            default:
                break;
        }

        program.push_back(inst);
    }

    return program;
}