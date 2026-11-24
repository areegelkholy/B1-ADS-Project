// Profiler.cpp
// Implementation of the top-K profiler (see Profiler.h).

#include "Profiler.h"
#include "Format.h"
#include "Isa.h"
#include "../ds/MinHeap.h"
#include "../ds/DynamicArray.h"

// One candidate: how often the instruction at "address" was executed.
struct ProfileEntry {
    long count;
    int address;
    ProfileEntry() : count(0), address(0) {}

    // "smaller" means executed less often. On a tie the higher address is
    // smaller, so the lower address stays in the heap (a stable, fixed result).
    bool operator<(const ProfileEntry& other) const {
        if (count != other.count) {
            return count < other.count;
        }
        return address > other.address;
    }
};

void printTopInstructions(const Simulator& sim, int k, std::ostream& out) {
    MinHeap<ProfileEntry> heap;

    for (int address = 0; address <= MAX_ADDRESS; address++) {
        long count = sim.getExecCount(address);
        if (count > 0) {
            ProfileEntry entry;
            entry.count = count;
            entry.address = address;
            heap.insert(entry);
            if (heap.size() > k) {
                ProfileEntry smallest;
                heap.extractMin(smallest);     // drop the least executed one
            }
        }
    }

    // The heap gives the smallest first, so collect and print from the back.
    DynamicArray<ProfileEntry> result;
    ProfileEntry entry;
    while (heap.extractMin(entry)) {
        result.pushBack(entry);
    }
    out << "Top " << k << " most executed instructions:" << "\n";
    for (int i = result.size() - 1; i >= 0; i--) {
        int word = sim.getMemoryWord(result[i].address);
        out << hexString(result[i].address, 2) << ": " << hexString(word, 4) << " "
            << disassemble(word) << "   executed " << result[i].count << " times" << "\n";
    }
}
