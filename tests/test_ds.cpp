// test_ds.cpp
// ---------------------------------------------------------------
// Simple tests for every data structure in the ds/ folder.
// Each test calls check(condition, "what we are testing").
// At the end we print how many checks passed and failed.
//
// Build and run with:   make test
// ---------------------------------------------------------------

#include <iostream>
#include <string>

#include "../ds/DynamicArray.h"
#include "../ds/Stack.h"
#include "../ds/Queue.h"
#include "../ds/HashMap.h"
#include "../ds/HashSet.h"
#include "../ds/MinHeap.h"
#include "../ds/Sort.h"

using namespace std;

int passed = 0;
int failed = 0;

// Records the result of one check and prints a message if it failed.
void check(bool condition, const char* description) {
    if (condition) {
        passed++;
    } else {
        failed++;
        cout << "  FAILED: " << description << endl;
    }
}

// Builds the key "label0", "label1", ... so we can make many different keys.
std::string makeKey(int number) {
    std::string key("label");
    if (number == 0) {
        key.push_back('0');
    }
    // collect digits in reverse order, then append them the right way round
    char digits[12];
    int digitCount = 0;
    while (number > 0) {
        digits[digitCount] = (char)('0' + number % 10);
        digitCount++;
        number = number / 10;
    }
    for (int i = digitCount - 1; i >= 0; i--) {
        key.push_back(digits[i]);
    }
    return key;
}

// ---------------- DynamicArray ----------------
void testDynamicArray() {
    cout << "Testing DynamicArray..." << endl;

    DynamicArray<int> arr;
    check(arr.isEmpty(), "new array is empty");

    // push 100 items: this forces the array to grow several times
    for (int i = 0; i < 100; i++) {
        arr.pushBack(i * 2);
    }
    check(arr.size() == 100, "size is 100 after 100 pushes");
    check(arr[0] == 0 && arr[50] == 100 && arr[99] == 198, "values survive growing");

    arr.removeLast();
    check(arr.size() == 99, "removeLast reduces size");
    check(arr[98] == 196, "last item is correct after removeLast");

    // out-of-range access must throw
    bool threw = false;
    try {
        int x = arr[500];
        (void)x;
    } catch (...) {
        threw = true;
    }
    check(threw, "index out of range throws");

    // copy must be independent from the original
    DynamicArray<int> copy(arr);
    copy[0] = 999;
    check(arr[0] == 0 && copy[0] == 999, "copy constructor makes a deep copy");

    DynamicArray<int> assigned;
    assigned = arr;
    assigned[1] = 777;
    check(arr[1] == 2 && assigned[1] == 777, "assignment makes a deep copy");

    // fill is used to create the simulator memory
    DynamicArray<int> memory;
    memory.fill(1000, 0);
    check(memory.size() == 1000 && memory[999] == 0, "fill creates n items");

    arr.clear();
    check(arr.isEmpty(), "clear empties the array");

    // Pushing an item of the array into the SAME array while it is full.
    // pushBack has to grow, which frees the old memory that the argument lives in.
    DynamicArray<std::string> words(2);
    words.pushBack("a string long enough to be stored on the heap, not inside the object");
    words.pushBack("second");
    words.pushBack(words[0]);   // array is full here, so this push must grow first
    check(words.size() == 3 && words[2] == words[0], "pushBack of own element works while growing");
}

// ---------------- Stack ----------------
void testStack() {
    cout << "Testing Stack..." << endl;

    Stack<int> s;
    int value = 0;
    check(s.isEmpty(), "new stack is empty");
    check(!s.pop(value), "pop on empty stack returns false");
    check(!s.peek(value), "peek on empty stack returns false");

    s.push(10);
    s.push(20);
    s.push(30);
    check(s.size() == 3, "size is 3 after three pushes");

    check(s.peek(value) && value == 30, "peek shows the top (30)");
    check(s.size() == 3, "peek does not remove");

    // LIFO order: last in, first out
    s.pop(value);
    check(value == 30, "first pop gives 30");
    s.pop(value);
    check(value == 20, "second pop gives 20");
    s.pop(value);
    check(value == 10, "third pop gives 10");
    check(s.isEmpty(), "stack empty again");

    // stack with a limit of 3 (detects overflow)
    Stack<int> limited(3);
    check(limited.push(1), "push 1 fits");
    check(limited.push(2), "push 2 fits");
    check(limited.push(3), "push 3 fits");
    check(limited.isFull(), "limited stack is full");
    check(!limited.push(4), "push 4 fails (overflow)");
    check(limited.size() == 3, "size stays 3 after failed push");

    limited.pop(value);
    check(limited.push(4), "push works again after a pop");
}

// ---------------- Queue ----------------
void testQueue() {
    cout << "Testing Queue..." << endl;

    Queue<int> q;
    int value = 0;
    check(q.isEmpty(), "new queue is empty");
    check(!q.dequeue(value), "dequeue on empty queue returns false");
    check(!q.peek(value), "peek on empty queue returns false");

    q.enqueue(1);
    q.enqueue(2);
    q.enqueue(3);
    check(q.size() == 3, "size is 3");
    check(q.peek(value) && value == 1, "peek shows the front (1)");

    // FIFO order: first in, first out
    q.dequeue(value);
    check(value == 1, "first dequeue gives 1");
    q.dequeue(value);
    check(value == 2, "second dequeue gives 2");
    q.dequeue(value);
    check(value == 3, "third dequeue gives 3");
    check(q.isEmpty(), "queue empty again");

    // the queue must still work after becoming empty (tail was reset)
    q.enqueue(42);
    check(q.dequeue(value) && value == 42, "queue works after being emptied");

    // queue with a limit of 2
    Queue<int> small(2);
    check(small.enqueue(1), "enqueue 1 fits");
    check(small.enqueue(2), "enqueue 2 fits");
    check(small.isFull(), "small queue is full");
    check(!small.enqueue(3), "enqueue 3 fails (full)");

    // clear() frees all nodes
    Queue<int> big;
    for (int i = 0; i < 100; i++) {
        big.enqueue(i);
    }
    big.clear();
    check(big.isEmpty() && big.size() == 0, "clear empties the queue");
}

// ---------------- HashMap ----------------
void testHashMap() {
    cout << "Testing HashMap..." << endl;

    HashMap<int> symbols;      // like the assembler symbol table
    int address = -1;
    check(symbols.isEmpty(), "new map is empty");
    check(!symbols.get(std::string("LOOP"), address), "missing key returns false");

    symbols.put(std::string("START"), 0);
    symbols.put(std::string("LOOP"), 8);
    symbols.put(std::string("END"), 40);
    check(symbols.size() == 3, "size is 3");
    check(symbols.get(std::string("LOOP"), address) && address == 8, "LOOP maps to 8");
    check(symbols.contains(std::string("END")), "contains END");
    check(!symbols.contains(std::string("NOPE")), "does not contain NOPE");

    // putting an existing key replaces the value, size does not change
    symbols.put(std::string("LOOP"), 12);
    check(symbols.size() == 3, "size unchanged when key is replaced");
    check(symbols.get(std::string("LOOP"), address) && address == 12, "value was replaced");

    // keys with the same letters in a different order are different keys
    symbols.put(std::string("AB"), 1);
    symbols.put(std::string("BA"), 2);
    check(symbols.get(std::string("AB"), address) && address == 1, "AB is 1");
    check(symbols.get(std::string("BA"), address) && address == 2, "BA is 2");

    // remove
    check(symbols.remove(std::string("LOOP")), "remove existing key returns true");
    check(!symbols.contains(std::string("LOOP")), "removed key is gone");
    check(!symbols.remove(std::string("LOOP")), "removing again returns false");

    // many keys: forces several rehashes (the table starts with 8 buckets)
    HashMap<int> big;
    for (int i = 0; i < 2000; i++) {
        big.put(makeKey(i), i * 3);
    }
    check(big.size() == 2000, "2000 keys stored");
    bool allFound = true;
    for (int i = 0; i < 2000; i++) {
        int found = -1;
        if (!big.get(makeKey(i), found) || found != i * 3) {
            allFound = false;
        }
    }
    check(allFound, "all 2000 keys still found after rehashing");

    // load factor: the table must never be more than half full
    check(big.capacity() >= 2 * big.size(), "hash map is at most 50% full");

    big.clear();
    check(big.isEmpty(), "clear empties the map");

    // Removing a key must not break the search for other keys that were
    // pushed past it by a collision (this is what the tombstones are for).
    HashMap<int> many;
    for (int i = 0; i < 40; i++) {
        many.put(makeKey(i), i);
    }
    for (int i = 0; i < 40; i += 2) {
        many.remove(makeKey(i));                    // remove every second key
    }
    bool restFound = true;
    for (int i = 1; i < 40; i += 2) {
        int found = -1;
        if (!many.get(makeKey(i), found) || found != i) {
            restFound = false;
        }
    }
    check(restFound, "keys are still found after other keys were removed");
    check(!many.contains(makeKey(0)), "a removed key stays removed");
    many.put(makeKey(0), 77);
    int back = -1;
    check(many.get(makeKey(0), back) && back == 77, "a removed key can be added again");
    check(many.size() == 21, "size is correct after removes and adds");

    // many inserts and removes: tombstones must be cleaned up by rehashing
    HashMap<int> churn;
    for (int round = 0; round < 3000; round++) {
        churn.put(makeKey(round), round);
        if (round >= 5) {
            churn.remove(makeKey(round - 5));       // keep only the last 5 keys
        }
    }
    check(churn.size() == 5, "churn: only the last 5 keys remain");
    check(churn.contains(makeKey(2999)) && !churn.contains(makeKey(2990)),
          "churn: newest key found, old key gone");
}

// ---------------- HashSet ----------------
void testHashSet() {
    cout << "Testing HashSet..." << endl;

    HashSet breakpoints;
    check(breakpoints.isEmpty(), "new set is empty");

    check(breakpoints.add(100), "adding 100 works");
    check(breakpoints.add(200), "adding 200 works");
    check(!breakpoints.add(100), "adding 100 again returns false (no duplicates)");
    check(breakpoints.size() == 2, "size is 2");

    check(breakpoints.contains(100), "contains 100");
    check(!breakpoints.contains(300), "does not contain 300");

    check(breakpoints.remove(100), "remove 100 works");
    check(!breakpoints.contains(100), "100 is gone");
    check(!breakpoints.remove(100), "removing 100 again returns false");

    // negative numbers must work too
    check(breakpoints.add(-5), "adding -5 works");
    check(breakpoints.contains(-5), "contains -5");

    // many values: forces rehashing
    HashSet big;
    for (int i = 0; i < 5000; i++) {
        big.add(i * 7);
    }
    check(big.size() == 5000, "5000 values stored");
    bool allFound = true;
    for (int i = 0; i < 5000; i++) {
        if (!big.contains(i * 7)) {
            allFound = false;
        }
    }
    check(allFound, "all 5000 values found after rehashing");
    check(!big.contains(1), "1 is not in the set (not a multiple of 7)");

    check(big.capacity() >= 2 * big.size(), "hash set is at most 50% full");

    big.clear();
    check(big.isEmpty(), "clear empties the set");

    // tombstones: removing must not hide other values
    HashSet s;
    for (int i = 0; i < 60; i++) {
        s.add(i * 11);
    }
    for (int i = 0; i < 60; i += 2) {
        s.remove(i * 11);
    }
    bool restFound = true;
    for (int i = 1; i < 60; i += 2) {
        if (!s.contains(i * 11)) {
            restFound = false;
        }
    }
    check(restFound, "values are still found after other values were removed");
    check(!s.contains(0) && s.size() == 30, "removed values are gone, size is 30");
    check(s.add(0) && s.contains(0), "a removed value can be added again");

    // many adds and removes
    HashSet churn;
    for (int round = 0; round < 5000; round++) {
        churn.add(round);
        if (round >= 3) {
            churn.remove(round - 3);
        }
    }
    check(churn.size() == 3 && churn.contains(4999) && !churn.contains(4990),
          "churn: only the last 3 values remain");
}

// ---------------- MinHeap ----------------

// Example of our own type for the profiler: an instruction and how many
// times it was executed. The heap compares entries by "count".
struct ProfileEntry {
    int opcode;
    int count;
    bool operator<(const ProfileEntry& other) const {
        return count < other.count;
    }
};

void testMinHeap() {
    cout << "Testing MinHeap..." << endl;

    MinHeap<int> heap;
    int value = 0;
    check(heap.isEmpty(), "new heap is empty");
    check(!heap.extractMin(value), "extractMin on empty heap returns false");
    check(!heap.peekMin(value), "peekMin on empty heap returns false");

    int input[] = {5, 3, 8, 1, 9, 2, 7, 3};   // note: 3 appears twice
    for (int i = 0; i < 8; i++) {
        heap.insert(input[i]);
    }
    check(heap.size() == 8, "size is 8");
    check(heap.peekMin(value) && value == 1, "smallest is 1");

    // extracting everything must give sorted order
    int expected[] = {1, 2, 3, 3, 5, 7, 8, 9};
    bool sorted = true;
    for (int i = 0; i < 8; i++) {
        if (!heap.extractMin(value) || value != expected[i]) {
            sorted = false;
        }
    }
    check(sorted, "extractMin returns values in sorted order");
    check(heap.isEmpty(), "heap empty after extracting everything");

    // bigger test: insert 1000 values in a scrambled order
    MinHeap<int> big;
    for (int i = 0; i < 1000; i++) {
        big.insert((i * 37) % 1000);       // a permutation of 0..999
    }
    bool bigSorted = true;
    for (int i = 0; i < 1000; i++) {
        if (!big.extractMin(value) || value != i) {
            bigSorted = false;
        }
    }
    check(bigSorted, "1000 scrambled values come out sorted");

    // Profiler example: keep the top K = 3 most executed instructions.
    int counts[] = {50, 10, 90, 30, 70, 20, 60};
    int K = 3;
    MinHeap<ProfileEntry> topK;
    for (int i = 0; i < 7; i++) {
        ProfileEntry entry;
        entry.opcode = i;
        entry.count = counts[i];
        if (topK.size() < K) {
            topK.insert(entry);
        } else {
            ProfileEntry smallest;
            topK.peekMin(smallest);
            if (smallest.count < entry.count) {   // new one is bigger: replace
                topK.extractMin(smallest);
                topK.insert(entry);
            }
        }
    }
    check(topK.size() == 3, "top-K heap holds exactly K entries");
    ProfileEntry e;
    topK.extractMin(e);
    check(e.count == 60, "third most executed has count 60");
    topK.extractMin(e);
    check(e.count == 70, "second most executed has count 70");
    topK.extractMin(e);
    check(e.count == 90, "most executed has count 90");
}

// ================================================================
// Tests for the functions added after reading the TISA-16 spec
// ================================================================

// ---------------- Queue: getAt and contains (prefetch buffer) ----------------
void testQueueExtras() {
    cout << "Testing Queue extras..." << endl;

    Queue<int> q;
    int value = 0;
    check(!q.contains(5), "empty queue contains nothing");
    check(!q.getAt(0, value), "getAt on empty queue returns false");

    q.enqueue(10);
    q.enqueue(20);
    q.enqueue(30);
    check(q.contains(10) && q.contains(20) && q.contains(30), "contains finds all three values");
    check(!q.contains(99), "contains does not find 99");

    check(q.getAt(0, value) && value == 10, "getAt(0) is the front");
    check(q.getAt(2, value) && value == 30, "getAt(2) is the back");
    check(!q.getAt(3, value), "getAt(3) is out of range");
    check(!q.getAt(-1, value), "getAt(-1) is out of range");
    check(q.size() == 3, "getAt and contains do not remove anything");

    // after a dequeue the front has moved
    q.dequeue(value);
    check(!q.contains(10), "10 is gone after dequeue");
    check(q.getAt(0, value) && value == 20, "new front is 20");

    // flushing (clear) empties it, like a taken branch empties the prefetch buffer
    q.clear();
    check(!q.contains(20), "nothing is found after clear");
}

// ---------------- HashMap: getAllKeys ----------------
void testHashMapKeys() {
    cout << "Testing HashMap getAllKeys..." << endl;

    HashMap<int> empty;
    DynamicArray<std::string> noKeys;
    empty.getAllKeys(noKeys);
    check(noKeys.size() == 0, "empty map has no keys");

    // a symbol table like the one in the spec's worked example
    HashMap<int> symbols;
    symbols.put(std::string("start"), 0);
    symbols.put(std::string("loop"), 3);
    symbols.put(std::string("COUNT"), 15);
    symbols.put(std::string("Beta"), 7);
    symbols.put(std::string("alpha"), 9);

    DynamicArray<std::string> keys;
    symbols.getAllKeys(keys);
    check(keys.size() == 5, "five keys returned");

    heapSort(keys);
    check(keys[0] == std::string("Beta") && keys[1] == std::string("COUNT") &&
          keys[2] == std::string("alpha") && keys[3] == std::string("loop") &&
          keys[4] == std::string("start"),
          "sorted keys are in ASCII order");

    // removed keys must not be reported
    symbols.remove(std::string("loop"));
    DynamicArray<std::string> afterRemove;
    symbols.getAllKeys(afterRemove);
    check(afterRemove.size() == 4, "removed key is not returned");

    // many keys: every key must come back exactly once
    HashMap<int> big;
    for (int i = 0; i < 2000; i++) {
        big.put(makeKey(i), i);
    }
    DynamicArray<std::string> bigKeys;
    big.getAllKeys(bigKeys);
    check(bigKeys.size() == 2000, "2000 keys returned");
    heapSort(bigKeys);
    bool noDuplicates = true;
    for (int i = 1; i < bigKeys.size(); i++) {
        if (!(bigKeys[i - 1] < bigKeys[i])) {   // strictly increasing = sorted and no duplicates
            noDuplicates = false;
        }
    }
    check(noDuplicates, "2000 keys are all different and sorted");
}

// True if the array is in non-decreasing order.
bool isSortedInts(const DynamicArray<int>& arr) {
    for (int i = 1; i < arr.size(); i++) {
        if (arr[i] < arr[i - 1]) {
            return false;
        }
    }
    return true;
}

// ---------------- heapSort ----------------
void testHeapSort() {
    cout << "Testing heapSort..." << endl;

    DynamicArray<int> empty;
    heapSort(empty);
    check(empty.size() == 0, "sorting an empty array is fine");

    DynamicArray<int> one;
    one.pushBack(7);
    heapSort(one);
    check(one.size() == 1 && one[0] == 7, "sorting one item is fine");

    // the numbers from the bubble.asm test program, plus a duplicate
    int input[] = {42, -7, 300, 0, 15, -128, 99, 5, 42};
    DynamicArray<int> arr;
    for (int i = 0; i < 9; i++) {
        arr.pushBack(input[i]);
    }
    heapSort(arr);
    int expected[] = {-128, -7, 0, 5, 15, 42, 42, 99, 300};
    bool same = (arr.size() == 9);
    for (int i = 0; i < 9 && same; i++) {
        if (arr[i] != expected[i]) {
            same = false;
        }
    }
    check(same, "bubble.asm numbers (with a duplicate) are sorted");

    // already sorted and reverse sorted
    DynamicArray<int> ascending;
    DynamicArray<int> descending;
    for (int i = 0; i < 500; i++) {
        ascending.pushBack(i);
        descending.pushBack(499 - i);
    }
    heapSort(ascending);
    heapSort(descending);
    check(isSortedInts(ascending) && ascending.size() == 500, "already sorted input stays sorted");
    check(isSortedInts(descending) && descending[0] == 0 && descending[499] == 499,
          "reverse sorted input gets sorted");

    // 1000 scrambled values (a permutation of 0..999)
    DynamicArray<int> scrambled;
    for (int i = 0; i < 1000; i++) {
        scrambled.pushBack((i * 37) % 1000);
    }
    heapSort(scrambled);
    bool exact = true;
    for (int i = 0; i < 1000; i++) {
        if (scrambled[i] != i) {
            exact = false;
        }
    }
    check(exact, "1000 scrambled values become 0..999");

    // strings
    DynamicArray<std::string> words;
    words.pushBack(std::string("start"));
    words.pushBack(std::string("loop"));
    words.pushBack(std::string("COUNT"));
    heapSort(words);
    check(words[0] == std::string("COUNT") && words[1] == std::string("loop") && words[2] == std::string("start"),
          "strings sort like the spec's symbol table (COUNT, loop, start)");
}

int main() {
    testDynamicArray();
    testStack();
    testQueue();
    testHashMap();
    testHashSet();
    testMinHeap();
    testQueueExtras();
    testHashMapKeys();
    testHeapSort();

    cout << endl;
    cout << "Checks passed: " << passed << endl;
    cout << "Checks failed: " << failed << endl;
    if (failed == 0) {
        cout << "ALL TESTS PASSED" << endl;
    }
    return failed;   // 0 means success
}
