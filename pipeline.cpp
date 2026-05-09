#include "pipeline.h"

// ═══════════════════════════════════════════════════════════
//  Load program into instruction memory
// ═══════════════════════════════════════════════════════════
void Pipeline::loadProgram(const vector<Instruction> &program) {
    instrMemory = program;
    cpu = CPUState();       // reset entire CPU state
}

void Pipeline::initMemory() {
    // Pre-load some data memory for testing
    cpu.memory[10] = 5;
    cpu.memory[20] = 10;
    cpu.memory[30] = 15;
}

// ═══════════════════════════════════════════════════════════
//  STAGE 1: INSTRUCTION FETCH (IF)
//
//  - Uses PC to index into instruction memory
//  - Reads the instruction at instrMemory[PC]
//  - Writes fetched instruction + PC into IF/ID latch
//  - Increments PC (unless halted or PC out of range)
// ═══════════════════════════════════════════════════════════
void Pipeline::stageIF() {
    // Default: inject a bubble (invalid latch)
    cpu.ifId_next.valid = false;

    if (cpu.halted) return;

    int pc = cpu.pc;
    if (pc < 0 || pc >= (int)instrMemory.size()) {
        // PC fell off the end of program — stop fetching
        cpu.halted = true;
        return;
    }

    // ── Fetch the instruction from instruction memory ──
    Instruction fetched = instrMemory[pc];

    // ── Fill IF/ID pipeline register ──
    cpu.ifId_next.inst  = fetched;
    cpu.ifId_next.pc    = pc;
    cpu.ifId_next.valid = true;

    // ── Advance PC ──
    cpu.pc = pc + 1;

    // If we fetched a HALT, stop fetching after this
    if (fetched.opcode == OP_HALT) {
        cpu.halted = true;
    }
}

// ═══════════════════════════════════════════════════════════
//  STAGE 2: INSTRUCTION DECODE / REGISTER READ (ID)
//
//  - Reads IF/ID latch
//  - Decodes the opcode (already done by parser, but this
//    is where a real CPU would do it)
//  - Reads source registers from the register file
//  - Sign-extends the immediate field
//  - Writes everything into ID/EX latch
// ═══════════════════════════════════════════════════════════
void Pipeline::stageID() {
    cpu.idEx_next.valid = false;

    if (!cpu.ifId.valid) return;

    Instruction inst = cpu.ifId.inst;

    // ── Read register file ──
    // R0 is hardwired to 0 in our ISA
    int readData1 = (inst.rs1 == 0) ? 0 : cpu.registers[inst.rs1];
    int readData2 = (inst.rs2 == 0) ? 0 : cpu.registers[inst.rs2];

    // ── Fill ID/EX latch ──
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
//  STAGE 3: EXECUTE / ADDRESS CALCULATION (EX)
//
//  - Reads ID/EX latch
//  - Performs ALU operation based on opcode:
//      R-type (ADD, SUB, AND, OR, SLT):
//          aluResult = readData1 <op> readData2
//      I-type (ADDI):
//          aluResult = readData1 + imm
//      LOAD / STORE:
//          aluResult = readData1 + imm   (effective address)
//      BEQ / BNE:
//          compare readData1 vs readData2
//          branchTarget = pc + 1 + imm
//      JAL:
//          aluResult = pc + 1  (return address to save)
//          branchTarget = pc + imm
//  - Writes results into EX/MEM latch
// ═══════════════════════════════════════════════════════════
void Pipeline::stageEX() {
    cpu.exMem_next.valid = false;

    if (!cpu.idEx.valid) return;

    Opcode op   = cpu.idEx.opcode;
    int d1      = cpu.idEx.readData1;
    int d2      = cpu.idEx.readData2;
    int imm     = cpu.idEx.imm;
    int pc      = cpu.idEx.pc;

    int  aluResult    = 0;
    int  branchTarget = 0;
    bool branchTaken  = false;

    switch (op) {
        // ── R-type ALU ──
        case OP_ADD:  aluResult = d1 + d2;              break;
        case OP_SUB:  aluResult = d1 - d2;              break;
        case OP_AND:  aluResult = d1 & d2;              break;
        case OP_OR:   aluResult = d1 | d2;              break;
        case OP_SLT:  aluResult = (d1 < d2) ? 1 : 0;   break;

        // ── I-type ALU ──
        case OP_ADDI: aluResult = d1 + imm;             break;

        // ── Memory address calculation ──
        case OP_LOAD:
        case OP_STORE:
            aluResult = d1 + imm;  // effective address
            break;

        // ── Branch ──
        case OP_BEQ:
            branchTarget = pc + 1 + imm;
            branchTaken  = (d1 == d2);
            break;
        case OP_BNE:
            branchTarget = pc + 1 + imm;
            branchTaken  = (d1 != d2);
            break;

        // ── Jump and Link ──
        case OP_JAL:
            aluResult    = pc + 1;      // return address
            branchTarget = pc + imm;
            branchTaken  = true;        // unconditional
            break;

        case OP_HALT:
        case OP_NOP:
        default:
            break;
    }

    // ── Handle branch/jump: flush IF and ID, redirect PC ──
    if (branchTaken) {
        cpu.pc = branchTarget;
        cpu.halted = false;     // un-halt if PC was past end

        // Squash the two instructions in IF and ID
        // (they were fetched speculatively after the branch)
        cpu.ifId_next.valid  = false;
        cpu.idEx_next.valid  = false;
    }

    // ── Fill EX/MEM latch ──
    cpu.exMem_next.opcode       = op;
    cpu.exMem_next.rd           = cpu.idEx.rd;
    cpu.exMem_next.aluResult    = aluResult;
    cpu.exMem_next.writeData    = d2;   // for STORE: value from rs2
    cpu.exMem_next.branchTarget = branchTarget;
    cpu.exMem_next.branchTaken  = branchTaken;
    cpu.exMem_next.rawText      = cpu.idEx.rawText;
    cpu.exMem_next.valid        = true;
}

// ═══════════════════════════════════════════════════════════
//  STAGE 4: MEMORY ACCESS (MEM)
//
//  - Reads EX/MEM latch
//  - LOAD:  read data memory at aluResult (effective addr)
//  - STORE: write data memory at aluResult with writeData
//  - Other: just pass aluResult through
//  - Writes into MEM/WB latch
// ═══════════════════════════════════════════════════════════
void Pipeline::stageMEM() {
    cpu.memWb_next.valid = false;

    if (!cpu.exMem.valid) return;

    Opcode op       = cpu.exMem.opcode;
    int aluResult   = cpu.exMem.aluResult;
    int memData     = 0;

    if (op == OP_LOAD) {
        // ── Read from data memory ──
        if (aluResult >= 0 && aluResult < MEMORY_SIZE) {
            memData = cpu.memory[aluResult];
        } else {
            cerr << "[MEM] Load address out of bounds: " << aluResult << endl;
        }
    }
    else if (op == OP_STORE) {
        // ── Write to data memory ──
        if (aluResult >= 0 && aluResult < MEMORY_SIZE) {
            cpu.memory[aluResult] = cpu.exMem.writeData;
        } else {
            cerr << "[MEM] Store address out of bounds: " << aluResult << endl;
        }
    }

    // ── Fill MEM/WB latch ──
    cpu.memWb_next.opcode    = op;
    cpu.memWb_next.rd        = cpu.exMem.rd;
    cpu.memWb_next.aluResult = aluResult;
    cpu.memWb_next.memData   = memData;
    cpu.memWb_next.rawText   = cpu.exMem.rawText;
    cpu.memWb_next.valid     = true;
}

// ═══════════════════════════════════════════════════════════
//  STAGE 5: WRITE-BACK (WB)
//
//  - Reads MEM/WB latch
//  - Writes the result back into the register file
//      LOAD  → write memData into rd
//      R-type/ADDI/JAL → write aluResult into rd
//      STORE/BEQ/BNE/HALT/NOP → no write-back
//  - R0 is hardwired to 0 and is never written
// ═══════════════════════════════════════════════════════════
void Pipeline::stageWB() {
    cpu.lastWB.valid = false;

    if (!cpu.memWb.valid) return;

    Opcode op = cpu.memWb.opcode;
    int    rd = cpu.memWb.rd;

    // Determine if this instruction writes to a register
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
        default:
            break;
    }

    if (writesReg && rd != 0) {
        cpu.registers[rd] = writeVal;
    }

    // Track for display
    cpu.lastWB.rawText  = cpu.memWb.rawText;
    cpu.lastWB.opcode   = op;
    cpu.lastWB.rd       = rd;
    cpu.lastWB.writeVal = writeVal;
    cpu.lastWB.valid    = true;

    cpu.instructionsCompleted++;
}

// ═══════════════════════════════════════════════════════════
//  Clock Edge: advance all latches simultaneously
// ═══════════════════════════════════════════════════════════
void Pipeline::advanceLatches() {
    cpu.ifId  = cpu.ifId_next;
    cpu.idEx  = cpu.idEx_next;
    cpu.exMem = cpu.exMem_next;
    cpu.memWb = cpu.memWb_next;
}

// ═══════════════════════════════════════════════════════════
//  tick() — execute one complete clock cycle
//
//  IMPORTANT: Stages run in REVERSE order (WB → MEM → EX → ID → IF)
//  so that writes from later stages are visible to earlier stages
//  within the same cycle. Each stage reads from its INPUT latch
//  (current) and writes to the _next latch. At the end,
//  advanceLatches() copies _next → current (simulating clock edge).
// ═══════════════════════════════════════════════════════════
bool Pipeline::tick() {
    cpu.totalCycles++;

    // ── Clear next-latches (default: bubbles) ──
    cpu.ifId_next  = IF_ID_Latch();
    cpu.idEx_next  = ID_EX_Latch();
    cpu.exMem_next = EX_MEM_Latch();
    cpu.memWb_next = MEM_WB_Latch();

    // ── Run all 5 stages (reverse order to avoid conflicts) ──
    stageWB();
    stageMEM();
    stageEX();
    stageID();
    stageIF();

    // ── Advance pipeline registers (clock edge) ──
    advanceLatches();

    // ── Check if pipeline is fully drained ──
    bool pipelineEmpty = !cpu.ifId.valid && !cpu.idEx.valid
                      && !cpu.exMem.valid && !cpu.memWb.valid;

    return !(cpu.halted && pipelineEmpty);
}

// ═══════════════════════════════════════════════════════════
//  Printing
// ═══════════════════════════════════════════════════════════
string Pipeline::opcodeToString(Opcode op) const {
    switch (op) {
        case OP_NOP:   return "NOP";
        case OP_ADD:   return "ADD";
        case OP_SUB:   return "SUB";
        case OP_AND:   return "AND";
        case OP_OR:    return "OR";
        case OP_SLT:   return "SLT";
        case OP_ADDI:  return "ADDI";
        case OP_LOAD:  return "LOAD";
        case OP_STORE: return "STORE";
        case OP_BEQ:   return "BEQ";
        case OP_BNE:   return "BNE";
        case OP_JAL:   return "JAL";
        case OP_HALT:  return "HALT";
        default:       return "???";
    }
}

void Pipeline::printCycleState() const {
    cout << "┌─────────────────────────────────────────────────────────────────────┐" << endl;
    cout << "│  CYCLE " << setw(4) << cpu.totalCycles
         << "                                              PC = " << setw(3) << cpu.pc << "  │" << endl;
    cout << "├─────────┬───────────────────────────────────────────────────────────┤" << endl;

    // IF stage
    cout << "│  IF     │ ";
    if (cpu.ifId.valid)
        cout << setw(57) << left << cpu.ifId.inst.rawText;
    else
        cout << setw(57) << left << "--- bubble ---";
    cout << " │" << endl;

    // ID stage
    cout << "│  ID     │ ";
    if (cpu.idEx.valid)
        cout << setw(57) << left
             << (cpu.idEx.rawText + "  [rs1=" + to_string(cpu.idEx.readData1)
                + " rs2=" + to_string(cpu.idEx.readData2) + "]");
    else
        cout << setw(57) << left << "--- bubble ---";
    cout << " │" << endl;

    // EX stage
    cout << "│  EX     │ ";
    if (cpu.exMem.valid)
        cout << setw(57) << left
             << (cpu.exMem.rawText + "  [alu=" + to_string(cpu.exMem.aluResult)
                + (cpu.exMem.branchTaken ? " TAKEN" : "") + "]");
    else
        cout << setw(57) << left << "--- bubble ---";
    cout << " │" << endl;

    // MEM stage
    cout << "│  MEM    │ ";
    if (cpu.memWb.valid) {
        string info = cpu.memWb.rawText;
        if (cpu.memWb.opcode == OP_LOAD)
            info += "  [mem_read=" + to_string(cpu.memWb.memData) + "]";
        else if (cpu.memWb.opcode == OP_STORE)
            info += "  [stored]";
        else
            info += "  [pass_through]";
        cout << setw(57) << left << info;
    } else {
        cout << setw(57) << left << "--- bubble ---";
    }
    cout << " │" << endl;

    // WB stage
    cout << "│  WB     │ ";
    if (cpu.lastWB.valid) {
        string info = cpu.lastWB.rawText;
        if (cpu.lastWB.opcode == OP_STORE || cpu.lastWB.opcode == OP_BEQ
            || cpu.lastWB.opcode == OP_BNE || cpu.lastWB.opcode == OP_HALT
            || cpu.lastWB.opcode == OP_NOP) {
            info += "  [no write-back]";
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
    cout << "║  REGISTER FILE                                      ║" << endl;
    cout << "╠═══════════════════════════════════════════════════════╣" << endl;
    for (int i = 0; i < NUM_REGISTERS; i++) {
        if (cpu.registers[i] != 0) {
            cout << "║  R" << setw(2) << i << " = " << setw(10) << cpu.registers[i];
            cout << "                                    ║" << endl;
        }
    }
    cout << "╠═══════════════════════════════════════════════════════╣" << endl;
    cout << "║  DATA MEMORY (non-zero entries)                      ║" << endl;
    cout << "╠═══════════════════════════════════════════════════════╣" << endl;
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (cpu.memory[i] != 0) {
            cout << "║  Mem[" << setw(4) << i << "] = " << setw(10) << cpu.memory[i];
            cout << "                               ║" << endl;
        }
    }
    cout << "╚═══════════════════════════════════════════════════════╝" << endl;
}
