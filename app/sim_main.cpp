// sim_main.cpp
// The simulator program:
//   sim program.hex [interrupts.txt]        run to HALT, printing OUT values
//   sim -d program.hex [interrupts.txt]     same, under the debugger
// Extra options:
//   -l N    stop after N instructions (default 1000000, 0 = no limit)
//   -p K    after the run, print the K most executed instructions (profiler)
//   --pipeline        after the run, report how many cycles a 5-stage pipeline needs
//   --forwarding      same, but the pipeline forwards results (fewer stalls)
//   --pipeline-log N  also print the stages of the first N cycles as a table

#include <iostream>
#include "Simulator.h"
#include "Debugger.h"
#include "Profiler.h"
#include "Pipeline.h"
#include "Format.h"

static void printUsage() {
    std::cout << "usage: sim [-d] [-l N] [-p K] [--pipeline] [--forwarding] [--pipeline-log N] program.hex [interrupts.txt]" << std::endl;
}

int main(int argc, char** argv) {
    bool useDebugger = false;
    long limit = 1000000;
    long profileTop = 0;
    bool usePipeline = false;
    bool forwarding = false;
    long pipelineLog = 0;
    std::string hexPath;
    std::string interruptPath;

    // read the command line
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-d") {
            useDebugger = true;
        } else if (arg == "--pipeline") {
            usePipeline = true;
        } else if (arg == "--forwarding") {
            usePipeline = true;
            forwarding = true;
        } else if (arg == "-l" || arg == "-p" || arg == "--pipeline-log") {
            long number = 0;
            if (i + 1 >= argc || !parseDecimalNumber(argv[i + 1], number)) {
                printUsage();
                return 2;
            }
            if (arg == "-l") limit = number;
            else if (arg == "-p") profileTop = number;
            else {
                pipelineLog = number;
                usePipeline = true;
            }
            i++;                                  // we used the next argument too
        } else if (hexPath.empty()) {
            hexPath = arg;
        } else if (interruptPath.empty()) {
            interruptPath = arg;
        } else {
            printUsage();
            return 2;
        }
    }
    if (hexPath.empty()) {
        printUsage();
        return 2;
    }

    // load the program (and the interrupts)
    Simulator sim(std::cout);
    std::string text;
    std::string error;
    if (!readFileToString(hexPath, text)) {
        std::cout << "cannot open '" << hexPath << "'" << std::endl;
        return 2;
    }
    if (!sim.loadHexText(text, error)) {
        std::cout << error << std::endl;
        return 2;
    }
    if (!interruptPath.empty()) {
        if (!readFileToString(interruptPath, text)) {
            std::cout << "cannot open '" << interruptPath << "'" << std::endl;
            return 2;
        }
        if (!sim.loadInterruptText(text, error)) {
            std::cout << error << std::endl;
            return 2;
        }
    }
    sim.setInstructionLimit(limit);

    // for the pipeline model the simulator records every instruction it executes
    DynamicArray<TraceEntry> trace;
    if (usePipeline) {
        sim.setTrace(&trace);
    }

    // run
    if (useDebugger) {
        Debugger debugger(sim, std::cin, std::cout);
        debugger.run();
    } else {
        sim.run();
    }

    if (profileTop > 0) {
        printTopInstructions(sim, (int)profileTop, std::cout);
    }
    if (usePipeline) {
        Pipeline pipeline(forwarding);
        PipelineStats stats = pipeline.run(trace, std::cout, (int)pipelineLog);
        printPipelineReport(stats, forwarding, std::cout);
    }
    return sim.getState() == STATE_HALTED ? 0 : (useDebugger ? 0 : 1);
}
