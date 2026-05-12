#include "pipeline.h"

// ── Global state ──
int pc = 0;
int R[8] = {0};
int mem[256] = {0};
bool halted = false;
int cycle_count = 0;

LatchA la, la_next;
LatchB lb, lb_next;
LatchC lc, lc_next;
LatchD ld, ld_next;

Cache cache;
vector<Instruction> prog;

void setup(vector<Instruction> &p) {
    prog = p;
    mem[10] = 5;
    mem[20] = 10;
}

// ────────────────────────
//  STAGE 1: FETCH
// ────────────────────────
void fetch() {
    if (halted) return;
    if (pc >= (int)prog.size()) { halted = true; return; }

    la_next.inst  = prog[pc];
    la_next.valid = true;
    pc++;

    if (prog[pc - 1].op == HALT)
        halted = true;
}

// ────────────────────────
//  STAGE 2: DECODE
// ────────────────────────
void decode() {
    if (!la.valid) return;

    lb_next.op   = la.inst.op;
    lb_next.rd   = la.inst.rd;
    lb_next.rs1  = la.inst.rs1;
    lb_next.rs2  = la.inst.rs2;
    lb_next.val1 = R[la.inst.rs1];
    lb_next.val2 = R[la.inst.rs2];
    lb_next.imm  = la.inst.imm;
    lb_next.text = la.inst.text;
    lb_next.valid = true;
}

// ────────────────────────
//  STAGE 3: EXECUTE
//  + FORWARDING
// ────────────────────────
void execute() {
    if (!lb.valid) return;

    int v1 = lb.val1;
    int v2 = lb.val2;

    // Forward from MEM/WB (older, check first)
    if (ld.valid && ld.op != STORE && ld.op != HALT) {
        if (lb.rs1 == ld.rd) v1 = ld.result;
        if (lb.rs2 == ld.rd) v2 = ld.result;
    }

    // Forward from EX/MEM (newer, wins over MEM/WB)
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
    else if (lb.op == LOAD) lc_next.result = lb.imm;       // address
    else if (lb.op == STORE){ lc_next.result = lb.imm; lc_next.storeVal = v1; }
}

// ────────────────────────
//  STAGE 4: MEMORY
//  goes through CACHE
// ────────────────────────
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

// ────────────────────────
//  STAGE 5: WRITEBACK
// ────────────────────────
void writeback() {
    if (!ld.valid) return;

    if (ld.op == ADD || ld.op == SUB || ld.op == MOV || ld.op == LOAD)
        R[ld.rd] = ld.result;
}

// ────────────────────────
//  TICK = one clock cycle
// ────────────────────────
bool tick() {
    cycle_count++;

    // Clear next latches
    la_next = LatchA();
    lb_next = LatchB();
    lc_next = LatchC();
    ld_next = LatchD();

    // Detect load-use stall
    bool stall = false;
    if (lb.valid && lb.op == LOAD && la.valid) {
        if (la.inst.rs1 == lb.rd || la.inst.rs2 == lb.rd)
            stall = true;
    }

    // Run stages (reverse order)
    writeback();
    memory();
    execute();

    if (!stall) {
        decode();
        fetch();
    } else {
        la_next = la;           // freeze fetch/decode
        lb_next = LatchB();     // bubble into execute
    }

    // Clock edge: next becomes current
    la = la_next;
    lb = lb_next;
    lc = lc_next;
    ld = ld_next;

    bool empty = !la.valid && !lb.valid && !lc.valid && !ld.valid;
    return !(halted && empty);
}

void printCycle() {
    string s1 = la.valid ? la.inst.text : "---";
    string s2 = lb.valid ? lb.text : "---";
    string s3 = lc.valid ? lc.text : "---";
    string s4 = ld.valid ? ld.text : "---";

    cout << "Cycle " << cycle_count << ":  ";
    cout << "IF[" << s1 << "]  ID[" << s2 << "]  EX[" << s3 << "]  MEM[" << s4 << "]" << endl;
}

void printResult() {
    cout << "\n=== Registers ===" << endl;
    for (int i = 0; i < 8; i++)
        cout << "R" << i << "=" << R[i] << "  ";
    cout << endl;
    cout << "Mem[100]=" << mem[100] << endl;
    cout << "Cycles: " << cycle_count << endl;
    cache.print();
}
