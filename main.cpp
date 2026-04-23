#include "parser.h"
#include "executor.h"

int main() {
    vector<Instruction> program = parseInput("input.txt");

    executeProgram(program);

    return 0;
}