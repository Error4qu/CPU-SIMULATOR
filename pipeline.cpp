#include "pipeline.h"
int pc = 0;
int R[8] = {0};
int mem[256] = {0};
bool halted = false;
int cycle_count = 0;
bool branchTaken = false;
int branchTarget = 0;
LatchA la, la_next;
LatchB lb, lb_next;
LatchC lc, lc_next;
LatchD ld, ld_next;
Cache cache;
vector<Instruction> prog;
/**
 * Initializes the CPU pipeline with a program and sets up mock memory.
 *
 * @param p The list of instructions to load into the program memory.
 */
void setup(vector<Instruction> &p) {
    prog = p;
    mem[10] = 5;
    mem[20] = 10;
}
/**
 * Stage 1: Instruction Fetch
 * Fetches the next instruction from program memory into the IF/ID latch.
 */
void fetch() {
    if (halted) return;
    if (pc >= (int)prog.size()) { halted = true; return; }
    la_next.inst  = prog[pc];
    la_next.pc    = pc;        
    la_next.valid = true;
    pc++;
    if (prog[pc - 1].op == HALT)
        halted = true;
}
/**
 * Stage 2: Instruction Decode
 * Decodes the instruction, reads register values, and passes data to the ID/EX latch.
 */
void decode() {
    if (!la.valid) return;
    lb_next.op   = la.inst.op;
    lb_next.rd   = la.inst.rd;
    lb_next.rs1  = la.inst.rs1;
    lb_next.rs2  = la.inst.rs2;
    lb_next.val1 = R[la.inst.rs1];
    lb_next.val2 = R[la.inst.rs2];
    lb_next.imm  = la.inst.imm;
    lb_next.pc   = la.pc;      
    lb_next.text = la.inst.text;
    lb_next.valid = true;
}
/**
 * Stage 3: Execute
 * Performs ALU operations and branch evaluations.
 * Implements data forwarding from MEM and WB stages to resolve RAW hazards.
 */
void execute() {
    if (!lb.valid) return;
    int v1 = lb.val1;
    int v2 = lb.val2;
    if (ld.valid && ld.op != STORE && ld.op != HALT) {
        if (lb.rs1 == ld.rd) v1 = ld.result;
        if (lb.rs2 == ld.rd) v2 = ld.result;
    }
    if (lc.valid && lc.op != STORE && lc.op != HALT && lc.op != LOAD) {
        if (lb.rs1 == lc.rd) v1 = lc.result;
        if (lb.rs2 == lc.rd) v2 = lc.result;
    }
    lc_next.op    = lb.op;
    lc_next.rd    = lb.rd;
    lc_next.text  = lb.text;
    lc_next.valid = true;
    if (lb.op == ADD)       lc_next.result = v1 + v2;
    else if (lb.op == SUB)  lc_next.result = v1 - v2;
    else if (lb.op == MOV)  lc_next.result = v1;
    else if (lb.op == LOAD) lc_next.result = lb.imm;
    else if (lb.op == STORE){ lc_next.result = lb.imm; lc_next.storeVal = v1; }
    else if (lb.op == BEQ) {
        if (v1 == v2) {
            branchTaken  = true;
            branchTarget = lb.pc + 1 + lb.imm;
        }
    }
}
/**
 * Stage 4: Memory Access
 * Reads from or writes to the data cache based on the instruction type.
 */
void memory() {
    if (!lc.valid) return;
    ld_next.op    = lc.op;
    ld_next.rd    = lc.rd;
    ld_next.text  = lc.text;
    ld_next.valid = true;
    if (lc.op == LOAD)
        ld_next.result = cache.read(lc.result, mem);
    else if (lc.op == STORE)
        cache.write(lc.result, lc.storeVal, mem);
    else
        ld_next.result = lc.result;
}
/**
 * Stage 5: Writeback
 * Commits instruction results or loaded data back to the register file.
 */
void writeback() {
    if (!ld.valid) return;
    if (ld.op == ADD || ld.op == SUB || ld.op == MOV || ld.op == LOAD)
        R[ld.rd] = ld.result;
}
/**
 * Advances the pipeline by one clock cycle.
 * Executes stages in reverse order, detects load-use hazards to trigger stalls,
 * flushes the pipeline on taken branches, and updates all latches.
 *
 * @return True if the pipeline should continue executing, false if halted and empty.
 */
bool tick() {
    cycle_count++;
    la_next = LatchA();
    lb_next = LatchB();
    lc_next = LatchC();
    ld_next = LatchD();
    bool stall = false;
    if (lb.valid && lb.op == LOAD && la.valid) {
        if (la.inst.rs1 == lb.rd || la.inst.rs2 == lb.rd)
            stall = true;
    }
    writeback();
    memory();
    execute();
    if (branchTaken) {
        la_next = LatchA();
        lb_next = LatchB();
        pc = branchTarget;
        branchTaken = false;
        halted = false; 
    }
    else if (!stall) {
        decode();
        fetch();
    } else {
        la_next = la;
        lb_next = LatchB();
    }
    la = la_next;
    lb = lb_next;
    lc = lc_next;
    ld = ld_next;
    bool empty = !la.valid && !lb.valid && !lc.valid && !ld.valid;
    return !(halted && empty);
}
/**
 * Prints the current state of the pipeline latches for debugging and tracing.
 */
void printCycle() {
    string s1 = la.valid ? la.inst.text : "---";
    string s2 = lb.valid ? lb.text : "---";
    string s3 = lc.valid ? lc.text : "---";
    string s4 = ld.valid ? ld.text : "---";
    cout << "Cycle " << cycle_count << ":  ";
    cout << "IF[" << s1 << "]  ID[" << s2 << "]  EX[" << s3 << "]  MEM[" << s4 << "]" << endl;
}
/**
 * Prints the final state of the CPU, including registers, memory, and cache stats.
 */
void printResult() {
    cout << "\n=== Registers ===" << endl;
    for (int i = 0; i < 8; i++)
        cout << "R" << i << "=" << R[i] << "  ";
    cout << endl;
    cout << "Mem[100]=" << mem[100] << endl;
    cout << "Cycles: " << cycle_count << endl;
    cache.print();
}
