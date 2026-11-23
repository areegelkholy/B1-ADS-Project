// Pipeline.cpp
// Implementation of the 5-stage pipeline model .

#include "Pipeline.h"
#include "Isa.h"
#include "Format.h"
#include "../ds/Queue.h"

// Find out which registers an instruction reads and writes.
static void decodeRegisters(PipeInstr& p) {
    int word = p.word;
    int opcode = (word >> 12) & 0xF;
    int rd = (word >> 9) & 7;
    int rs = (word >> 6) & 7;
    int rt = (word >> 3) & 7;

    if (opcode <= 4) {                    // ADD SUB AND OR XOR: rd <- rs, rt
        p.dest = rd; p.src1 = rs; p.src2 = rt;
    } else if (opcode == 0x5) {           // ADDI rd, rs, imm
        p.dest = rd; p.src1 = rs;
    } else if (opcode == 0x6) {           // LDI rd, imm
        p.dest = rd;
    } else if (opcode == 0x7) {           // LW rd, off(rs)
        p.dest = rd; p.src1 = rs; p.isLoad = true;
    } else if (opcode == 0x8) {           // SW rd, off(rs): reads rd (the value) and rs (the address)
        p.src1 = rs; p.src2 = rd;
    } else if (opcode == 0x9) {           // CMP rs, rt
        p.src1 = rs; p.src2 = rt;
    } else if (opcode == 0xD) {           // PUSH rs
        p.src1 = rs;
    } else if (opcode == 0xE) {           // POP rd (reads memory, so it is a load)
        p.dest = rd; p.isLoad = true;
    } else if (opcode == 0xF && (word & 7) == 2) {   // OUT rs
        p.src1 = rs;
    }
    // JMP/Bcc, CALL, RET and the other system instructions use no registers.

    // R0 is always 0: it never makes a hazard
    if (p.dest == 0) p.dest = -1;
    if (p.src1 == 0) p.src1 = -1;
    if (p.src2 == 0) p.src2 = -1;
}

Pipeline::Pipeline(bool useForwarding)
    : forwarding(useForwarding),
      fetchStage(1), decodeStage(1), executeStage(1), memoryStage(1), writeStage(1) {
}

// Does the instruction in ID have to wait? (read-after-write hazard)
// Called while the older instructions are still in EX and MEM.
bool Pipeline::hasHazard(const PipeInstr& waiting) const {
    PipeInstr older;

    if (forwarding) {
        // Results are forwarded. Only a load in EX is too slow (its data
        // appears after MEM), so the next instruction waits one cycle.
        if (executeStage.peek(older) && older.isLoad && older.dest != -1) {
            if (older.dest == waiting.src1 || older.dest == waiting.src2) {
                return true;
            }
        }
        return false;
    }

    // No forwarding: wait while an older instruction in EX or MEM will still write the register.
    // (An instruction in WB writes in the first half of the cycle, so it is no problem.)
    if (executeStage.peek(older) && older.dest != -1) {
        if (older.dest == waiting.src1 || older.dest == waiting.src2) {
            return true;
        }
    }
    if (memoryStage.peek(older) && older.dest != -1) {
        if (older.dest == waiting.src1 || older.dest == waiting.src2) {
            return true;
        }
    }
    return false;
}

// One line of the cycle table: what is in every stage during this cycle.
void Pipeline::printStages(std::ostream& log, long cycle, bool stalled) const {
    const Queue<PipeInstr>* stages[5] = {&fetchStage, &decodeStage, &executeStage, &memoryStage, &writeStage};
    const char* names[5] = {"IF", "ID", "EX", "MEM", "WB"};

    log << "cycle " << cycle << ":";
    for (int i = 0; i < 5; i++) {
        PipeInstr p;
        log << "  " << names[i] << " ";
        if (stages[i]->peek(p)) {
            log << hexString(p.address, 2) << " " << disassemble(p.word);
            if (i == 1 && stalled) {
                log << " (stall)";
            }
        } else {
            log << "--";
        }
        if (i < 4) {
            log << " |";
        }
    }
    log << "\n";
}

PipelineStats Pipeline::run(const DynamicArray<TraceEntry>& trace, std::ostream& log, int logCycles) {
    PipelineStats stats;
    fetchStage.clear();
    decodeStage.clear();
    executeStage.clear();
    memoryStage.clear();
    writeStage.clear();

    int nextToFetch = 0;               // index of the next trace entry to fetch
    bool fetchBlocked = false;         // true after a redirecting instruction was fetched, until it leaves EX
    int lastStalledIndex = -1;         // so we count each waiting instruction only once

    // The loop ends when nothing is left before WB: an instruction that is in
    // WB finishes in that very cycle, so no extra cycle is needed for it.
    while (nextToFetch < trace.size() || !fetchStage.isEmpty() || !decodeStage.isEmpty() ||
           !executeStage.isEmpty() || !memoryStage.isEmpty()) {
        stats.cycles++;
        PipeInstr p;

        // Decide first whether ID must wait (the older instructions have not moved yet).
        bool stall = false;
        PipeInstr waiting;
        if (decodeStage.peek(waiting) && hasHazard(waiting)) {
            stall = true;
            stats.stallCycles++;
            if (waiting.index != lastStalledIndex) {
                stats.hazardInstructions++;
                lastStalledIndex = waiting.index;
            }
        }

        // Now move every instruction one stage to the right, starting at the END
        // so that every stage is empty when the instruction behind it arrives.
        writeStage.dequeue(p);                       // WB: the instruction of the last cycle is gone
        if (memoryStage.dequeue(p)) {                // MEM -> WB
            writeStage.enqueue(p);
            stats.instructions++;                    // it finishes in its WB cycle
        }
        if (executeStage.dequeue(p)) {               // EX -> MEM
            memoryStage.enqueue(p);
            if (p.redirects) {                       // the branch is resolved: fetching may go on
                fetchBlocked = false;
                stats.controlFlushes++;
            }
        }
        if (!stall && decodeStage.dequeue(p)) {      // ID -> EX (otherwise EX gets a bubble)
            executeStage.enqueue(p);
        }
        if (decodeStage.isEmpty() && fetchStage.dequeue(p)) {   // IF -> ID
            decodeStage.enqueue(p);
        }
        if (fetchStage.isEmpty() && !fetchBlocked && nextToFetch < trace.size()) {   // fetch
            PipeInstr fresh;
            fresh.index = nextToFetch;
            fresh.address = trace[nextToFetch].address;
            fresh.word = trace[nextToFetch].word;
            decodeRegisters(fresh);
            // control hazard: the next executed instruction is not the next address
            if (nextToFetch + 1 < trace.size() &&
                trace[nextToFetch + 1].address != fresh.address + 1) {
                fresh.redirects = true;
                fetchBlocked = true;                 // what follows is on the wrong path: bubbles
            }
            fetchStage.enqueue(fresh);
            nextToFetch++;
        }

        if (stats.cycles <= logCycles) {
            printStages(log, stats.cycles, stall);
        }
    }
    return stats;
}

// "1.25" for 125 / 100
static std::string ratioText(long top, long bottom) {
    if (bottom == 0) {
        return "0.00";
    }
    long hundredths = (top * 100 + bottom / 2) / bottom;
    std::string cents = numberToString(hundredths % 100);
    if (cents.length() < 2) {
        cents = "0" + cents;
    }
    return numberToString(hundredths / 100) + "." + cents;
}

void printPipelineReport(const PipelineStats& stats, bool forwarding, std::ostream& out) {
    out << "pipeline (5 stages, forwarding " << (forwarding ? "on" : "off") << "):\n";
    out << "  instructions:    " << stats.instructions << "\n";
    out << "  cycles:          " << stats.cycles << "\n";
    out << "  CPI:             " << ratioText(stats.cycles, stats.instructions)
        << "  (cycles per instruction, 1.00 would be perfect)\n";
    out << "  data hazards:    " << stats.hazardInstructions << " instructions waited, "
        << stats.stallCycles << " stall cycles\n";
    out << "  control hazards: " << stats.controlFlushes << " taken branches/calls/returns, "
        << stats.controlFlushes * 2 << " bubble cycles\n";
}
