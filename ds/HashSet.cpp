#include "HashSet.h"
#include "Primes.h"

// First hash function: the remainder after dividing by the table size.
// We convert to unsigned first, because % of a negative int is negative in C++
int HashSet::startIndex(int value, int size) {
    unsigned int u = (unsigned int)value;
    return (int)(u % (unsigned int)size);
}

// Second hash function: uses the quotient, so values that start at the
// same slot get different step sizes. The result is from 1 to size-1.
int HashSet::stepSize(int value, int size) {
    unsigned int u = (unsigned int)value;
    return 1 + (int)((u / (unsigned int)size) % (unsigned int)(size - 1));
}

HashSet::HashSet() {
    tableSize = 11;          // a small prime, the table grows by itself
    count = 0;
    deletedCount = 0;
    slots = new Slot[tableSize];
    for (int i = 0; i < tableSize; i++) {
        slots[i].value = 0;
        slots[i].state = EMPTY;
    }
}

HashSet::~HashSet() {
    delete[] slots;
}

// Follow the probe sequence of "value" until we find it or reach an empty slot
int HashSet::findIndex(int value) const {
    int index = startIndex(value, tableSize);
    int step = stepSize(value, tableSize);
    for (int tries = 0; tries < tableSize; tries++) {
        if (slots[index].state == EMPTY) {
            return -1;                       // the chain ends(not in the set)
        }
        if (slots[index].state == USED && slots[index].value == value) {
            return index;
        }
        index = (index + step) % tableSize;  // taken or a tombstone: next slot
    }
    return -1;
}

// First slot of the probe sequence that is not ued, works on any table,
// so rehash() can use it for the new table too
int HashSet::freeSlotIn(const Slot* table, int size, int value) {
    int index = startIndex(value, size);
    int step = stepSize(value, size);
    while (table[index].state == USED) {
        index = (index + step) % size;
    }
    return index;
}

bool HashSet::contains(int value) const {
    return findIndex(value) != -1;
}

bool HashSet::add(int value) {
    if (contains(value)) {
        return false;                 // sets never hold duplicates
    }
    int index = freeSlotIn(slots, tableSize, value);
    if (slots[index].state == DELETED) {
        deletedCount--;               // we reuse a tombstone
    }
    slots[index].value = value;
    slots[index].state = USED;
    count++;
    if ((count + deletedCount) * 2 > tableSize) {   // more than 50% full -> grow
        rehash();
    }
    return true;
}

bool HashSet::remove(int value) {
    int index = findIndex(value);
    if (index == -1) {
        return false;
    }
    slots[index].state = DELETED;     // tombstone
    count--;
    deletedCount++;
    return true;
}

void HashSet::rehash() {
    int newSize = nextPrime(tableSize * 2);
    Slot* newSlots = new Slot[newSize];
    for (int i = 0; i < newSize; i++) {
        newSlots[i].value = 0;
        newSlots[i].state = EMPTY;
    }
    // Move every used value into its place in the bigger table
    for (int i = 0; i < tableSize; i++) {
        if (slots[i].state == USED) {
            int index = freeSlotIn(newSlots, newSize, slots[i].value);
            newSlots[index].value = slots[i].value;
            newSlots[index].state = USED;
        }
    }
    delete[] slots;
    slots = newSlots;
    tableSize = newSize;
    deletedCount = 0;
}

void HashSet::clear() {
    for (int i = 0; i < tableSize; i++) {
        slots[i].state = EMPTY;
    }
    count = 0;
    deletedCount = 0;
}

int HashSet::size() const {
    return count;
}

int HashSet::capacity() const {
    return tableSize;
}

bool HashSet::isEmpty() const {
    return count == 0;
}
