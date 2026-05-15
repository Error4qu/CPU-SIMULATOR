# CPU Pipeline Simulator with Hazard Handling & Cache

A cycle-accurate 5-stage pipelined CPU simulator with **data and control hazard handling** (forwarding, stalling, flushing) and a **direct-mapped cache simulator**. Written in clean, simple C++ to demonstrate core computer architecture concepts.

## Pipeline Architecture

```
┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐
│   IF   │───▶│   ID   │───▶│   EX   │───▶│  MEM   │───▶│   WB   │
│ Fetch  │    │ Decode │    │Execute │    │Memory  │    │Write   │
│        │    │+ Reg   │    │+ Addr  │    │Access  │    │Back    │
│        │    │  Read  │    │  Calc  │    │(Cache) │    │        │
└────────┘    └────────┘    └────────┘    └────────┘    └────────┘
     │              │             │             │
   Latch A        Latch B       Latch C       Latch D
 (IF/ID)        (ID/EX)       (EX/MEM)      (MEM/WB)
```

## Features

### 5-Stage Pipeline
| Stage | Function |
|-------|----------|
| **IF** | Fetches instruction from instruction memory using PC |
| **ID** | Decodes opcode, reads source registers from register file |
| **EX** | ALU computes arithmetic, memory addresses, and evaluates branch conditions |
| **MEM** | LOAD/STORE access data memory **through the cache** |
| **WB** | Writes result back to register file |

### Hazard Handling
- **Forwarding (Bypassing)**: Two forwarding paths to avoid stalls on RAW (Read-After-Write) data hazards.
  - **EX/MEM → EX**: Forward ALU result from MEM stage back to EX (1-cycle-ago instruction).
  - **MEM/WB → EX**: Forward result from WB stage back to EX (2-cycles-ago instruction).
- **Load-Use Stall**: When a `LOAD` is immediately followed by an instruction that needs its result, the pipeline stalls for 1 cycle. (LOAD data isn't available until after MEM stage — forwarding alone can't fix this).
- **Branch Flush**: When a `BEQ` branch is taken in EX, the 2 speculatively-fetched instructions in IF and ID are squashed (flushed) to resolve the control hazard.

### Direct-Mapped Cache
- **4 slots × 4 words = 16 words** total capacity (designed to be small and easy to trace).
- **Address breakdown**: `word = addr % 4`, `slot = (addr/4) % 4`, `tag = addr / 16`.
- **Write-through policy**: Writes always go directly to main memory to keep it synchronized.
- **Write-no-allocate**: STORE misses don't bring blocks into cache.
- Tracks hits, misses, and calculates hit rate.

## ISA (7 Instructions)

| Instruction | Format | Description |
|-------------|--------|-------------|
| `ADD` | `ADD Rd, Rs1, Rs2` | Rd = Rs1 + Rs2 |
| `SUB` | `SUB Rd, Rs1, Rs2` | Rd = Rs1 - Rs2 |
| `MOV` | `MOV Rd, Rs1` | Rd = Rs1 |
| `LOAD` | `LOAD Rd, Addr` | Loads memory at `Addr` into `Rd` |
| `STORE` | `STORE Rs1, Addr` | Stores `Rs1` into memory at `Addr` |
| `BEQ` | `BEQ Rs1, Rs2, offset` | If Rs1 == Rs2, branch `offset` instructions forward |
| `HALT` | `HALT` | Stops CPU execution |

## Project Structure

```
CPU_SIMULATOR/
├── main.cpp          # Entry point, runner
├── parser.h/.cpp     # Assembly parser (string to Instruction struct)
├── pipeline.h/.cpp   # 5-stage pipeline logic, hazard handling, globals
├── cache.h/.cpp      # Direct-mapped cache implementation
├── utils.h           # Shared structs (Opcode, Instruction, Latches)
└── input.txt         # Test assembly program
```

## Build & Run

```bash
g++ -std=c++17 -o simulator.exe main.cpp parser.cpp pipeline.cpp cache.cpp
.\simulator.exe
```

## Sample Output

```
=== Pipeline ===
Cycle 1:  IF[LOAD R1, 10]  ID[---]  EX[---]  MEM[---]
Cycle 2:  IF[LOAD R2, 20]  ID[LOAD R1, 10]  EX[---]  MEM[---]
Cycle 3:  IF[ADD R3, R1, R2]  ID[LOAD R2, 20]  EX[LOAD R1, 10]  MEM[---]
Cycle 4:  IF[ADD R3, R1, R2]  ID[---]  EX[LOAD R2, 20]  MEM[LOAD R1, 10]  <-- Load-Use Stall!
Cycle 5:  IF[SUB R4, R3, R1]  ID[ADD R3, R1, R2]  EX[---]  MEM[LOAD R2, 20]
...
Cycle 8:  IF[---]  ID[---]  EX[BEQ R1, R1, 2]  MEM[SUB R4, R3, R1]  <-- Branch flush!
...

=== Registers ===
R0=0  R1=5  R2=10  R3=15  R4=10  R5=0  R6=0  R7=0  
Mem[100]=15
Cycles: 14

=== Cache ===
Hits: 0  Misses: 3
Hit Rate: 0%
```

## Key Design Decisions

1. **Double-Buffered Latches (`_next`)**: Each pipeline latch has a `current` and `_next` version. Stages read from `current` and write to `_next`. At the end of the clock cycle, all `_next` values are copied to `current`. This perfectly simulates hardware flip-flops updating simultaneously on a clock edge and prevents stages from corrupting each other's data during the same cycle.
2. **Reverse Execution Order**: Inside the `tick()` function, stages are executed in reverse order (`writeback -> memory -> execute -> decode -> fetch`). This ensures that instructions move cleanly from one stage to the next without jumping two stages in a single cycle.
3. **Simplicity over Optimization**: The code intentionally avoids complex classes and uses global state arrays to remain readable, easy to trace, and perfect for learning/interview explanations.
