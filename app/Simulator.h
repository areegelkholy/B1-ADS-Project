// Simulator.h
// The TISA-16 simulator: a pretend CPU that runs the machine words
// produced by the assembler.
//
// ONE CYCLE (step()):
//   1. interrupt check   (pending-interrupt queue)
//   2. fetch             (from the 4-entry prefetch queue)
//   3. decode            (cut the 16-bit word into its fields)
//   4. execute           (registers, memory, flags, stack ...)
//
// Data structures used here:
//   DynamicArray<int>   memory (256 words), registers (8), execution counters
//   Queue<PrefetchEntry>  the instruction prefetch buffer (max 4 entries)
//   Queue<Interrupt>      interrupts from the input file, waiting for their cycle
//   Queue<int>            the pending-interrupt queue (FIFO)
//   (the debugger adds a HashSet of breakpoints)
//
// THE CALL / RET / PUSH / POP STACK is NOT the Stack class. In TISA-16 it
// lives INSIDE the simulated memory , because
// programs can read and overwrite it with LW / SW. So pushWord()/popWord()
// work directly on the memory array.
//
// All output (OUT values, HALT report, runtime errors) goes to the
// std::ostream given to the constructor, so the tests can capture it.
// ---------------------------------------------------------------

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <ostream>
#include <string>
#include "../ds/DynamicArray.h"
#include "../ds/Queue.h"
#include "Pipeline.h"

// One line of the interrupt file: "<cycle> <line>".
struct Interrupt {
    int cycle;     // the interrupt arrives when this many instructions have run
    int line;      // interrupt line 0..3
    int order;     // position in the file (so sorting keeps the file order for equal cycles)
    Interrupt() : cycle(0), line(0), order(0) {}
    bool operator<(const Interrupt& other) const {
        if (cycle != other.cycle) {
            return cycle < other.cycle;
        }
        return order < other.order;
    }
};

// One word waiting in the prefetch buffer, together with its address.
struct PrefetchEntry {
    int address;
    int word;
    PrefetchEntry() : address(0), word(0) {}
};

// The states of the simulation.
const int STATE_RUNNING = 0;   // can execute more instructions
const int STATE_HALTED = 1;    // a HALT instruction was executed
const int STATE_ERROR = 2;     // a runtime error stopped the program
const int STATE_LIMIT = 3;     // the instruction limit was reached

class Simulator {
private:
    std::ostream& out;                  // where all text goes

    // the machine 
    DynamicArray<int> memory;           // 256 words, each 0..65535
    DynamicArray<int> registers;        // R0..R7, each 0..65535
    int pc;                             // program counter (0..0xFF, or 0x100 after running off the end)
    int sp;                             // stack pointer
    int zflag;
    int nflag;
    int ie;                             // interrupt enable
    long instructionCount;
    int stackLimit;                     // lowest address the stack may use

    // prefetch buffer 
    Queue<PrefetchEntry> prefetch;      // at most 4 words
    int prefetchNext;                   // address of the next word to put into the buffer
    long prefetchFlushes;

    //  interrupts 
    Queue<Interrupt> incoming;          // from the file, sorted by cycle
    Queue<int> pending;                 // lines that have arrived and wait for IE = 1

    // bookkeeping 
    int state;
    long instructionLimit;              // 0 = no limit
    int currentAddress;                 // address of the instruction that is running now
    bool haltRequested;                 // set by HALT while it executes
    DynamicArray<long> execCount;       // how often each address was executed (profiler)
    DynamicArray<TraceEntry>* trace;    // if not 0: every executed instruction is recorded (pipeline challenge)

    // reading / writing registers and flags
    int readRegister(int r) const;
    void writeRegister(int r, int value);
    void setFlags(int result);

    // the stack in simulated memory
    bool pushWord(int value);           // false on stack overflow
    bool popWord(int& value);           // false on stack underflow

    // prefetch buffer
    void refillPrefetch();
    void flushPrefetch();
    bool prefetchContains(int address) const;
    int fetchWord();

    // one cycle, in pieces
    void handleInterrupts();
    void execute(int word);
    void executeSystem(int word);
    void runtimeError(int address, const std::string& message);
    void printHaltReport();

public:
    explicit Simulator(std::ostream& output);

    // Load a program from the text of a .hex file ("AA: WWWW" per line).
    // Resets the machine. Returns false and sets "error" if the text is bad.
    bool loadHexText(const std::string& text, std::string& error);

    // Load the optional interrupt file ("<cycle> <line>" per line).
    bool loadInterruptText(const std::string& text, std::string& error);

    // 0 = no limit. If the limit is reached the simulation stops with a message.
    void setInstructionLimit(long limit) { instructionLimit = limit; }

    // Record every executed instruction in this array (used for the pipeline challenge).
    // Pass 0 to stop recording. The array is owned by the caller.
    void setTrace(DynamicArray<TraceEntry>* target) { trace = target; }

    // Execute one cycle. Returns the state afterwards.
    int step();

    // Execute until HALT, an error or the limit. Returns the final state.
    int run();

    //  read-only access for the debugger and the profiler 
    int getState() const { return state; }
    bool isFinished() const { return state != STATE_RUNNING; }
    int getPc() const { return pc; }
    int getSp() const { return sp; }
    int getMemoryWord(int address) const { return memory[address]; }
    long getInstructionCount() const { return instructionCount; }
    long getExecCount(int address) const { return execCount[address]; }
    void printRegisters() const;        // the 3-line register dump
};

#endif
