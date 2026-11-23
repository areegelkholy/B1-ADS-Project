
// A set of integers with fast add / contains / remove (average O(1)).
// Same idea as HashMap (open addressing + double hashing, rehash when
// more than 50% of the slots are in use), but it stores only ints and
// no values.
//
// WHY WE NEED IT:
//   The debugger keeps the addresses of all breakpoints in a HashSet.
//   Before every instruction the simulator asks "is this address a
//   breakpoint?", so that check must be O(1).
//
// HOW THE TWO HASH FUNCTIONS WORK FOR INTS:
//   Let u be the value as an unsigned number (so negative numbers work).
//        startIndex = u % size
//        step       = 1 + (u / size) % (size - 1)
//   The second function uses the QUOTIENT u / size. Two values that start
//   at the same slot have the same remainder but different quotients, so
//   they take different steps. The size is a prime number, so every step
//   visits every slot

#ifndef HASHSET_H
#define HASHSET_H

class HashSet {
private:
    // The three states a slot can be in.
    enum { EMPTY = 0, USED = 1, DELETED = 2 };

    struct Slot {
        int value;
        int state;
    };

    Slot* slots;       // the table
    int tableSize;     // number of slots, always a prime number
    int count;         // slots in state USED
    int deletedCount;  // slots in state DELETED (tombstones)

    static int startIndex(int value, int size);          // first slot to try
    static int stepSize(int value, int size);            // jump size (1 .. size-1)
    int findIndex(int value) const;                      // slot of value, or -1
    static int freeSlotIn(const Slot* table, int size, int value);
    void rehash();                                       // bigger table, re-insert all

public:
    HashSet();
    ~HashSet();

    // Copying is disabled 
    // WHY: the set owns its slot array, which lives on the heap. If
    // made a copy automatically, it would only copy the "slots" pointer, so
    // the original and the copy would share the SAME array. Then:
    //   - changing one set would silently change the other, and
    //   - when both are destroyed, each destructor would delete[] the same
    //     array, freeing the same memory twice, which crashes the program.
    // "= delete" makes the compiler refuse any code that tries to copy a
    // HashSet, so the mistake is caught when compiling instead of crashing
    // while the program runs.
    //
    // We lose nothing, because we never need a copy (there is only one set
    // of breakpoints).
    HashSet(const HashSet&) = delete;
    HashSet& operator=(const HashSet&) = delete;

    bool add(int value);          // returns false if it was already in the set
    bool contains(int value) const;
    bool remove(int value);       // returns false if it was not in the set
    void clear();
    int size() const;
    int capacity() const;         // number of slots (used by the tests)
    bool isEmpty() const;
};

#endif









