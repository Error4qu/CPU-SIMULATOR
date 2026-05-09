#include "parser.h"
#include "pipeline.h"

int main() {
    cout << "╔═══════════════════════════════════════════════════════╗" << endl;
    cout << "║        CPU PIPELINE SIMULATOR  (5-Stage)             ║" << endl;
    cout << "║        IF → ID → EX → MEM → WB                     ║" << endl;
    cout << "╚═══════════════════════════════════════════════════════╝" << endl;
    cout << endl;

    // ── Parse the assembly program ──
    vector<Instruction> program = parseProgram("input.txt");

    cout << "Loaded " << program.size() << " instructions:" << endl;
    for (int i = 0; i < (int)program.size(); i++) {
        cout << "  [" << i << "] " << program[i].rawText << endl;
    }
    cout << endl;

    // ── Set up the pipeline ──
    Pipeline pipeline;
    pipeline.loadProgram(program);
    pipeline.initMemory();

    // ── Run cycle by cycle ──
    while (pipeline.tick()) {
        pipeline.printCycleState();
    }
    // Print the last cycle too
    pipeline.printCycleState();

    // ── Final results ──
    pipeline.printFinalState();

    return 0;
}