// The interactive debugger (spec section 8). It drives a Simulator from
// a command prompt: set breakpoints, run, single-step, look at registers,
// memory and the stack.
//
// Data structure used here:
//   HashSet breakpoints, the addresses where run must stop
//   The simulator asks "is this address a breakpoint?" before every
//   instruction, so the check must be O(1): that is why it is a hash set.
//
// Commands:
//   break <addr> / b     set a breakpoint (address in hex)
//   delete <addr> / d    remove a breakpoint
//   breaks               list the breakpoints
//   run / r              run until a breakpoint, HALT or an error
//   step [n] / s         execute 1 (or n) instructions, ignoring breakpoints
//   regs                 print PC SP Z N IE and R0..R7
//   mem <addr> [count]   print count words (default 8) starting at addr
//   stack                print the words on the stack, top first
//   quit / q             leave the debugger

#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <istream>
#include <ostream>
#include <string>
#include "../ds/DynamicArray.h"
#include "../ds/HashSet.h"
#include "Simulator.h"

class Debugger {
private:
    Simulator& sim;
    std::istream& in;
    std::ostream& out;
    HashSet breakpoints;

    void printNextInstruction();                       // "AA: WWWW <disassembly>"
    void commandBreak(const DynamicArray<std::string>& words);
    void commandDelete(const DynamicArray<std::string>& words);
    void commandBreaks();
    void commandRun();
    void commandStep(const DynamicArray<std::string>& words);
    void commandMem(const DynamicArray<std::string>& words);
    void commandStack();
    bool readAddress(const std::string& text, int& address);
    void splitWords(const std::string& line, DynamicArray<std::string>& words);

public:
    Debugger(Simulator& simulator, std::istream& input, std::ostream& output);

    // Show the prompt and handle commands until "quit" or the end of the input.
    void run();
};

#endif
