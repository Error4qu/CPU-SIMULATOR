# CPU Pipeline Simulator (5-Stage)

A cycle-accurate 5-stage pipelined CPU simulator modeled after the classic MIPS/RISC-V datapath. Each pipeline stage performs its real-world function — fetch reads instruction memory, decode reads the register file, execute runs the ALU, memory accesses data memory, and write-back commits results.

## Pipeline Architecture

```
┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐
│   IF   │───▶│   ID   │───▶│   EX   │───▶│  MEM   │───▶│   WB   │
│ Fetch  │    │ Decode │    │Execute │    │Memory  │    │Write   │
│        │    │+ Reg   │    │+ Addr  │    │Access  │    │Back    │
│        │    │  Read  │    │  Calc  │    │        │    │        │
└────────┘    └────────┘    └────────┘    └────────┘    └────────┘
     │              │             │             │
  IF/ID          ID/EX        EX/MEM        MEM/WB
  Latch          Latch        Latch         Latch
```

## What Each Stage Does

| Stage | Function | Details |
|-------|----------|---------|
| **IF** | Instruction Fetch | Uses PC to index into instruction memory, fetches the instruction, increments PC |
| **ID** | Decode + Register Read | Decodes opcode, reads source registers (rs1, rs2) from the register file, sign-extends immediate |
| **EX** | Execute + Address Calc | ALU performs arithmetic (ADD/SUB), computes effective address (LOAD/STORE), evaluates branch conditions |
| **MEM** | Memory Access | LOAD reads from data memory, STORE writes to data memory, other instructions pass through |
| **WB** | Write-Back | Writes ALU result or memory data back to the destination register (R0 hardwired to 0) |

## ISA (Instruction Set)

### R-Type (Register-Register)
```
ADD  Rd, Rs1, Rs2       # Rd = Rs1 + Rs2
SUB  Rd, Rs1, Rs2       # Rd = Rs1 - Rs2
AND  Rd, Rs1, Rs2       # Rd = Rs1 & Rs2
OR   Rd, Rs1, Rs2       # Rd = Rs1 | Rs2
SLT  Rd, Rs1, Rs2       # Rd = (Rs1 < Rs2) ? 1 : 0
```

### I-Type (Immediate)
```
ADDI Rd, Rs1, Imm       # Rd = Rs1 + Imm
```

### Memory
```
LOAD  Rd, offset(Rs1)   # Rd = Memory[Rs1 + offset]
STORE Rs2, offset(Rs1)  # Memory[Rs1 + offset] = Rs2
```

### Branch & Jump
```
BEQ  Rs1, Rs2, offset   # if (Rs1 == Rs2) PC += offset
BNE  Rs1, Rs2, offset   # if (Rs1 != Rs2) PC += offset
JAL  Rd, offset          # Rd = PC+1; PC += offset
```

### Control
```
HALT                     # Stop fetching instructions
NOP                      # No operation (bubble)
```

## Design Details

### Double-Buffered Pipeline Registers
Each latch has a `current` and `_next` version. Stages read from `current` and write to `_next`. At the clock edge, all `_next` values are copied to `current` simultaneously — preventing read/write ordering issues within a single cycle.

### Reverse-Order Stage Execution
Stages execute in WB → MEM → EX → ID → IF order within each `tick()`. This ensures WB writes to the register file before ID reads from it, modeling the real hardware behavior where the register file supports same-cycle write-then-read.

### Branch Resolution
Branches are resolved in the EX stage. When a branch is taken:
- PC is redirected to the branch target
- The two instructions fetched after the branch (in IF and ID) are squashed (replaced with bubbles)
- This results in a 2-cycle branch penalty

### R0 = 0 Convention
Register R0 is hardwired to zero (MIPS/RISC-V convention). Writes to R0 are silently discarded.

## Project Structure

```
CPU_SIMULATOR/
├── main.cpp          # Entry point — loads program, runs simulation loop
├── parser.h/.cpp     # Assembly parser — text file → instruction memory
├── pipeline.h/.cpp   # 5-stage pipeline engine (IF, ID, EX, MEM, WB)
├── utils.h           # ISA definitions, pipeline latch structs, CPU state
└── input.txt         # Test assembly program
```

## Build & Run

```bash
g++ -std=c++17 -O2 -o simulator.exe main.cpp parser.cpp pipeline.cpp
./simulator.exe
```

## Sample Output

```
╔═══════════════════════════════════════════════════════╗
║        CPU PIPELINE SIMULATOR  (5-Stage)             ║
║        IF → ID → EX → MEM → WB                     ║
╚═══════════════════════════════════════════════════════╝

┌─────────────────────────────────────────────────────────────────────┐
│  CYCLE    1                                              PC =   1  │
├─────────┬───────────────────────────────────────────────────────────┤
│  IF     │ LOAD R1, 10(R0)                                          │
│  ID     │ --- bubble ---                                           │
│  EX     │ --- bubble ---                                           │
│  MEM    │ --- bubble ---                                           │
│  WB     │ --- bubble ---                                           │
└─────────┴───────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  CYCLE    4                                              PC =   4  │
├─────────┬───────────────────────────────────────────────────────────┤
│  IF     │ SUB R4, R3, R1                                           │
│  ID     │ ADD R3, R1, R2  [rs1=0 rs2=0]   ← data hazard!          │
│  EX     │ LOAD R2, 20(R0)  [alu=20]                               │
│  MEM    │ LOAD R1, 10(R0)  [mem_read=5]                            │
│  WB     │ --- bubble ---                                           │
└─────────┴───────────────────────────────────────────────────────────┘
```

> **Note:** The pipeline currently has **no hazard handling**. Data hazards are visible in the output — for example, `ADD` reads R1/R2 as 0 because the preceding LOADs haven't reached WB yet. This is correct behavior for a bare pipeline without forwarding or stalling.

## Roadmap

- [ ] **Data Hazard Detection** — Detect RAW (Read After Write) dependencies between stages
- [ ] **Forwarding / Bypassing** — EX→EX and MEM→EX forwarding paths to resolve most data hazards
- [ ] **Load-Use Stall** — Insert 1-cycle stall when a LOAD is immediately followed by a dependent instruction
- [ ] **Branch Prediction** — Static or dynamic prediction to reduce branch penalty
- [ ] **Performance Counters** — Track stalls, flushes, forwarding events, CPI breakdown
