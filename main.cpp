#include "parser.h"
#include "pipeline.h"

int main() {
    vector<Instruction> p = parse("input.txt");
    setup(p);
    cout << "=== Pipeline ===" << endl;
    while (tick())
        printCycle();
    printCycle();
    printResult();
    return 0;
}