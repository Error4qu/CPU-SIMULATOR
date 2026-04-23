#include "executor.h"

void executeProgram(vector<Instruction> &program)
{
    int pc = 0; // program counter
    int n = program.size();
    PipelineStage IF, ID, EX, MEM, WB;
    int reg[32] = {0};
    int mem[1024] = {0};

    // initialize memory (optional)
    mem[10] = 5;
    mem[20] = 10;
    int cycle = 1;
    while (true)
    {
        if (pc >= n && IF.empty && ID.empty && EX.empty && MEM.empty && WB.empty)
            break;

        // WB stage (just clear)
        if (!WB.empty)
        {
            WB.empty = true;
        }

        // MEM → WB
        WB = MEM;

        // EX → MEM
        MEM = EX;

        // ID → EX
        EX = ID;

        // IF → ID
        ID = IF;
        if (pc < n)
        {
            IF.inst = program[pc];
            IF.empty = false;
            pc++;
        }
        else
        {
            IF.empty = true;
        }
        if (!EX.empty)
        {
            Instruction inst = EX.inst;

            if (inst.opcode == "ADD")
            {
                reg[inst.dest] = reg[inst.src1] + reg[inst.src2];
            }
            else if (inst.opcode == "SUB")
            {
                reg[inst.dest] = reg[inst.src1] - reg[inst.src2];
            }
            else if (inst.opcode == "LOAD")
            {
                reg[inst.dest] = mem[inst.src1];
            }
            else if (inst.opcode == "STORE")
            {
                mem[inst.dest] = reg[inst.src1];
            }
        }
        cout << "Cycle " << cycle << ":\n";

        cout << "IF: " << (IF.empty ? "Empty" : IF.inst.opcode) << " | ";
        cout << "ID: " << (ID.empty ? "Empty" : ID.inst.opcode) << " | ";
        cout << "EX: " << (EX.empty ? "Empty" : EX.inst.opcode) << " | ";
        cout << "MEM: " << (MEM.empty ? "Empty" : MEM.inst.opcode) << " | ";
        cout << "WB: " << (WB.empty ? "Empty" : WB.inst.opcode) << endl;

        cycle++;
    }

    // print registers
    cout << "Registers:\n";
    for (int i = 0; i < 8; i++)
    {
        cout << "R" << i << ": " << reg[i] << endl;
    }

    cout << "\nMemory[100]: " << mem[100] << endl;
}