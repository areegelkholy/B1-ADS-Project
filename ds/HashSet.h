// HashSet.h
// A set of integers with fast add / contains / remove (average O(1)).
// Same idea as HashMap (buckets + linked chains + rehashing), but it
// stores only ints and no values.
//
// WHY WE NEED IT:
//   The debugger keeps the addresses of all breakpoints in a HashSet.
//   Before every instruction the simulator asks "is this address a
//   breakpoint?", so that check must be O(1).
//


#ifndef HASHSET_H
#define HASHSET_H

class HashSet {
private:
    struct Node {
        int value;
        Node* next;
    };

    Node** buckets;   // each bucket is the head of a linked chain
    int bucketCount;
    int count;

    static int indexFor(int value, int numBuckets);  // which bucket for this value
    void rehash();                                   // double the buckets

public:
    HashSet();
    ~HashSet();

    // Copying is disabled (same reason as HashMap and Queue).
    //
    // WHY: the set owns its linked nodes, which live on the heap. If C++ made
    // a copy automatically, it would only copy the "buckets" pointer, so the
    // original and the copy would share the SAME nodes. Then:
    //   - changing one set would silently change the other, and
    //   - when both are destroyed, each destructor would delete the same
    //     nodes, freeing the same memory twice, which crashes the program.
    // "= delete" makes the compiler refuse any code that tries to copy a
    // HashSet, so the mistake is caught when compiling instead of crashing
    // while the program runs.
    
    HashSet(const HashSet&) = delete;
    HashSet& operator=(const HashSet&) = delete;

    bool add(int value);          // returns false if it was already in the set
    bool contains(int value) const;
    bool remove(int value);       // returns false if it was not in the set
    void clear();
    int size() const;
    bool isEmpty() const;
};

#endif
