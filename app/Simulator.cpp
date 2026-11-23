// Simulator.cpp
// Implementation of the TISA-16 simulator 

#include "Simulator.h"
#include "Format.h"
#include "Isa.h"
#include "../ds/Sort.h"

const int PREFETCH_SIZE = 4;

Simulator::Simulator(std::ostream& output)
    : out(output), pc(0), sp(STACK_START), zflag(0), nflag(0), ie(0),
      instructionCount(0), stackLimit(0), prefetch(PREFETCH_SIZE),
      prefetchNext(0), prefetchFlushes(0), state(STATE_RUNNING),
      instructionLimit(0), currentAddress(0), haltRequested(false), trace(0) {
    memory.fill(MEMORY_SIZE, 0);
    registers.fill(REGISTER_COUNT, 0);
    execCount.fill(MEMORY_SIZE, 0);
}

// Loading

bool Simulator::loadHexText(const std::string& text, std::string& error) {
    // reset state 
    memory.fill(MEMORY_SIZE, 0);
    registers.fill(REGISTER_COUNT, 0);
    execCount.fill(MEMORY_SIZE, 0);
    pc = 0;
    sp = STACK_START;
    zflag = 0;
    nflag = 0;
    ie = 0;
    instructionCount = 0;
    prefetch.clear();
    prefetchNext = 0;
    prefetchFlushes = 0;
    incoming.clear();
    pending.clear();
    state = STATE_RUNNING;
    haltRequested = false;

    int highest = -1;                    // highest address below 0xFC that the program uses
    DynamicArray<std::string> lines;
    splitLines(text, lines);
    for (int i = 0; i < lines.size(); i++) {
        std::string line = trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        int colon = (int)line.find(':');
        int address = 0;
        int word = 0;
        bool ok = colon != (int)std::string::npos &&
                  parseHexNumber(line.substr(0, colon), address) &&
                  parseHexNumber(line.substr(colon + 1), word) &&
                  address >= 0 && address <= MAX_ADDRESS && word >= 0 && word <= 0xFFFF;
        if (!ok) {
            error = "bad hex file, line " + numberToString(i + 1) + ": '" + line + "'";
            return false;
        }
        memory[address] = word;
        if (address < STACK_START && address > highest) {
            highest = address;
        }
    }
    // The stack may grow down until one address above the program .
    stackLimit = highest + 1;
    return true;
}

bool Simulator::loadInterruptText(const std::string& text, std::string& error) {
    DynamicArray<Interrupt> list;
    DynamicArray<std::string> lines;
    splitLines(text, lines);
    for (int i = 0; i < lines.size(); i++) {
        std::string line = trim(lines[i]);
        if (line.empty()) {
            continue;
        }
        // split "10 0" at the first space or tab
        int split = 0;
        while (split < (int)line.length() && line[split] != ' ' && line[split] != '\t') {
            split++;
        }
        long cycle = 0;
        long lineNumber = 0;
        bool ok = parseDecimalNumber(line.substr(0, split), cycle) &&
                  parseDecimalNumber(line.substr(split), lineNumber) &&
                  lineNumber >= 0 && lineNumber <= 3;
        if (!ok) {
            error = "bad interrupt file, line " + numberToString(i + 1) + ": '" + line + "'";
            return false;
        }
        Interrupt interrupt;
        interrupt.cycle = (int)cycle;
        interrupt.line = (int)lineNumber;
        interrupt.order = i;
        list.pushBack(interrupt);
    }
    // The spec says the file is sorted. We sort anyway (heap sort), so a
    // wrong order cannot block later interrupts.
    heapSort(list);
    incoming.clear();
    for (int i = 0; i < list.size(); i++) {
        incoming.enqueue(list[i]);
    }
    return true;
}


// Registers and flags


int Simulator::readRegister(int r) const {
    if (r == 0) {
        return 0;                          // R0 always reads as 0
    }
    return registers[r];
}

void Simulator::writeRegister(int r, int value) {
    if (r != 0) {                          // writes to R0 are ignored
        registers[r] = value & 0xFFFF;
    }
}

// Z = result is 0, N = bit 15 of the result is 1.
void Simulator::setFlags(int result) {
    zflag = (result == 0) ? 1 : 0;
    nflag = (result >> 15) & 1;
}

// ---------------------------------------------------------------
// The stack (lives in simulated memory)


// push(v): SP <- SP - 1; M[SP] <- v.  Overflow if SP would go below the stack limit.
bool Simulator::pushWord(int value) {
    if (sp - 1 < stackLimit) {
        return false;
    }
    sp = sp - 1;
    memory[sp] = value & 0xFFFF;
    return true;
}

// pop(): v <- M[SP]; SP <- SP + 1.  Underflow if the stack is empty (SP = 0xFC).
bool Simulator::popWord(int& value) {
    if (sp >= STACK_START) {
        return false;
    }
    value = memory[sp];
    sp = sp + 1;
    return true;
}

// ---------------------------------------------------------------
// Prefetch buffer: a Queue of at most 4 upcoming instruction words


// Fill the buffer with the next sequential addresses (never beyond 0xFF).
void Simulator::refillPrefetch() {
    while (!prefetch.isFull() && prefetchNext <= MAX_ADDRESS) {
        PrefetchEntry entry;
        entry.address = prefetchNext;
        entry.word = memory[prefetchNext];
        prefetch.enqueue(entry);
        prefetchNext++;
    }
}

// Empty the buffer. Called AFTER pc has its new value, because refilling
// restarts at pc. Only a buffer that really held words counts as a flush.
void Simulator::flushPrefetch() {
    if (!prefetch.isEmpty()) {
        prefetchFlushes++;
    }
    prefetch.clear();
    prefetchNext = pc;
}

// Is this address waiting in the buffer? (a SW into it makes the word stale)
bool Simulator::prefetchContains(int address) const {
    for (int i = 0; i < prefetch.size(); i++) {
        PrefetchEntry entry;
        prefetch.getAt(i, entry);
        if (entry.address == address) {
            return true;
        }
    }
    return false;
}

// Take the instruction at pc from the head of the buffer and move pc on.
int Simulator::fetchWord() {
    if (prefetch.isEmpty()) {
        prefetchNext = pc;
        refillPrefetch();
    }
    PrefetchEntry entry;
    prefetch.dequeue(entry);
    pc = pc + 1;
    refillPrefetch();                      // top the buffer up again
    return entry.word;
}

// ---------------------------------------------------------------
// One cycle


// Cycle step 1 .
void Simulator::handleInterrupts() {
    // Interrupts whose cycle has come move into the pending queue (FIFO).
    Interrupt next;
    while (incoming.peek(next) && next.cycle <= instructionCount) {
        incoming.dequeue(next);
        pending.enqueue(next.line);
    }

    // With interrupts enabled, serve the oldest pending one.
    if (ie == 1 && !pending.isEmpty()) {
        int line = 0;
        pending.dequeue(line);
        int flagsWord = zflag | (nflag << 1);     // Z in bit 0, N in bit 1
        if (!pushWord(pc) || !pushWord(flagsWord)) {
            runtimeError(pc, "stack overflow");
            return;
        }
        ie = 0;                                   // handlers are not interrupted
        pc = memory[VECTOR_TABLE + line] & 0xFF;  // handler address from the vector table
        flushPrefetch();
    }
}

int Simulator::step() {
    if (state != STATE_RUNNING) {
        return state;
    }
    if (instructionLimit > 0 && instructionCount >= instructionLimit) {
        out << "instruction limit reached after " << instructionCount << " instructions" << "\n";
        printRegisters();
        state = STATE_LIMIT;
        return state;
    }

    currentAddress = pc;
    handleInterrupts();                           // 1. interrupt check
    if (state != STATE_RUNNING) {
        return state;
    }

    currentAddress = pc;                          // the handler may have moved pc
    int word = fetchWord();                       // 2. fetch
    execute(word);                                // 3.+4. decode and execute
    if (state != STATE_RUNNING) {
        return state;                             // a runtime error happened
    }

    instructionCount++;
    execCount[currentAddress]++;
    if (trace != 0) {
        TraceEntry entry;
        entry.address = currentAddress;
        entry.word = word;
        trace->pushBack(entry);
    }
    if (haltRequested) {
        state = STATE_HALTED;
        printHaltReport();
    } else if (pc > MAX_ADDRESS) {
        runtimeError(currentAddress, "PC moved past 0xFF");
    }
    return state;
}

int Simulator::run() {
    while (state == STATE_RUNNING) {
        step();
    }
    return state;
}

// Cut the word into fields and do what the instruction says.
void Simulator::execute(int word) {
    int opcode = (word >> 12) & 0xF;
    int rd = (word >> 9) & 7;
    int rs = (word >> 6) & 7;
    int rt = (word >> 3) & 7;

    if (opcode <= 4) {                            // ADD SUB AND OR XOR
        int a = readRegister(rs);
        int b = readRegister(rt);
        int result = 0;
        if (opcode == 0) result = a + b;
        else if (opcode == 1) result = a - b;
        else if (opcode == 2) result = a & b;
        else if (opcode == 3) result = a | b;
        else result = a ^ b;
        result = result & 0xFFFF;                 // wrap around modulo 2^16
        writeRegister(rd, result);
        setFlags(result);                         // flags are set even if rd is R0
        return;
    }

    switch (opcode) {
        case 0x5: {                               // ADDI rd, rs, imm6
            int result = (readRegister(rs) + signExtend(word, 6)) & 0xFFFF;
            writeRegister(rd, result);
            setFlags(result);
            break;
        }
        case 0x6:                                 // LDI rd, imm9 (flags unchanged)
            writeRegister(rd, signExtend(word, 9) & 0xFFFF);
            break;

        case 0x7:                                 // LW rd, off(rs)
        case 0x8: {                               // SW rd, off(rs)
            int address = (readRegister(rs) + signExtend(word, 6)) & 0xFFFF;
            std::string name = (opcode == 0x7) ? "LW" : "SW";
            if (address > MAX_ADDRESS) {
                runtimeError(currentAddress, name + " address 0x" + hexString(address, 4) +
                             " is outside memory (0x00 to 0xFF)");
                return;
            }
            if (opcode == 0x7) {
                writeRegister(rd, memory[address]);
            } else {
                memory[address] = readRegister(rd);
                if (prefetchContains(address)) {  // the prefetched copy is stale
                    flushPrefetch();
                }
            }
            break;
        }
        case 0x9: {                               // CMP rs, rt
            int a = toSigned16(readRegister(rs));
            int b = toSigned16(readRegister(rt));
            zflag = (a == b) ? 1 : 0;
            nflag = (a < b) ? 1 : 0;
            break;
        }
        case 0xA: {                               // JMP / Bcc: condition in bits 11..9
            int cond = rd;
            bool taken = false;
            if (cond == 0) taken = true;
            else if (cond == 1) taken = (zflag == 1);
            else if (cond == 2) taken = (zflag == 0);
            else if (cond == 3) taken = (nflag == 1);
            else if (cond == 4) taken = (nflag == 0);
            else if (cond == 5) taken = (zflag == 0 && nflag == 0);
            else if (cond == 6) taken = (zflag == 1 || nflag == 1);
            else {
                runtimeError(currentAddress, "invalid branch condition 7");
                return;
            }
            if (taken) {
                pc = word & 0xFF;
                flushPrefetch();
            }
            break;
        }
        case 0xB:                                 // CALL addr8: push the return address
            if (!pushWord(pc)) {                  // pc already points to the next instruction
                runtimeError(currentAddress, "stack overflow");
                return;
            }
            pc = word & 0xFF;
            flushPrefetch();
            break;

        case 0xC: {                               // RET
            int target = 0;
            if (!popWord(target)) {
                runtimeError(currentAddress, "stack underflow");
                return;
            }
            pc = target & 0xFF;
            flushPrefetch();
            break;
        }
        case 0xD:                                 // PUSH rs
            if (!pushWord(readRegister(rs))) {
                runtimeError(currentAddress, "stack overflow");
            }
            break;

        case 0xE: {                               // POP rd
            int value = 0;
            if (!popWord(value)) {
                runtimeError(currentAddress, "stack underflow");
                return;
            }
            writeRegister(rd, value);
            break;
        }
        default:                                  // 0xF: system instructions
            executeSystem(word);
            break;
    }
}

void Simulator::executeSystem(int word) {
    int func = word & 7;
    int rs = (word >> 6) & 7;

    switch (func) {
        case 0:                                   // NOP
            break;
        case 1:                                   // HALT
            haltRequested = true;
            break;
        case 2:                                   // OUT rs: signed decimal on its own line
            out << toSigned16(readRegister(rs)) << "\n";
            break;
        case 3:                                   // EI
            ie = 1;
            break;
        case 4:                                   // DI
            ie = 0;
            break;
        case 5: {                                 // RETI: undo the interrupt entry
            int flagsWord = 0;
            int returnAddress = 0;
            if (!popWord(flagsWord) || !popWord(returnAddress)) {
                runtimeError(currentAddress, "stack underflow");
                return;
            }
            zflag = flagsWord & 1;
            nflag = (flagsWord >> 1) & 1;
            pc = returnAddress & 0xFF;
            ie = 1;
            flushPrefetch();
            break;
        }
        default:
            runtimeError(currentAddress, "invalid system function " + numberToString(func));
            break;
    }
}

// ---------------------------------------------------------------
// Reports


void Simulator::printRegisters() const {
    out << "PC=" << hexString(pc, 2) << " SP=" << hexString(sp, 2)
        << " Z=" << zflag << " N=" << nflag << " IE=" << ie << "\n";
    for (int r = 0; r < REGISTER_COUNT; r++) {
        out << "R" << r << "=" << hexString(readRegister(r), 4);
        if (r == 3 || r == 7) {
            out << "\n";                           // four registers per line
        } else {
            out << " ";
        }
    }
}

void Simulator::printHaltReport() {
    out << "HALT at " << hexString(currentAddress, 2) << " after " << instructionCount
        << " instructions" << "\n";
    printRegisters();
    out << "prefetch flushes: " << prefetchFlushes << "\n";
}

void Simulator::runtimeError(int address, const std::string& message) {
    out << "runtime error at " << hexString(address, 2) << ": " << message << "\n";
    printRegisters();
    state = STATE_ERROR;
}
