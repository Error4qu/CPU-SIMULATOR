# CPU Pipeline Simulator with Hazard Handling & Cache

A cycle-accurate 5-stage pipelined CPU simulator with **data hazard handling** (forwarding + stalling) and a **direct-mapped cache simulator**. Modeled after the classic MIPS/RISC-V datapath from Patterson & Hennessy.

## Pipeline Architecture

```
┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐
│   IF   │───▶│   ID   │───▶│   EX   │───▶│  MEM   │───▶│   WB   │
│ Fetch  │    │ Decode │    │Execute │    │Memory  │    │Write   │
│        │    │+ Reg   │    │+ Addr  │    │Access  │    │Back    │
│        │    │  Read  │    │  Calc  │    │(Cache) │    │        │
└────────┘    └────────┘    └────────┘    └────────┘    └────────┘
     │              │             │             │
  IF/ID          ID/EX        EX/MEM        MEM/WB
  Latch          Latch        Latch         Latch
```

## Features

### 5-Stage Pipeline
| Stage | Function |
|-------|----------|
| **IF** | Fetches instruction from instruction memory using PC |
| **ID** | Decodes opcode, reads source registers from register file |
| **EX** | ALU computes arithmetic, addresses, branch conditions |
| **MEM** | LOAD/STORE access data memory **through cache** |
| **WB** | Writes result back to register file |

### Data Hazard Handling
- **Forwarding (Bypassing)**: Two forwarding paths to avoid stalls
  - **EX/MEM → EX**: Forward ALU result from MEM stage back to EX (1-cycle-ago instruction)
  - **MEM/WB → EX**: Forward result from WB stage back to EX (2-cycles-ago instruction)
- **Load-Use Stall**: When a LOAD is immediately followed by an instruction that needs its result, the pipeline stalls for 1 cycle (LOAD data isn't available until after MEM stage — forwarding alone can't fix this)
- **Branch Flush**: When a branch is taken in EX, the 2 speculatively-fetched instructions in IF and ID are squashed

### Direct-Mapped Cache
- **16 cache lines × 4 words = 64 words** total capacity
- **Address breakdown**: `offset = addr % 4`, `index = (addr/4) % 16`, `tag = addr / 64`
- **Write-through policy**: Writes always go to main memory
- **No-allocate on write miss**: STORE misses don't bring blocks into cache
- Tracks hits, misses, and hit rate

## ISA (13 Instructions)

| Type | Instructions | Format |
|------|-------------|--------|
| R-type | `ADD`, `SUB`, `AND`, `OR`, `SLT` | `OP Rd, Rs1, Rs2` |
| I-type | `ADDI` | `OP Rd, Rs1, Imm` |
| Memory | `LOAD`, `STORE` | `LOAD Rd, offset(Rs1)` / `STORE Rs2, offset(Rs1)` |
| Branch | `BEQ`, `BNE` | `OP Rs1, Rs2, offset` |
| Jump | `JAL` | `JAL Rd, offset` |
| Control | `HALT`, `NOP` | (no operands) |

## Project Structure

```
CPU_SIMULATOR/
├── main.cpp          # Entry point
├── parser.h/.cpp     # Assembly parser → instruction memory
├── pipeline.h/.cpp   # 5-stage pipeline with hazard handling
├── cache.h/.cpp      # Direct-mapped cache simulator
├── utils.h           # ISA, pipeline latches, CPU state
└── input.txt         # Test assembly program
```

## Build & Run

```bash
g++ -std=c++17 -O2 -o simulator.exe main.cpp parser.cpp pipeline.cpp cache.cpp
./simulator.exe
```

## Sample Output (Key Cycles)

**Cycle 4 — Load-Use Stall in action:**
```
│  CYCLE    4                                              PC =   3  │
│  IF     │ ADD R3, R1, R2          ← frozen (stall)                 │
│  ID     │ --- bubble/stall ---    ← bubble injected                │
│  EX     │ LOAD R1, 10(R0)  [alu=10]                               │
│  MEM    │ --- bubble ---                                           │
```
PC stays at 3 — the pipeline stalled because ADD needs R1 but LOAD R1 is still in EX.

**Final Results (with hazard handling — correct values!):**
```
║  R3  = 15          (5 + 10, correctly forwarded)
║  R4  = 10          (15 - 5, correctly forwarded)
║  R11 = 15          (cache hits for re-accessed data)
║
║  Forwarding Events :      4
║  Load-Use Stalls   :      2
║  Cache Hit Rate    :  33.3%  (2 hits / 6 accesses)
```

## Key Design Decisions

1. **Double-Buffered Latches**: Each pipeline register has `current` + `_next`. Stages read `current`, write `_next`. At clock edge, `_next → current`. Prevents ordering issues.

2. **Reverse-Order Execution**: Stages run WB → IF so that WB writes registers before ID reads them (models real hardware where register file write port fires first half of clock, read port second half).

3. **Forwarding Priority**: EX/MEM forwarding takes priority over MEM/WB when both write to the same register (closer instruction's value is more recent).

4. **Write-Through Cache**: Simplest cache write policy — always writes to memory, keeping cache and memory consistent. No dirty bits needed.
