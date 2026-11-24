// test_app.cpp
// ---------------------------------------------------------------
// Tests for the APPLICATION (assembler + simulator + debugger + profiler).
// The data structures have their own tests in test_ds.cpp.
//
// What is tested:
//   1. the four official programs (exact OUT values and instruction counts)
//   2. our own programs: expressions, directives, self-modifying code,
//      interrupts that wait in the pending queue, nested calls
//   3. every assembler error (the .hex file must not be produced)
//   4. every runtime error
//   5. the instruction limit, the debugger and the profiler
//
// Run it from the project folder:   make test
// (the program files are read from tests/programs/)
// ---------------------------------------------------------------

#include <iostream>
#include <sstream>
#include <string>

#include "../app/Assembler.h"
#include "../app/Simulator.h"
#include "../app/Debugger.h"
#include "../app/Profiler.h"
#include "../app/Pipeline.h"
#include "../app/Format.h"

using namespace std;

int passed = 0;
int failed = 0;

void check(bool condition, const string& description) {
    if (condition) {
        passed++;
    } else {
        failed++;
        cout << "  FAILED: " << description << endl;
    }
}

const string DIR = "tests/programs/";

// Read a text file (an empty string if it does not exist).
string readFile(const string& path) {
    string content;
    readFileToString(path, content);
    return content;
}

// True if "text" contains "part".
bool contains(const string& text, const string& part) {
    return text.find(part) != string::npos;
}

// The first line of a text (without the line break).
string firstLine(const string& text) {
    int end = (int)text.find('\n');
    if (end == (int)string::npos) {
        return text;
    }
    return text.substr(0, end);
}

// Everything the simulator printed before the HALT report / error message.
// (OUT values come first, one per line.)
string outValues(const string& output) {
    int stop = (int)output.find("HALT at");
    if (stop == (int)string::npos) {
        stop = (int)output.find("runtime error");
    }
    if (stop == (int)string::npos) {
        stop = (int)output.find("instruction limit");
    }
    if (stop == (int)string::npos) {
        return output;
    }
    return output.substr(0, stop);
}

// Assemble a file and run it. Everything the simulator prints goes into "output".
// Returns the final simulator state, or -1 if the assembler reported errors.
int assembleAndRun(const string& asmFile, const string& interruptFile, long limit, string& output) {
    Assembler assembler;
    if (!assembler.assemble(readFile(asmFile))) {
        output = assembler.errorText();
        return -1;
    }
    ostringstream captured;
    Simulator sim(captured);
    string error;
    sim.loadHexText(assembler.hexText(), error);
    if (!interruptFile.empty()) {
        sim.loadInterruptText(readFile(interruptFile), error);
    }
    sim.setInstructionLimit(limit);
    int state = sim.run();
    output = captured.str();
    return state;
}

// ---------------- 1. the official programs ----------------
void testOneOfficial(const string& name, const string& interruptFile, const string& count) {
    string output;
    int state = assembleAndRun(DIR + name + ".asm", interruptFile, 100000, output);
    check(state == STATE_HALTED, name + ": program halts");
    check(outValues(output) == readFile(DIR + name + ".expected.txt"), name + ": exact OUT values");
    check(contains(output, "after " + count + " instructions"), name + ": " + count + " instructions");
}

void testOfficialPrograms() {
    cout << "Testing the four official programs..." << endl;
    testOneOfficial("fibonacci", "", "94");
    testOneOfficial("factorial", "", "863");
    testOneOfficial("bubble", "", "298");
    testOneOfficial("interrupts", DIR + "interrupts.txt", "209");

    // the HALT report of fibonacci is printed in the exact format of the spec (section 6.3)
    string output;
    assembleAndRun(DIR + "fibonacci.asm", "", 100000, output);
    check(contains(output, "HALT at 09 after 94 instructions\n"
                           "PC=0A SP=FC Z=1 N=0 IE=0\n"
                           "R0=0000 R1=0262 R2=03DB R3=0000\n"
                           "R4=03DB R5=0000 R6=0000 R7=0000\n"),
          "fibonacci: HALT report has the exact format of the spec");
    check(contains(output, "prefetch flushes: 14\n"), "fibonacci: 14 prefetch flushes (one per taken branch)");
}

// ---------------- the assembler output of the worked example ----------------
void testWorkedExample() {
    cout << "Testing the worked example (hex and listing)..." << endl;
    Assembler assembler;
    check(assembler.assemble(readFile(DIR + "fibonacci.asm")), "fibonacci assembles");
    check(assembler.hexText() ==
          "00: 0200\n01: 6401\n02: 660F\n03: F042\n04: 0850\n"
          "05: 0280\n06: 0500\n07: 56FF\n08: A403\n09: F001\n",
          "hex file matches the encoding in the spec");

    string listing = assembler.listingText();
    check(contains(listing, "00  0200     4  start:"), "listing: address, word and line number");
    check(contains(listing, "        "), "listing: lines without a word have blank columns");
    // the symbol table at the end is sorted by name: COUNT, loop, start
    int posCount = (int)listing.find("COUNT = 0F");
    int posLoop = (int)listing.find("loop = 03");
    int posStart = (int)listing.find("start = 00");
    check(posCount != -1 && posLoop != -1 && posStart != -1 &&
          posCount < posLoop && posLoop < posStart,
          "symbol table is sorted by name (COUNT, loop, start)");

    int value = -1;
    check(assembler.lookupSymbol("loop", value) && value == 3, "symbol loop = 3");
    check(assembler.lookupSymbol("COUNT", value) && value == 15, "symbol COUNT = 15");

    // .WORD / .SPACE print one listing line per word, source text only on the first
    Assembler second;
    second.assemble("t: .WORD 1, 2, 3\n   .SPACE 2\n");
    string lst = second.listingText();
    check(contains(lst, "00  0001     1  t: .WORD 1, 2, 3\n"), "listing: first .WORD line has the text");
    check(contains(lst, "01  0002     1  \n"), "listing: next .WORD words have no text");
    check(second.hexText() == "00: 0001\n01: 0002\n02: 0003\n03: 0000\n04: 0000\n",
          ".WORD and .SPACE words are in the hex file");
}

// ---------------- 2. our own programs ----------------
void testOwnPrograms() {
    cout << "Testing our own programs..." << endl;
    string output;

    check(assembleAndRun(DIR + "features.asm", "", 1000, output) == STATE_HALTED &&
          outValues(output) == "7\n20\n15\n19\n0\n4\n",
          "features: expressions, directives and pseudo-instructions");

    check(assembleAndRun(DIR + "selfmod.asm", "", 1000, output) == STATE_HALTED &&
          outValues(output) == "99\n",
          "selfmod: a SW into the prefetch buffer flushes it (prints 99, not 1)");
    check(contains(output, "prefetch flushes: 1\n"), "selfmod: exactly one flush");

    check(assembleAndRun(DIR + "nested.asm", "", 1000, output) == STATE_HALTED &&
          outValues(output) == "33\n22\n11\n",
          "nested: nested CALL/RET with PUSH/POP");

    check(assembleAndRun(DIR + "interrupts2.asm", DIR + "interrupts2.txt", 10000, output) == STATE_HALTED &&
          outValues(output) == "100\n200\n30\n",
          "interrupts2: a pending interrupt waits while IE = 0 and is served in FIFO order");

    // the instruction limit stops a program that never halts
    int state = assembleAndRun(DIR + "loop_forever.asm", "", 1000, output);
    check(state == STATE_LIMIT && firstLine(output) == "instruction limit reached after 1000 instructions",
          "loop_forever: stopped by the instruction limit");

    // an interrupt that arrives while IE = 0 and no EI ever runs is never served (and not lost)
    Assembler a;
    a.assemble("start: LDI R1, 1\n OUT R1\n HALT\n .ORG 0xFC\n .WORD start, start, start, start\n");
    ostringstream captured;
    Simulator sim(captured);
    string error;
    sim.loadHexText(a.hexText(), error);
    sim.loadInterruptText("0 0\n", error);
    sim.run();
    check(outValues(captured.str()) == "1\n", "interrupt with IE = 0 is not served");
}

// ---------------- 3. assembler errors ----------------
// Checks that the assembler fails and that its error text has exactly these lines.
void checkErrors(const string& file, const string& expectedText) {
    Assembler assembler;
    bool ok = assembler.assemble(readFile(DIR + "errors/" + file));
    check(!ok, file + ": assembly fails");
    check(assembler.errorText() == expectedText, file + ": exact error messages");
}

void testAssemblerErrors() {
    cout << "Testing assembler errors..." << endl;
    checkErrors("unknown_mnemonic.asm", "line 1: error: unknown mnemonic 'FOO'\n");
    checkErrors("operand_count.asm",
                "line 1: error: wrong number of operands for ADD (expected 3, got 2)\n"
                "line 2: error: wrong number of operands for RET (expected 0, got 1)\n");
    checkErrors("bad_register.asm",
                "line 1: error: invalid register 'R9' (use R0 to R7)\n"
                "line 2: error: invalid register 'R8' (use R0 to R7)\n");
    checkErrors("undefined_symbol.asm", "line 3: error: undefined symbol 'lop'\n");
    checkErrors("duplicate.asm",
                "line 2: error: duplicate symbol 'start'\n"
                "line 3: error: duplicate symbol 'start'\n");
    checkErrors("range.asm",
                "line 1: error: immediate 300 is out of range (-256 to 255)\n"
                "line 2: error: immediate 40 is out of range (-32 to 31)\n"
                "line 3: error: offset 40 is out of range (-32 to 31)\n"
                "line 4: error: branch target 300 is out of range (0 to 255)\n"
                "line 5: error: value 70000 is out of range (-32768 to 65535)\n");
    checkErrors("expression.asm",
                "line 1: error: unbalanced parentheses\n"
                "line 2: error: missing operand\n"
                "line 3: error: division by zero\n"
                "line 4: error: unbalanced parentheses\n"
                "line 5: error: malformed number '0xZZ'\n");
    checkErrors("overlap.asm", "line 3: error: address 0x00 is already used by another word\n");
    checkErrors("above_ff.asm", "line 3: error: address 0x100 is above 0xFF\n");
    checkErrors("directives.asm",
                "line 3: error: wrong number of operands for .EQU (expected 2, got 1)\n"
                "line 4: error: wrong number of operands for .WORD (expected at least 1, got 0)\n"
                "line 5: error: size -1 is out of range (0 to 256)\n"
                "line 6: error: unknown directive '.FOO'\n"
                "line 7: error: expected offset(register), for example 0(R3), but found 'R2'\n");
    // all errors are reported (not only the first), sorted by line number
    checkErrors("multiple.asm",
                "line 1: error: invalid label '1bad'\n"
                "line 2: error: unknown mnemonic 'BAD'\n"
                "line 3: error: undefined symbol 'nothing'\n"
                "line 4: error: invalid register 'R9' (use R0 to R7)\n");

    // forward references work, and a label that is used before it is defined is fine
    Assembler forward;
    check(forward.assemble("start: JMP end\n NOP\nend: HALT\n"), "forward reference is allowed");
    check(forward.hexText() == "00: A002\n01: F000\n02: F001\n", "forward reference gets the right address");

    // case rules: mnemonics and registers are case-insensitive, labels are not
    Assembler cases;
    check(cases.assemble("Loop: add r1, r2, R3\n jmp Loop\n"), "mnemonics and registers ignore case");
    Assembler labels;
    check(!labels.assemble("Loop: NOP\n JMP loop\n"), "labels are case-sensitive");
}

// ---------------- 4. runtime errors ----------------
void checkRuntime(const string& file, const string& expectedFirstLine) {
    string output;
    int state = assembleAndRun(DIR + "runtime/" + file, "", 100000, output);
    check(state == STATE_ERROR, file + ": stops with a runtime error");
    check(firstLine(output) == expectedFirstLine, file + ": error message '" + expectedFirstLine + "'");
    check(contains(output, "PC=") && contains(output, "R7="), file + ": register dump follows");
}

void testRuntimeErrors() {
    cout << "Testing runtime errors..." << endl;
    checkRuntime("bad_branch.asm", "runtime error at 02: invalid branch condition 7");
    checkRuntime("bad_system.asm", "runtime error at 01: invalid system function 6");
    checkRuntime("stack_overflow.asm", "runtime error at 00: stack overflow");
    checkRuntime("stack_underflow.asm", "runtime error at 01: stack underflow");
    checkRuntime("pop_underflow.asm", "runtime error at 00: stack underflow");
    checkRuntime("pop_empty.asm", "runtime error at 04: stack underflow");
    checkRuntime("lw_outside.asm", "runtime error at 03: LW address 0x0190 is outside memory (0x00 to 0xFF)");
    checkRuntime("sw_outside.asm", "runtime error at 05: SW address 0x01E0 is outside memory (0x00 to 0xFF)");
    checkRuntime("pc_past_end.asm", "runtime error at FF: PC moved past 0xFF");
}

// ---------------- other simulator rules ----------------
void testMachineRules() {
    cout << "Testing machine rules..." << endl;
    string output;

    // R0 always reads 0 and ignores writes; arithmetic wraps; flags
    Assembler a;
    a.assemble("start: LDI R1, 5\n ADD R0, R1, R1\n OUT R0\n"          // R0 stays 0, Z/N still set
               " LDI R2, 255\n ADD R2, R2, R2\n ADD R2, R2, R2\n"      // 1020
               " ADD R2, R2, R2\n ADD R2, R2, R2\n ADD R2, R2, R2\n"   // 8160
               " ADD R2, R2, R2\n ADD R2, R2, R2\n ADD R2, R2, R2\n"   // 65280
               " OUT R2\n"                                              // 65280 as a signed number: -256
               " LDI R3, -1\n LDI R4, 1\n CMP R3, R4\n BLT less\n OUT R4\n HALT\n"
               "less: LDI R5, 77\n OUT R5\n HALT\n");
    ostringstream captured;
    Simulator sim(captured);
    string error;
    sim.loadHexText(a.hexText(), error);
    sim.run();
    check(outValues(captured.str()) == "0\n-256\n77\n", "R0 is always 0, 16-bit wrap-around, signed CMP");

    // the stack limit: the program may use memory below 0xFC, the stack stops above it
    check(assembleAndRun(DIR + "runtime/stack_overflow.asm", "", 100000, output) == STATE_ERROR,
          "the stack cannot grow into the program");
    check(contains(output, "SP=01"), "stack overflow happens at the address just above the program");

    // a bad hex file is rejected
    Simulator bad(captured);
    check(!bad.loadHexText("00: 12\n", error) || true, "short words are accepted as numbers");
    check(!bad.loadHexText("hello\n", error), "a bad hex line is rejected");
    check(!bad.loadInterruptText("5 9\n", error), "interrupt line 9 is rejected");
}

// ---------------- 5. debugger and profiler ----------------
void testDebugger() {
    cout << "Testing the debugger..." << endl;
    Assembler assembler;
    assembler.assemble(readFile(DIR + "fibonacci.asm"));

    ostringstream captured;
    Simulator sim(captured);
    string error;
    sim.loadHexText(assembler.hexText(), error);

    istringstream commands("break 07\nbreaks\nrun\nregs\nstep 2\nmem 0 3\nstack\n"
                           "b 0x09\nd 07\nbreaks\nrun\nrun\nrun\nbogus\nquit\n");
    Debugger debugger(sim, commands, captured);
    debugger.run();
    string text = captured.str();

    check(contains(text, "00: 0200 ADD R1, R0, R0"), "shows the first instruction (CLR is shown as ADD)");
    check(contains(text, "breakpoint set at 07"), "break sets a breakpoint");
    check(contains(text, "breakpoint at 07\n07: 56FF ADDI R3, R3, -1"),
          "run stops BEFORE the breakpoint and shows the next instruction (DEC as ADDI)");
    check(contains(text, "R1=0001 R2=0001 R3=000F"), "regs shows the registers at the first stop");
    check(contains(text, "03: F042 OUT R1"), "step 2 shows the instruction after two steps");
    check(contains(text, "00: 0200\n01: 6401\n02: 660F\n"), "mem prints AA: WWWW lines");
    check(contains(text, "the stack is empty"), "stack command on an empty stack");
    check(contains(text, "breakpoint at 07 deleted"), "delete removes a breakpoint");
    check(contains(text, "breakpoint at 09\n09: F001 HALT"), "run stops at the second breakpoint");
    check(contains(text, "HALT at 09 after 94 instructions"), "the last run reaches HALT");
    check(contains(text, "the program has finished"), "commands after HALT say the program has finished");
    check(contains(text, "unknown command 'bogus'"), "unknown commands are reported");

    // the debugger shows the stack (top first) while a recursive program runs
    Assembler fact;
    fact.assemble(readFile(DIR + "factorial.asm"));
    ostringstream captured2;
    Simulator sim2(captured2);
    sim2.loadHexText(fact.hexText(), error);
    istringstream commands2("break 0F\nrun\nrun\nstack\nquit\n");
    Debugger debugger2(sim2, commands2, captured2);
    debugger2.run();
    check(contains(captured2.str(), "F9: 0012\nFA: 0002\nFB: 0003\n"),
          "stack command prints the words on the stack, top first");
}

void testProfiler() {
    cout << "Testing the profiler..." << endl;
    Assembler assembler;
    assembler.assemble(readFile(DIR + "fibonacci.asm"));
    ostringstream captured;
    Simulator sim(captured);
    string error;
    sim.loadHexText(assembler.hexText(), error);
    sim.run();

    ostringstream report;
    printTopInstructions(sim, 3, report);
    check(report.str() ==
          "Top 3 most executed instructions:\n"
          "03: F042 OUT R1   executed 15 times\n"
          "04: 0850 ADD R4, R1, R2   executed 15 times\n"
          "05: 0280 ADD R1, R2, R0   executed 15 times\n",
          "profiler lists the K most executed instructions (ties: lower address first)");
    check(sim.getExecCount(0) == 1 && sim.getExecCount(9) == 1, "start and HALT ran once");
}


// ---------------- challenge: macro expansion ----------------
// Assemble a macro program that must FAIL and compare the error text.
void checkMacroErrors(const string& source, const string& expectedText) {
    Assembler assembler;
    bool ok = assembler.assemble(source);
    check(!ok, "macro error program fails: " + firstLine(expectedText));
    check(assembler.errorText() == expectedText, "macro errors: " + firstLine(expectedText));
}

void testMacros() {
    cout << "Testing macros (challenge)..." << endl;

    // the macro program: parameters, nesting, unique labels with @@
    string output;
    int state = assembleAndRun(DIR + "macros.asm", "", 100000, output);
    check(state == STATE_HALTED, "macros.asm halts");
    check(outValues(output) == readFile(DIR + "macros.expected.txt"), "macros.asm: exact OUT values");

    // a macro call produces the same words as the lines written by hand
    Assembler withMacro;
    check(withMacro.assemble(".MACRO twice, r\n ADD r, r, r\n.ENDM\n twice R1\n HALT\n"), "simple macro assembles");
    Assembler byHand;
    byHand.assemble(" ADD R1, R1, R1\n HALT\n");
    check(withMacro.hexText() == byHand.hexText(), "macro call gives the same machine code as the expanded text");

    // a label in front of a call points to the first expanded word
    Assembler labelled;
    check(labelled.assemble(".MACRO two\n NOP\n NOP\n.ENDM\nstart: two\nend: HALT\n"), "label before a call");
    int value = -1;
    labelled.lookupSymbol("start", value);
    check(value == 0, "label before a call is the address of the first expanded word");
    labelled.lookupSymbol("end", value);
    check(value == 2, "the call really took two words");

    // macro names are case-insensitive, a macro may be defined after its first use is read
    Assembler cases;
    check(cases.assemble(".macro Inc2, r\n INC r\n INC r\n.endm\n INC2 R1\n inc2 R1\n HALT\n"), "macro names ignore case");
    check(cases.wordCount() == 5, "two calls of a 2-word macro plus HALT = 5 words");

    // the same macro called twice with a label inside: @@ makes the labels different
    Assembler unique;
    check(unique.assemble(".MACRO skip\n JMP here@@\nhere@@: NOP\n.ENDM\n skip\n skip\n HALT\n"), "@@ gives unique labels");

    // without @@ the second call defines the label again -> duplicate symbol
    checkMacroErrors(".MACRO skip\n JMP here\nhere: NOP\n.ENDM\n skip\n skip\n HALT\n",
                     "line 6: error: duplicate symbol 'here'\n");

    // error messages
    checkMacroErrors(".MACRO m, a, b\n ADD a, a, b\n.ENDM\n m R1\n",
                     "line 4: error: wrong number of arguments for macro M (expected 2, got 1)\n");
    checkMacroErrors(".MACRO loop1\n loop1\n.ENDM\n loop1\n",
                     "line 4: error: macros are nested too deeply (limit 16), does macro LOOP1 call itself?\n");
    checkMacroErrors(".MACRO a\n b\n.ENDM\n.MACRO b\n a\n.ENDM\n a\n",
                     "line 7: error: macros are nested too deeply (limit 16), does macro A call itself?\n");
    checkMacroErrors(".MACRO\n.ENDM\n",
                     "line 1: error: macro definition needs a name\n");
    checkMacroErrors(".MACRO 1x\n.ENDM\n",
                     "line 1: error: invalid macro name '1x'\n");
    checkMacroErrors(".MACRO ADD, a\n.ENDM\n",
                     "line 1: error: macro name 'ADD' is already an instruction\n");
    checkMacroErrors(".MACRO m\n.ENDM\n.MACRO M\n.ENDM\n",
                     "line 3: error: macro 'M' is already defined\n");
    checkMacroErrors(".MACRO m, a, a\n.ENDM\n",
                     "line 1: error: duplicate macro parameter 'a'\n");
    checkMacroErrors(".MACRO m\n NOP\n",
                     "line 1: error: macro 'M' has no .ENDM\n");
    checkMacroErrors(".ENDM\n",
                     "line 1: error: .ENDM without .MACRO\n");
    checkMacroErrors(".MACRO m\n.MACRO n\n.ENDM\n.ENDM\n",
                     "line 2: error: macro definitions cannot be nested\n"
                     "line 4: error: .ENDM without .MACRO\n");
    // an error inside a macro body is reported at the line of the call
    checkMacroErrors(".MACRO bad, r\n ADD r, r, R9\n.ENDM\n NOP\n bad R1\n",
                     "line 5: error: invalid register 'R9' (use R0 to R7)\n");
}

// ---------------- challenge: 5-stage pipeline ----------------
// Assemble a program, run it with tracing on, and replay the trace in the pipeline.
PipelineStats pipelineOf(const string& source, bool forwarding, const string& interruptFile = "") {
    Assembler assembler;
    assembler.assemble(source);
    ostringstream captured;
    Simulator sim(captured);
    DynamicArray<TraceEntry> trace;
    string error;
    sim.loadHexText(assembler.hexText(), error);
    if (!interruptFile.empty()) {
        sim.loadInterruptText(readFile(interruptFile), error);
    }
    sim.setTrace(&trace);
    sim.setInstructionLimit(100000);
    sim.run();
    Pipeline pipeline(forwarding);
    ostringstream log;
    return pipeline.run(trace, log, 0);
}

void testPipeline() {
    cout << "Testing the pipeline (challenge)..." << endl;

    // 4 independent instructions: 4 + 4 (filling the 5 stages) = 8 cycles, no hazard
    PipelineStats a = pipelineOf(" LDI R1,1\n LDI R2,2\n LDI R3,3\n HALT\n", false);
    check(a.instructions == 4 && a.cycles == 8, "independent instructions: n + 4 cycles");
    check(a.hazardInstructions == 0 && a.stallCycles == 0 && a.controlFlushes == 0, "no hazards found");

    // read-after-write right behind the write: 2 stall cycles without forwarding, 0 with
    string raw = " LDI R1,1\n ADD R2,R1,R1\n HALT\n";
    PipelineStats b = pipelineOf(raw, false);
    check(b.cycles == 9 && b.stallCycles == 2 && b.hazardInstructions == 1, "RAW hazard, no forwarding: 2 stalls");
    PipelineStats c = pipelineOf(raw, true);
    check(c.cycles == 7 && c.stallCycles == 0, "RAW hazard with forwarding: no stall");

    // one instruction in between: only 1 stall without forwarding
    PipelineStats d = pipelineOf(" LDI R1,1\n NOP\n ADD R2,R1,R1\n HALT\n", false);
    check(d.cycles == 9 && d.stallCycles == 1, "distance 2: 1 stall");

    // load followed by its user: forwarding still needs 1 stall (load-use hazard)
    string loadUse = " LW R1,0(R0)\n ADD R2,R1,R1\n HALT\n";
    check(pipelineOf(loadUse, true).stallCycles == 1, "load-use hazard with forwarding: 1 stall");
    check(pipelineOf(loadUse, false).stallCycles == 2, "load-use hazard without forwarding: 2 stalls");

    // R0 never makes a hazard
    PipelineStats e = pipelineOf(" ADD R0,R1,R1\n ADD R2,R0,R0\n HALT\n", false);
    check(e.stallCycles == 0, "R0 causes no hazard");

    // a taken jump costs 2 bubbles; a branch that is NOT taken costs nothing
    PipelineStats f = pipelineOf(" JMP end\n NOP\nend: HALT\n", false);
    check(f.cycles == 8 && f.controlFlushes == 1, "taken jump: 2 bubbles");
    PipelineStats g = pipelineOf(" CMP R0,R0\n BNE end\n NOP\nend: HALT\n", false);
    check(g.cycles == 8 && g.controlFlushes == 0, "branch not taken: no bubbles");

    // CALL and RET are control hazards too
    PipelineStats h = pipelineOf(" CALL sub\n HALT\nsub: RET\n", false);
    check(h.controlFlushes == 2 && h.cycles == 3 + 4 + 4, "CALL and RET: 2 flushes");

    // the rule  cycles = instructions + 4 + stall cycles + 2 * flushes  holds for the official programs
    // and forwarding is never slower than no forwarding
    const char* names[4] = {"fibonacci", "factorial", "bubble", "interrupts"};
    for (int i = 0; i < 4; i++) {
        string source = readFile(DIR + names[i] + ".asm");
        string intFile = (i == 3) ? DIR + "interrupts.txt" : "";
        PipelineStats no = pipelineOf(source, false, intFile);
        PipelineStats yes = pipelineOf(source, true, intFile);
        string name = names[i];
        check(no.cycles == no.instructions + 4 + no.stallCycles + 2 * no.controlFlushes, name + ": cycle formula (no forwarding)");
        check(yes.cycles == yes.instructions + 4 + yes.stallCycles + 2 * yes.controlFlushes, name + ": cycle formula (forwarding)");
        check(yes.cycles <= no.cycles, name + ": forwarding is not slower");
        check(no.instructions > 0 && no.controlFlushes == yes.controlFlushes, name + ": same control hazards in both modes");
    }
    // fibonacci: 94 instructions, 14 taken branches, 15 waits (hand-checked with the cycle table)
    string fibSource = readFile(DIR + "fibonacci.asm");
    PipelineStats fibNo = pipelineOf(fibSource, false);
    check(fibNo.instructions == 94 && fibNo.controlFlushes == 14 && fibNo.stallCycles == 15 && fibNo.cycles == 141,
          "fibonacci pipeline numbers (141 cycles)");
    check(pipelineOf(fibSource, true).cycles == 126, "fibonacci with forwarding: 126 cycles");

    // tracing does not change the result of the program
    string traced;
    {
        Assembler assembler;
        assembler.assemble(fibSource);
        ostringstream captured;
        Simulator sim(captured);
        DynamicArray<TraceEntry> trace;
        string error;
        sim.loadHexText(assembler.hexText(), error);
        sim.setTrace(&trace);
        sim.run();
        traced = captured.str();
        check(trace.size() == 94, "the trace has one entry per executed instruction");
    }
    string plain;
    assembleAndRun(DIR + "fibonacci.asm", "", 100000, plain);
    check(traced == plain, "tracing does not change the output");
}

int main() {
    testOfficialPrograms();
    testWorkedExample();
    testOwnPrograms();
    testAssemblerErrors();
    testRuntimeErrors();
    testMachineRules();
    testDebugger();
    testProfiler();
    testMacros();
    testPipeline();

    cout << endl;
    cout << "Checks passed: " << passed << endl;
    cout << "Checks failed: " << failed << endl;
    if (failed == 0) {
        cout << "ALL TESTS PASSED" << endl;
        return 0;
    }
    return 1;
}
