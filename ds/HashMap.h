// HashMap.h
// ---------------------------------------------------------------
// A hash map from std::string keys to values of any type V
// (our own version of std::unordered_map).
//
// WHY WE NEED IT:
//   - Symbol table:  label    -> address   (HashMap<int>)
//   - Opcode table:  mnemonic -> encoding  (HashMap<OpInfo>)
//   Both need FAST lookup: O(1) on average.
//   - The listing file prints the symbol table sorted by name, so
//     getAllKeys() hands out all keys (then we sort them with Sort.h).
//
// HOW IT WORKS :
//   There is ONE array of "slots" and every slot holds at most one pair.
//   A key has a first choice of slot (startIndex). If that slot is taken
//   by another key (a collision), we do NOT build a list. We jump to
//   another slot a fixed number of steps further, and again, until we
//   find a free one.

//
//   "Double hashing" means the step size comes from a SECOND hash
//   function, so two keys that collide on the first slot usually take
//   different steps afterwards and do not follow each other around the
//   table (this avoids "clustering").
//
//        startIndex = firstHash(key)  % size
//        step       = 1 + secondHash(key) % (size - 1)
//

//
// WHY NOT SEPARATE CHAINING (linked lists) ANY MORE?
//   Our professor pointed out that with chaining the lists can become
//   long, and following pointers through the heap is slow. With one
//   array and a low load factor the keys are found after about 1-2 probes
//   and the memory is contiguous. (Milestone 1 used chaining at 75%.)
//
// LOAD FACTOR 50%:
//   Open addressing slows down a lot when the table gets full. So we
//   rehash (build a table of roughly double the size and re-insert every
//   key) as soon as MORE THAN HALF of the slots are in use.
//
// REMOVING A KEY:
//   If we simply emptied the slot of a removed key, lookups of other keys
//   that were pushed past it by a collision would stop too early and say
//   "not found". So a removed slot is marked DELETED & find() skips over DELETED slots and keeps going, put() may reuse a DELETED slot for a new key.
//
// ---------------------------------------------------------------

#ifndef HASHMAP_H
#define HASHMAP_H

#include <string>  
#include "DynamicArray.h"
#include "Primes.h"

template <typename V>
class HashMap {
private:
    // The three states a slot can be in.
    enum { EMPTY = 0, USED = 1, DELETED = 2 };

    // One slot of the table.
    struct Slot {
        std::string key;
        V value;
        int state;
        Slot() : key(), value(), state(EMPTY) {}
    };

    Slot* slots;       // the table
    int tableSize;     // number of slots, always a prime number
    int count;         // slots in state USED (= number of pairs stored)
    int deletedCount;  // slots in state DELETED (tombstones)

    // First hash function: hash = hash * 31 + character.
    static unsigned long firstHash(const std::string& key) {
        unsigned long hash = 0;
        for (int i = 0; i < (int)key.length(); i++) {
            hash = hash * 31 + (unsigned char)key[i];
        }
        return hash;
    }

    // Second hash function: a different start value and multiplier, so it
    // gives a different number than firstHash for the same key.
    static unsigned long secondHash(const std::string& key) {
        unsigned long hash = 7;
        for (int i = 0; i < (int)key.length(); i++) {
            hash = hash * 37 + (unsigned char)key[i];
        }
        return hash;
    }

    // Where the search for this key starts.
    static int startIndex(const std::string& key, int size) {
        return (int)(firstHash(key) % (unsigned long)size);
    }

    // How far we jump on each collision: a number from 1 to size-1.
    static int stepSize(const std::string& key, int size) {
        return 1 + (int)(secondHash(key) % (unsigned long)(size - 1));
    }

    // Find the slot that holds this key. Returns -1 if the key is not there.
    int findIndex(const std::string& key) const {
        int index = startIndex(key, tableSize);
        int step = stepSize(key, tableSize);
        for (int tries = 0; tries < tableSize; tries++) {
            if (slots[index].state == EMPTY) {
                return -1;                     // chain ends here: key is not in the table
            }
            if (slots[index].state == USED && slots[index].key == key) {
                return index;
            }
            index = (index + step) % tableSize;   // collision or tombstone: next slot
        }
        return -1;
    }

    // First slot on this key's probe sequence that is NOT in use
    // (EMPTY or DELETED). That is where a new key can be stored.
    // Works on any table, so rehash() can use it for the new table too.
    static int freeSlotIn(const Slot* table, int size, const std::string& key) {
        int index = startIndex(key, size);
        int step = stepSize(key, size);
        while (table[index].state == USED) {
            index = (index + step) % size;
        }
        return index;
    }

    // Build a bigger table (next prime >= 2 * size) and move every pair.
    // Tombstones are not copied, so they disappear here. O(size).
    void rehash() {
        int newSize = nextPrime(tableSize * 2);
        Slot* newSlots = new Slot[newSize];
        for (int i = 0; i < tableSize; i++) {
            if (slots[i].state == USED) {
                int index = freeSlotIn(newSlots, newSize, slots[i].key);
                newSlots[index].key = slots[i].key;
                newSlots[index].value = slots[i].value;
                newSlots[index].state = USED;
            }
        }
        delete[] slots;
        slots = newSlots;
        tableSize = newSize;
        deletedCount = 0;
    }

public:
    HashMap() {
        tableSize = 11;       // a small prime, the table grows by itself
        count = 0;
        deletedCount = 0;
        slots = new Slot[tableSize];
    }

    ~HashMap() {
        delete[] slots;
    }

    // Copying is not allowed.
    //
    // WHY: the map owns its slot array, which lives on the heap. If C++
    // made a copy automatically, it would only copy the pointer, so the
    // original and the copy would share the SAME array. Then:
    //   - put() or remove() on one map would silently change the other, and
    //   - when both are destroyed, each destructor would delete[] the same
    //     array, freeing memory twice, which crashes the program.
    // "= delete" makes the compiler refuse any code that tries to copy a
    // HashMap, so the mistake is caught when compiling instead of crashing
    // while the program runs.
    //
    // We lose nothing, because we never need a copy: there is one symbol
    // table and one opcode table, and we pass them around by reference.
    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;

    int size() const { return count; }
    bool isEmpty() const { return count == 0; }

    // Number of slots in the table (used by the tests and the benchmark).
    int capacity() const { return tableSize; }

    // Insert a new pair, or replace the value if the key already exists.
    // Average O(1).
    void put(const std::string& key, const V& value) {
        int existing = findIndex(key);
        if (existing != -1) {
            slots[existing].value = value;     // key already there: just update
            return;
        }
        int index = freeSlotIn(slots, tableSize, key);
        if (slots[index].state == DELETED) {
            deletedCount--;                    // we reuse a tombstone
        }
        slots[index].key = key;
        slots[index].value = value;
        slots[index].state = USED;
        count++;
        if ((count + deletedCount) * 2 > tableSize) {   // more than 50% full
            rehash();
        }
    }

    // Look up a key. If found, copy the value into "result" and return true.
    // Average O(1).
    bool get(const std::string& key, V& result) const {
        int index = findIndex(key);
        if (index == -1) {
            return false;
        }
        result = slots[index].value;
        return true;
    }

    bool contains(const std::string& key) const {
        return findIndex(key) != -1;
    }

    // Remove a key. Returns false if it was not there. Average O(1).
    // The slot becomes a tombstone (DELETED), see the top of this file.
    bool remove(const std::string& key) {
        int index = findIndex(key);
        if (index == -1) {
            return false;
        }
        slots[index].state = DELETED;
        slots[index].key = std::string();     // free the text of the old key
        count--;
        deletedCount++;
        return true;
    }

    // Add every key to the END of "keys". The order is NOT meaningful
    // (it depends on the hash functions), so sort afterwards if needed.
    // O(tableSize): every slot is visited once.
    void getAllKeys(DynamicArray<std::string>& keys) const {
        for (int i = 0; i < tableSize; i++) {
            if (slots[i].state == USED) {
                keys.pushBack(slots[i].key);
            }
        }
    }

    // Delete every pair (the table itself is kept).
    void clear() {
        for (int i = 0; i < tableSize; i++) {
            slots[i].state = EMPTY;
            slots[i].key = std::string();
        }
        count = 0;
        deletedCount = 0;
    }
};

#endif
