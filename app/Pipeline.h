// Pipeline.h
// ---------------------------------------------------------------
// Challenge: simulate a 5-STAGE PIPELINE with a QUEUE PER STAGE and
// detect DATA HAZARDS.
//fetch (IF) then decodes by reading registers (ID) then executes (EX) then accesses memory (MEM) then writes back (WB)
//
// HOW IT FITS THE REST OF THE PROJECT
//   The Simulator already computes the correct RESULT of the program. The
//   pipeline does not change that. It only answers: "how many clock
//   cycles would this program need on a pipelined CPU?"
//   So the Simulator records a TRACE (address + word of every instruction
//   that it executed, in order) and the Pipeline replays the trace, one
//   clock cycle at a time. (Run it with:  sim --pipeline program.hex)
//
// THE MODEL (simple on purpose, every instruction needs 1 cycle per stage)
//   - Every stage is a Queue that holds at most ONE instruction.
//     Each clock cycle, instructions move one stage to the right, if the
//     next stage is free.
//   - Registers are read in ID and written in WB. A register that is
//     written in WB can be read by ID in the SAME cycle (write first half
//     of the cycle, read second half).
//   - DATA HAZARD (read-after-write): the instruction in ID needs a register
//     that an older instruction in EX or MEM has not written yet.
//       Without forwarding: ID waits (a "stall") until the older instruction
//                           reaches WB.
//       With forwarding:    the result is passed straight from the EX/MEM
//                           stage to the next instruction. Only a LOAD
//                           (LW, POP) followed immediately by an instruction
//                           that uses its register still needs one stall.
//   - CONTROL HAZARD: a taken branch, CALL, RET, RETI (or an interrupt
//     entry) is only known in EX. The two instructions fetched after it
//     are on the wrong path, so the fetch stage inserts 2 bubbles
//     (empty cycles) until the target address can be fetched.
//     We detect it in the trace: the next executed address is not
//     address + 1.
//   - R0 is always 0, so it never causes a hazard. Hazards through SP or
//     the flags are ignored (the flags are set in EX and used in EX).
//
// Time: O(cycles). Memory: O(1) (five queues with one entry each).
// ---------------------------------------------------------------

#ifndef PIPELINE_H
#define PIPELINE_H

#include <ostream>
#include "../ds/DynamicArray.h"
#include "../ds/Queue.h"

// One executed instruction, as recorded by the Simulator.
struct TraceEntry {
    int address;
    int word;
    TraceEntry() : address(0), word(0) {}
};

// One instruction while it travels through the pipeline.
struct PipeInstr {
    int index;          // position in the trace (0, 1, 2 ...)
    int address;
    int word;
    int dest;           // register it writes, or -1 (R0 counts as "none")
    int src1;           // registers it reads, or -1
    int src2;
    bool isLoad;        // reads memory (LW, POP): result only available after MEM
    bool redirects;     // the next executed instruction is not at address + 1
    PipeInstr() : index(0), address(0), word(0), dest(-1), src1(-1), src2(-1),
                  isLoad(false), redirects(false) {}
};

// The numbers we report.
struct PipelineStats {
    long instructions;       // instructions that finished
    long cycles;             // clock cycles used
    long hazardInstructions; // instructions that had to wait for a register (data hazards detected)
    long stallCycles;        // cycles lost because of data hazards
    long controlFlushes;     // taken control transfers (each costs 2 bubbles)
    PipelineStats() : instructions(0), cycles(0), hazardInstructions(0), stallCycles(0), controlFlushes(0) {}
};

class Pipeline {
private:
    bool forwarding;

    // one queue per stage; each holds at most 1 instruction
    Queue<PipeInstr> fetchStage;     // IF
    Queue<PipeInstr> decodeStage;    // ID
    Queue<PipeInstr> executeStage;   // EX
    Queue<PipeInstr> memoryStage;    // MEM
    Queue<PipeInstr> writeStage;     // WB

    bool hasHazard(const PipeInstr& waiting) const;
    void printStages(std::ostream& log, long cycle, bool stalled) const;

public:
    explicit Pipeline(bool useForwarding);

    // Replay the trace. If logCycles > 0, the first logCycles cycles are
    // printed as a table (one line per cycle, one column per stage).
    PipelineStats run(const DynamicArray<TraceEntry>& trace, std::ostream& log, int logCycles);
};

// Print the statistics in a readable form.
void printPipelineReport(const PipelineStats& stats, bool forwarding, std::ostream& out);

#endif
