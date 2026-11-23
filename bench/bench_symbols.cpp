// Challenge: compare symbol lookup in our HashMap (double hashing) with
// a plain linear list, as the program (number of labels) grows.
//
// For each size n we
//   1) build n labels  "label0" ... "label<n-1>"  in both structures,
//   2) look every label up once (n lookups) and measure the time.
//
// Expected result: the hash map needs about the same time PER lookup for
// every n (O(1) on average), while the list needs more and more time
// per lookup (O(n)), so its total time grows like n * n.
//
// to build and run:   make bench

#include <chrono>
#include <iostream>
#include <string>

#include "../ds/DynamicArray.h"
#include "../ds/HashMap.h"

using namespace std;

// One entry of the plain list.
struct Entry {
    string name;
    int value;
};

string makeLabel(int number) {
    return "label" + to_string(number);
}

// Linear search: look at the entries one by one. O(n).
bool listLookup(const DynamicArray<Entry>& list, const string& name, int& value) {
    for (int i = 0; i < list.size(); i++) {
        if (list[i].name == name) {
            value = list[i].value;
            return true;
        }
    }
    return false;
}

int main() {
    cout << "labels   hash map (ms)   list (ms)   list / hash" << endl;

    int sizes[] = {100, 500, 1000, 2000, 5000, 10000, 20000};
    for (int s = 0; s < 7; s++) {
        int n = sizes[s];

        HashMap<int> map;
        DynamicArray<Entry> list;
        DynamicArray<string> names;
        for (int i = 0; i < n; i++) {
            string name = makeLabel(i);
            names.pushBack(name);
            map.put(name, i);
            Entry entry;
            entry.name = name;
            entry.value = i;
            list.pushBack(entry);
        }

        long long checksum = 0;      // use the results so the compiler cannot skip the work

        auto start1 = chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            int value = 0;
            if (map.get(names[i], value)) checksum += value;
        }
        auto end1 = chrono::steady_clock::now();

        auto start2 = chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            int value = 0;
            if (listLookup(list, names[i], value)) checksum += value;
        }
        auto end2 = chrono::steady_clock::now();

        double hashMs = chrono::duration<double, milli>(end1 - start1).count();
        double listMs = chrono::duration<double, milli>(end2 - start2).count();
        cout << n << "\t " << hashMs << "\t\t " << listMs << "\t " << (listMs / (hashMs > 0 ? hashMs : 0.001))
             << "x   (check " << checksum << ")" << endl;
    }
    return 0;
}
