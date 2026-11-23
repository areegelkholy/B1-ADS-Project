
#include "Debugger.h"
#include "Format.h"
#include "Isa.h"

Debugger::Debugger(Simulator& simulator, std::istream& input, std::ostream& output)
    : sim(simulator), in(input), out(output) {
}

// Cut a command line into words at spaces and tabs.
void Debugger::splitWords(const std::string& line, DynamicArray<std::string>& words) {
    std::string current;
    for (int i = 0; i < (int)line.length(); i++) {
        if (line[i] == ' ' || line[i] == '\t' || line[i] == '\r') {
            if (!current.empty()) {
                words.pushBack(current);
                current = std::string();
            }
        } else {
            current.push_back(line[i]);
        }
    }
    if (!current.empty()) {
        words.pushBack(current);
    }
}

// Addresses are written in hex ("0x0A" or "0A") and must be 0x00..0xFF.
bool Debugger::readAddress(const std::string& text, int& address) {
    if (!parseHexNumber(text, address) || address < 0 || address > MAX_ADDRESS) {
        out << "bad address '" << text << "' (use hex, 00 to FF)" << "\n";
        return false;
    }
    return true;
}

// When the program is stopped we show the instruction that runs next.
void Debugger::printNextInstruction() {
    if (sim.isFinished()) {
        return;
    }
    int pc = sim.getPc();
    int word = sim.getMemoryWord(pc);
    out << hexString(pc, 2) << ": " << hexString(word, 4) << " " << disassemble(word) << "\n";
}

void Debugger::commandBreak(const DynamicArray<std::string>& words) {
    int address = 0;
    if (words.size() != 2) {
        out << "usage: break <addr>" << "\n";
    } else if (readAddress(words[1], address)) {
        if (breakpoints.add(address)) {
            out << "breakpoint set at " << hexString(address, 2) << "\n";
        } else {
            out << "there is already a breakpoint at " << hexString(address, 2) << "\n";
        }
    }
}

void Debugger::commandDelete(const DynamicArray<std::string>& words) {
    int address = 0;
    if (words.size() != 2) {
        out << "usage: delete <addr>" << "\n";
    } else if (readAddress(words[1], address)) {
        if (breakpoints.remove(address)) {
            out << "breakpoint at " << hexString(address, 2) << " deleted" << "\n";
        } else {
            out << "no breakpoint at " << hexString(address, 2) << "\n";
        }
    }
}

// Addresses are only 0..255, so we ask the set about each one: this lists
// the breakpoints in order without needing a get all function. O(256).
void Debugger::commandBreaks() {
    bool any = false;
    for (int address = 0; address <= MAX_ADDRESS; address++) {
        if (breakpoints.contains(address)) {
            out << hexString(address, 2) << "\n";
            any = true;
        }
    }
    if (!any) {
        out << "no breakpoints" << "\n";
    }
}

void Debugger::commandRun() {
    if (sim.isFinished()) {
        out << "the program has finished" << "\n";
        return;
    }
    // The first instruction always runs, even if there is a breakpoint on it,
    // otherwise run could never leave a breakpoint
    sim.step();
    while (!sim.isFinished()) {
        // The breakpoint check happens before every instruction: O(1) with the hash set
        if (breakpoints.contains(sim.getPc())) {
            out << "breakpoint at " << hexString(sim.getPc(), 2) << "\n";
            break;
        }
        sim.step();
    }
    printNextInstruction();
}

void Debugger::commandStep(const DynamicArray<std::string>& words) {
    long count = 1;
    if (words.size() >= 2 && (!parseDecimalNumber(words[1], count) || count < 1)) {
        out << "usage: step [n]" << "\n";
        return;
    }
    if (sim.isFinished()) {
        out << "the program has finished" << "\n";
        return;
    }
    for (long i = 0; i < count && !sim.isFinished(); i++) {
        sim.step();                         // single-stepping ignores breakpoints
    }
    printNextInstruction();
}

void Debugger::commandMem(const DynamicArray<std::string>& words) {
    int address = 0;
    long count = 8;
    if (words.size() < 2 || words.size() > 3) {
        out << "usage: mem <addr> [count]" << "\n";
        return;
    }
    if (!readAddress(words[1], address)) {
        return;
    }
    if (words.size() == 3 && (!parseDecimalNumber(words[2], count) || count < 1)) {
        out << "bad count '" << words[2] << "'" << "\n";
        return;
    }
    for (long i = 0; i < count && address + i <= MAX_ADDRESS; i++) {
        int a = (int)(address + i);
        out << hexString(a, 2) << ": " << hexString(sim.getMemoryWord(a), 4) << "\n";
    }
}

// The stack grows down from 0xFB, so the top is at SP and the bottom at 0xFB
void Debugger::commandStack() {
    if (sim.getSp() >= STACK_START) {
        out << "the stack is empty" << "\n";
        return;
    }
    for (int address = sim.getSp(); address < STACK_START; address++) {
        out << hexString(address, 2) << ": " << hexString(sim.getMemoryWord(address), 4) << "\n";
    }
}

void Debugger::run() {
    out << "TISA-16 debugger. Commands: break, delete, breaks, run, step, regs, mem, stack, quit" << "\n";
    printNextInstruction();

    std::string line;
    while (true) {
        out << "(tisa) " << std::flush;
        if (!std::getline(in, line)) {
            out << "\n";
            break;                           // end of the input
        }
        DynamicArray<std::string> words;
        splitWords(line, words);
        if (words.isEmpty()) {
            continue;
        }
        std::string command = toUpper(words[0]);

        if (command == "BREAK" || command == "B") {
            commandBreak(words);
        } else if (command == "DELETE" || command == "D") {
            commandDelete(words);
        } else if (command == "BREAKS") {
            commandBreaks();
        } else if (command == "RUN" || command == "R") {
            commandRun();
        } else if (command == "STEP" || command == "S") {
            commandStep(words);
        } else if (command == "REGS") {
            sim.printRegisters();
        } else if (command == "MEM") {
            commandMem(words);
        } else if (command == "STACK") {
            commandStack();
        } else if (command == "QUIT" || command == "Q") {
            break;
        } else {
            out << "unknown command '" << words[0] << "'" << "\n";
        }
    }
}
