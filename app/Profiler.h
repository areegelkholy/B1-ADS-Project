// Profiler.h
// ---------------------------------------------------------------
// Challenge: report the K most executed instructions.
//
// HOW IT WORKS (uses our MinHeap):
//   Keep a min-heap that holds at most K entries (count, address).
//   For every address that ran at least once, insert it. If the heap
//   then has more than K entries, remove the smallest one (the top).
//   After all 256 addresses, the heap holds exactly the K largest counts.
//   Time: O(n log K) for n addresses, memory: O(K).
//   Then we take the entries out (smallest first) and print them backwards,
//   so the most executed instruction is shown first.
// ---------------------------------------------------------------

#ifndef PROFILER_H
#define PROFILER_H

#include <ostream>
#include "Simulator.h"

void printTopInstructions(const Simulator& sim, int k, std::ostream& out);

#endif
