#include "pipeline.h"

// ═══════════════════════════════════════════════════════════
//  Setup
// ═══════════════════════════════════════════════════════════

void Pipeline::loadProgram(const vector<Instruction> &program) {
    instrMemory = program;
    cpu = CPUState();
    cache = Cache();        // reset cache too
}

void Pipeline::initMemory() {
    cpu.memory[10] = 5;
    cpu.memory[20] = 10;
    cpu.memory[30] = 15;
}

// ═══════════════════════════════════════════════════════════
//  Helper: does this opcode write to a register?
// ═══════════════════════════════════════════════════════════
bool Pipeline::writesToRegister(Opcode op) const {
    return op == OP_ADD  || op == OP_SUB  || op == OP_AND ||
           op == OP_OR   || op == OP_SLT  || op == OP_ADDI ||
           op == OP_LOAD || op == OP_JAL;
}

// ═══════════════════════════════════════════════════════════
//  HAZARD: Detect Load-Use (returns true if stall needed)
//
//  If the instruction in ID/EX (about to execute) is a LOAD,
//  and the instruction in IF/ID (about to decode) reads
//  from the LOAD's destination register → we MUST stall
//  because LOAD data isn't available until after MEM stage.
//  Forwarding alone can't fix this — we need 1 cycle delay.
// ═══════════════════════════════════════════════════════════
bool Pipeline::detectLoadUseHazard() {
    if (!cpu.idEx.valid || cpu.idEx.opcode != OP_LOAD) return false;
    if (!cpu.ifId.valid) return false;

    int loadDest = cpu.idEx.rd;
    if (loadDest == 0) return false;    // R0 is always 0, no hazard

    Instruction next = cpu.ifId.inst;

    // Which source registers does the next instruction need?
    bool usesRs1 = false, usesRs2 = false;
    switch (next.opcode) {
        case OP_ADD: case OP_SUB: case OP_AND: case OP_OR: case OP_SLT:
        case OP_BEQ: case OP_BNE:
            usesRs1 = true; usesRs2 = true; break;
        case OP_ADDI: case OP_LOAD:
            usesRs1 = true; break;
        case OP_STORE:
            usesRs1 = true; usesRs2 = true; break;
        default: break;
    }

    if (usesRs1 && next.rs1 == loadDest) return true;
    if (usesRs2 && next.rs2 == loadDest) return true;
    return false;
}

// ═══════════════════════════════════════════════════════════
//  FORWARDING: Fix operands before ALU uses them
//
//  Two forwarding paths:
//    1. EX/MEM → EX  (from the instruction 1 ahead, in MEM stage)
//       Forward exMem.aluResult (for non-LOAD instructions)
//    2. MEM/WB → EX  (from the instruction 2 ahead, in WB stage)
//       Forward memWb.aluResult or memWb.memData (for LOAD)
//
//  Priority: EX/MEM wins over MEM/WB (closer instruction)
// ═══════════════════════════════════════════════════════════
void Pipeline::applyForwarding(int &d1, int &d2) {
    int rs1 = cpu.idEx.rs1;
    int rs2 = cpu.idEx.rs2;

    // ── Forward from EX/MEM (1 stage ahead) ──
    if (cpu.exMem.valid && cpu.exMem.rd != 0 && writesToRegister(cpu.exMem.opcode)) {
        // Note: if exMem is a LOAD, we stalled already so this won't happen
        int fwdVal = cpu.exMem.aluResult;
        if (rs1 == cpu.exMem.rd) { d1 = fwdVal; cpu.forwardCount++; }
        if (rs2 == cpu.exMem.rd) { d2 = fwdVal; cpu.forwardCount++; }
    }

    // ── Forward from MEM/WB (2 stages ahead) ──
    // Only if EX/MEM didn't already forward for the same register
    if (cpu.memWb.valid && cpu.memWb.rd != 0 && writesToRegister(cpu.memWb.opcode)) {
        int fwdVal = (cpu.memWb.opcode == OP_LOAD) ? cpu.memWb.memData : cpu.memWb.aluResult;

        bool exMemForwardsRs1 = cpu.exMem.valid && cpu.exMem.rd == rs1 && writesToRegister(cpu.exMem.opcode);
        bool exMemForwardsRs2 = cpu.exMem.valid && cpu.exMem.rd == rs2 && writesToRegister(cpu.exMem.opcode);

        if (rs1 == cpu.memWb.rd && !exMemForwardsRs1) { d1 = fwdVal; cpu.forwardCount++; }
        if (rs2 == cpu.memWb.rd && !exMemForwardsRs2) { d2 = fwdVal; cpu.forwardCount++; }
    }
}

// ═══════════════════════════════════════════════════════════
//  STAGE 1: IF (Instruction Fetch)
// ═══════════════════════════════════════════════════════════
void Pipeline::stageIF() {
    cpu.ifId_next.valid = false;
    if (cpu.halted) return;

    int pc = cpu.pc;
    if (pc < 0 || pc >= (int)instrMemory.size()) {
        cpu.halted = true;
        return;
    }

    Instruction fetched = instrMemory[pc];
    cpu.ifId_next.inst  = fetched;
    cpu.ifId_next.pc    = pc;
    cpu.ifId_next.valid = true;
    cpu.pc = pc + 1;

    if (fetched.opcode == OP_HALT) {
        cpu.halted = true;
    }
}

// ═══════════════════════════════════════════════════════════
//  STAGE 2: ID (Decode + Register Read)
// ═══════════════════════════════════════════════════════════
void Pipeline::stageID() {
    cpu.idEx_next.valid = false;
    if (!cpu.ifId.valid) return;

    Instruction inst = cpu.ifId.inst;

    int readData1 = (inst.rs1 == 0) ? 0 : cpu.registers[inst.rs1];
    int readData2 = (inst.rs2 == 0) ? 0 : cpu.registers[inst.rs2];

    cpu.idEx_next.opcode    = inst.opcode;
    cpu.idEx_next.rd        = inst.rd;
    cpu.idEx_next.rs1       = inst.rs1;
    cpu.idEx_next.rs2       = inst.rs2;
    cpu.idEx_next.readData1 = readData1;
    cpu.idEx_next.readData2 = readData2;
    cpu.idEx_next.imm       = inst.imm;
    cpu.idEx_next.pc        = cpu.ifId.pc;
    cpu.idEx_next.rawText   = inst.rawText;
    cpu.idEx_next.valid     = true;
}

// ═══════════════════════════════════════════════════════════
//  STAGE 3: EX (Execute + ALU)
//  Now with FORWARDING — operands are corrected before ALU
// ═══════════════════════════════════════════════════════════
void Pipeline::stageEX() {
    cpu.exMem_next.valid = false;
    if (!cpu.idEx.valid) return;

    Opcode op = cpu.idEx.opcode;
    int d1    = cpu.idEx.readData1;
    int d2    = cpu.idEx.readData2;
    int imm   = cpu.idEx.imm;
    int pc    = cpu.idEx.pc;

    // ── Apply forwarding BEFORE using d1/d2 ──
    applyForwarding(d1, d2);

    int  aluResult    = 0;
    int  branchTarget = 0;
    bool branchTaken  = false;

    switch (op) {
        case OP_ADD:  aluResult = d1 + d2;            break;
        case OP_SUB:  aluResult = d1 - d2;            break;
        case OP_AND:  aluResult = d1 & d2;            break;
        case OP_OR:   aluResult = d1 | d2;            break;
        case OP_SLT:  aluResult = (d1 < d2) ? 1 : 0; break;
        case OP_ADDI: aluResult = d1 + imm;           break;
        case OP_LOAD:
        case OP_STORE:
            aluResult = d1 + imm;
            break;
        case OP_BEQ:
            branchTarget = pc + 1 + imm;
            branchTaken  = (d1 == d2);
            break;
        case OP_BNE:
            branchTarget = pc + 1 + imm;
            branchTaken  = (d1 != d2);
            break;
        case OP_JAL:
            aluResult    = pc + 1;
            branchTarget = pc + imm;
            branchTaken  = true;
            break;
        default: break;
    }

    // Branch taken → flush IF and ID (2-cycle penalty)
    if (branchTaken) {
        cpu.pc = branchTarget;
        cpu.halted = false;
        cpu.ifId_next.valid = false;
        cpu.idEx_next.valid = false;
        cpu.flushCount += 2;
    }

    cpu.exMem_next.opcode       = op;
    cpu.exMem_next.rd           = cpu.idEx.rd;
    cpu.exMem_next.aluResult    = aluResult;
    cpu.exMem_next.writeData    = d2;       // for STORE
    cpu.exMem_next.branchTarget = branchTarget;
    cpu.exMem_next.branchTaken  = branchTaken;
    cpu.exMem_next.rawText      = cpu.idEx.rawText;
    cpu.exMem_next.valid        = true;
}

// ═══════════════════════════════════════════════════════════
//  STAGE 4: MEM (Memory Access — through CACHE)
// ═══════════════════════════════════════════════════════════
void Pipeline::stageMEM() {
    cpu.memWb_next.valid = false;
    if (!cpu.exMem.valid) return;

    Opcode op     = cpu.exMem.opcode;
    int aluResult = cpu.exMem.aluResult;
    int memData   = 0;

    if (op == OP_LOAD) {
        if (aluResult >= 0 && aluResult < MEMORY_SIZE) {
            // Go through cache instead of directly to memory
            memData = cache.read(aluResult, cpu.memory);
        }
    }
    else if (op == OP_STORE) {
        if (aluResult >= 0 && aluResult < MEMORY_SIZE) {
            // Write-through cache
            cache.write(aluResult, cpu.exMem.writeData, cpu.memory);
        }
    }

    cpu.memWb_next.opcode    = op;
    cpu.memWb_next.rd        = cpu.exMem.rd;
    cpu.memWb_next.aluResult = aluResult;
    cpu.memWb_next.memData   = memData;
    cpu.memWb_next.rawText   = cpu.exMem.rawText;
    cpu.memWb_next.valid     = true;
}

// ═══════════════════════════════════════════════════════════
//  STAGE 5: WB (Write-Back)
// ═══════════════════════════════════════════════════════════
void Pipeline::stageWB() {
    cpu.lastWB.valid = false;
    if (!cpu.memWb.valid) return;

    Opcode op = cpu.memWb.opcode;
    int    rd = cpu.memWb.rd;
    bool writesReg = false;
    int  writeVal  = 0;

    switch (op) {
        case OP_ADD: case OP_SUB: case OP_AND: case OP_OR: case OP_SLT:
        case OP_ADDI: case OP_JAL:
            writesReg = true;
            writeVal  = cpu.memWb.aluResult;
            break;
        case OP_LOAD:
            writesReg = true;
            writeVal  = cpu.memWb.memData;
            break;
        default: break;
    }

    if (writesReg && rd != 0) {
        cpu.registers[rd] = writeVal;
    }

    cpu.lastWB.rawText  = cpu.memWb.rawText;
    cpu.lastWB.opcode   = op;
    cpu.lastWB.rd       = rd;
    cpu.lastWB.writeVal = writeVal;
    cpu.lastWB.valid    = true;
    cpu.instructionsCompleted++;
}

// ═══════════════════════════════════════════════════════════
//  Clock Edge
// ═══════════════════════════════════════════════════════════
void Pipeline::advanceLatches() {
    cpu.ifId  = cpu.ifId_next;
    cpu.idEx  = cpu.idEx_next;
    cpu.exMem = cpu.exMem_next;
    cpu.memWb = cpu.memWb_next;
}

// ═══════════════════════════════════════════════════════════
//  tick() — one clock cycle
//
//  1. Detect load-use hazard (before running any stage)
//  2. Run stages in reverse order (WB → MEM → EX → ID → IF)
//  3. If stall detected: freeze IF + ID, inject bubble into EX
//  4. Advance latches (clock edge)
// ═══════════════════════════════════════════════════════════
bool Pipeline::tick() {
    cpu.totalCycles++;

    // Clear next-latches
    cpu.ifId_next  = IF_ID_Latch();
    cpu.idEx_next  = ID_EX_Latch();
    cpu.exMem_next = EX_MEM_Latch();
    cpu.memWb_next = MEM_WB_Latch();

    // ── Detect load-use hazard BEFORE running stages ──
    bool stall = detectLoadUseHazard();

    // ── Run stages (reverse order) ──
    stageWB();
    stageMEM();
    stageEX();

    if (!stall) {
        stageID();
        stageIF();
    } else {
        // STALL: freeze IF and ID
        // Keep ifId the same (re-decode next cycle)
        cpu.ifId_next = cpu.ifId;
        // idEx_next stays as bubble (default cleared above)
        // Undo any PC change — PC should not advance
        // (IF didn't run, so PC is unchanged — nothing to undo)
        cpu.stallCount++;
    }

    advanceLatches();

    bool pipelineEmpty = !cpu.ifId.valid && !cpu.idEx.valid
                      && !cpu.exMem.valid && !cpu.memWb.valid;
    return !(cpu.halted && pipelineEmpty);
}

// ═══════════════════════════════════════════════════════════
//  Printing
// ═══════════════════════════════════════════════════════════

string Pipeline::opcodeToString(Opcode op) const {
    switch (op) {
        case OP_NOP:   return "NOP";   case OP_ADD:   return "ADD";
        case OP_SUB:   return "SUB";   case OP_AND:   return "AND";
        case OP_OR:    return "OR";    case OP_SLT:   return "SLT";
        case OP_ADDI:  return "ADDI";  case OP_LOAD:  return "LOAD";
        case OP_STORE: return "STORE"; case OP_BEQ:   return "BEQ";
        case OP_BNE:   return "BNE";   case OP_JAL:   return "JAL";
        case OP_HALT:  return "HALT";  default:       return "???";
    }
}

void Pipeline::printCycleState() const {
    cout << "┌─────────────────────────────────────────────────────────────────────┐" << endl;
    cout << "│  CYCLE " << setw(4) << cpu.totalCycles
         << "                                              PC = " << setw(3) << cpu.pc << "  │" << endl;
    cout << "├─────────┬───────────────────────────────────────────────────────────┤" << endl;

    // IF
    cout << "│  IF     │ ";
    if (cpu.ifId.valid)
        cout << setw(57) << left << cpu.ifId.inst.rawText;
    else
        cout << setw(57) << left << "--- bubble ---";
    cout << " │" << endl;

    // ID
    cout << "│  ID     │ ";
    if (cpu.idEx.valid)
        cout << setw(57) << left
             << (cpu.idEx.rawText + "  [rs1=" + to_string(cpu.idEx.readData1)
                + " rs2=" + to_string(cpu.idEx.readData2) + "]");
    else
        cout << setw(57) << left << "--- bubble/stall ---";
    cout << " │" << endl;

    // EX
    cout << "│  EX     │ ";
    if (cpu.exMem.valid)
        cout << setw(57) << left
             << (cpu.exMem.rawText + "  [alu=" + to_string(cpu.exMem.aluResult)
                + (cpu.exMem.branchTaken ? " TAKEN" : "") + "]");
    else
        cout << setw(57) << left << "--- bubble ---";
    cout << " │" << endl;

    // MEM
    cout << "│  MEM    │ ";
    if (cpu.memWb.valid) {
        string info = cpu.memWb.rawText;
        if (cpu.memWb.opcode == OP_LOAD)
            info += "  [mem=" + to_string(cpu.memWb.memData) + "]";
        else if (cpu.memWb.opcode == OP_STORE)
            info += "  [stored]";
        else
            info += "  [pass]";
        cout << setw(57) << left << info;
    } else {
        cout << setw(57) << left << "--- bubble ---";
    }
    cout << " │" << endl;

    // WB
    cout << "│  WB     │ ";
    if (cpu.lastWB.valid) {
        string info = cpu.lastWB.rawText;
        if (!writesToRegister(cpu.lastWB.opcode)) {
            info += "  [no writeback]";
        } else {
            info += "  [R" + to_string(cpu.lastWB.rd) + " <- "
                  + to_string(cpu.lastWB.writeVal) + "]";
        }
        cout << setw(57) << left << info;
    } else {
        cout << setw(57) << left << "--- bubble ---";
    }
    cout << " │" << endl;

    cout << "└─────────┴───────────────────────────────────────────────────────────┘" << endl;
    cout << endl;
}

void Pipeline::printFinalState() const {
    cout << endl;
    cout << "╔═══════════════════════════════════════════════════════╗" << endl;
    cout << "║              SIMULATION COMPLETE                     ║" << endl;
    cout << "╠═══════════════════════════════════════════════════════╣" << endl;
    cout << "║  Total Cycles        : " << setw(6) << cpu.totalCycles << "                        ║" << endl;
    cout << "║  Instructions Done   : " << setw(6) << cpu.instructionsCompleted << "                        ║" << endl;
    if (cpu.totalCycles > 0) {
        double ipc = (double)cpu.instructionsCompleted / cpu.totalCycles;
        cout << "║  IPC (Instr/Cycle)   : " << fixed << setprecision(3) << setw(6) << ipc << "                        ║" << endl;
    }
    cout << "╠═══════════════════════════════════════════════════════╣" << endl;
    cout << "║  PIPELINE HAZARD STATS                               ║" << endl;
    cout << "╠═══════════════════════════════════════════════════════╣" << endl;
    cout << "║  Forwarding Events : " << setw(6) << cpu.forwardCount << "                          ║" << endl;
    cout << "║  Load-Use Stalls   : " << setw(6) << cpu.stallCount << "                          ║" << endl;
    cout << "║  Branch Flushes    : " << setw(6) << cpu.flushCount << "                          ║" << endl;
    cout << "╠═══════════════════════════════════════════════════════╣" << endl;
    cout << "║  REGISTER FILE                                      ║" << endl;
    cout << "╠═══════════════════════════════════════════════════════╣" << endl;
    for (int i = 0; i < NUM_REGISTERS; i++) {
        if (cpu.registers[i] != 0) {
            cout << "║  R" << setw(2) << i << " = " << setw(10) << cpu.registers[i]
                 << "                                    ║" << endl;
        }
    }
    cout << "╠═══════════════════════════════════════════════════════╣" << endl;
    cout << "║  DATA MEMORY (non-zero entries)                      ║" << endl;
    cout << "╠═══════════════════════════════════════════════════════╣" << endl;
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (cpu.memory[i] != 0) {
            cout << "║  Mem[" << setw(4) << i << "] = " << setw(10) << cpu.memory[i]
                 << "                               ║" << endl;
        }
    }
    cout << "╠═══════════════════════════════════════════════════════╣" << endl;
    cache.printStats();
    cache.printContents();
    cout << "╚═══════════════════════════════════════════════════════╝" << endl;
}
