// HashMap.h
// ---------------------------------------------------------------
// A hash map from std::string keys to values of any type V
// (our own version of std::unordered_map).
//
// WHY WE NEED IT:
//   - Symbol table:  label  -> address   (HashMap<int>)
//   - Opcode table:  mnemonic -> encoding (HashMap<int> or a struct)
//   Both need FAST lookup: O(1) on average.
//   - The listing file prints the symbol table sorted by name, so
//     getAllKeys() hands out all keys (then we sort them with Sort.h).
//
// HOW IT WORKS (separate chaining):
//   We keep an array of "buckets". A hash function turns a key into a
//   number, and (number % bucketCount) tells us which bucket to use.
//   Each bucket is a linked list, so if two keys land in the same bucket
//   (a collision) they just sit next to each other in that list.
//
// REHASHING:
//   If the lists get long, lookups get slow. So when
//   count > 75% of bucketCount (load factor 0.75) we double the number of
//   buckets and move every node to its new bucket. This keeps the chains
//   short, so the average lookup stays O(1).
//
// DESIGN DECISIONS:
//   - Keys are always std::string (both of our tables use text keys), which
//     keeps the code simple compared to a fully generic hash map.
//   - get() returns true/false and writes the value into a parameter, so
//     the assembler can report "undefined label" itself.
//  - Copying is not allowed
    // WHY: the map owns its linked nodes, which live on the heap, and the
    // "buckets" array that points to them. If C++ made a copy automatically,
    // it would only copy the pointers, so the original and the copy would
    // share the SAME nodes. 
    //   - put() or remove() on one map would silently change the other, and
    //   - when both are destroyed, each destructor would delete the same
    //     nodes (and the same buckets array), freeing memory twice, which
    //     crashes the program.
    // "= delete" makes the compiler refuse any code that tries to copy a
    // HashMap, so the mistake is caught when compiling instead of crashing
    // while the program runs.
    //
    // We lose nothing, because we never need a copy: there is one symbol
    // table and one opcode table, and we pass them around by reference.
    // If we ever needed a copy, we would write a copy constructor that
    // duplicates every node, the way DynamicArray does.
// ---------------------------------------------------------------

#ifndef HASHMAP_H
#define HASHMAP_H

#include <string>
#include "DynamicArray.h"

template <typename V>
class HashMap {
private:
    // One entry in a bucket's linked list.
    struct Node {
        std::string key;
        V value;
        Node* next;
        Node(const std::string& k, const V& v, Node* n) : key(k), value(v), next(n) {}
    };

    Node** buckets;   // array of pointers, each one is the head of a chain
    int bucketCount;  // how many buckets
    int count;        // how many key/value pairs are stored

    // Hash function: turns a string into a big number.
    // For each character: hash = hash * 31 + character.
    // (31 is a common choice; it mixes the characters well.)
    static unsigned long hashOf(const std::string& key) {
        unsigned long hash = 0;
        for (int i = 0; i < (int)key.length(); i++) {
            hash = hash * 31 + (unsigned char)key[i];
        }
        return hash;
    }

    // Which bucket does this key belong to
    int indexFor(const std::string& key, int numBuckets) const {
        return (int)(hashOf(key) % (unsigned long)numBuckets);
    }

    // Find the node with this key, or return 0 (NULL) if it is not there.
    Node* findNode(const std::string& key) const {
        Node* current = buckets[indexFor(key, bucketCount)];
        while (current != 0) {
            if (current->key == key) {
                return current;
            }
            current = current->next;
        }
        return 0;
    }

    // Double the number of buckets and move every node to its new place.
    void rehash() {
        int newBucketCount = bucketCount * 2;
        Node** newBuckets = new Node*[newBucketCount];
        for (int i = 0; i < newBucketCount; i++) {
            newBuckets[i] = 0;
        }
        // Go through every old chain and re-insert each node (no new nodes
        // are created, we just change the "next" pointers).
        for (int i = 0; i < bucketCount; i++) {
            Node* current = buckets[i];
            while (current != 0) {
                Node* nextNode = current->next;               // remember before we change it
                int newIndex = indexFor(current->key, newBucketCount);
                current->next = newBuckets[newIndex];         // put at front of new chain
                newBuckets[newIndex] = current;
                current = nextNode;
            }
        }
        delete[] buckets;
        buckets = newBuckets;
        bucketCount = newBucketCount;
    }

public:
    HashMap() {
        bucketCount = 8;
        count = 0;
        buckets = new Node*[bucketCount];
        for (int i = 0; i < bucketCount; i++) {
            buckets[i] = 0;
        }
    }

    ~HashMap() {
        clear();
        delete[] buckets;
    }

    // Copying is not allowed.
    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;

    int size() const { return count; }
    bool isEmpty() const { return count == 0; }

    // Insert a new pair, or replace the value if the key already exists.
    // Average O(1).
    void put(const std::string& key, const V& value) {
        Node* existing = findNode(key);
        if (existing != 0) {
            existing->value = value;       // key already there: just update
            return;
        }
        int index = indexFor(key, bucketCount);
        buckets[index] = new Node(key, value, buckets[index]);  // add at front
        count++;
        if (count * 4 > bucketCount * 3) { // more than 75% full
            rehash();
        }
    }

    // Look up a key. If found, copy the value into "result" and return true.
    // Average O(1).
    bool get(const std::string& key, V& result) const {
        Node* node = findNode(key);
        if (node == 0) {
            return false;
        }
        result = node->value;
        return true;
    }

    bool contains(const std::string& key) const {
        return findNode(key) != 0;
    }

    // Remove a key. Returns false if it was not there. Average O(1).
    bool remove(const std::string& key) {
        int index = indexFor(key, bucketCount);
        Node* current = buckets[index];
        Node* previous = 0;
        while (current != 0) {
            if (current->key == key) {
                if (previous == 0) {
                    buckets[index] = current->next;    // it was the first node
                } else {
                    previous->next = current->next;    // skip over it
                }
                delete current;
                count--;
                return true;
            }
            previous = current;
            current = current->next;
        }
        return false;
    }

    // Add every key to the END of "keys". The order is NOT meaningful
    // (it depends on the hash function), so sort afterwards if needed.
    // O(bucketCount + n): every bucket and every node is visited once.
    void getAllKeys(DynamicArray<std::string>& keys) const {
        for (int i = 0; i < bucketCount; i++) {
            Node* current = buckets[i];
            while (current != 0) {
                keys.pushBack(current->key);
                current = current->next;
            }
        }
    }

    // Delete every pair (the bucket array itself is kept).
    void clear() {
        for (int i = 0; i < bucketCount; i++) {
            Node* current = buckets[i];
            while (current != 0) {
                Node* nextNode = current->next;
                delete current;
                current = nextNode;
            }
            buckets[i] = 0;
        }
        count = 0;
    }
};

#endif
