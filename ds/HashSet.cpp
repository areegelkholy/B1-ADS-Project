// HashSet.cpp
// Implementation of HashSet.

#include "HashSet.h"

// Hash function for ints: just the remainder after dividing by the number
// of buckets. Negative numbers give a negative remainder in C++, so we fix that.
int HashSet::indexFor(int value, int numBuckets) {
    int index = value % numBuckets;
    if (index < 0) {
        index = index + numBuckets;
    }
    return index;
}

HashSet::HashSet() {
    bucketCount = 8;
    count = 0;
    buckets = new Node*[bucketCount];
    for (int i = 0; i < bucketCount; i++) {
        buckets[i] = 0;
    }
}

HashSet::~HashSet() {
    clear();
    delete[] buckets;
}

// Walk the chain of the right bucket looking for the value.
bool HashSet::contains(int value) const {
    Node* current = buckets[indexFor(value, bucketCount)];
    while (current != 0) {
        if (current->value == value) {
            return true;
        }
        current = current->next;
    }
    return false;
}

bool HashSet::add(int value) {
    if (contains(value)) {
        return false;                 // sets never hold duplicates
    }
    int index = indexFor(value, bucketCount);
    Node* newNode = new Node;
    newNode->value = value;
    newNode->next = buckets[index];   // insert at the front of the chain
    buckets[index] = newNode;
    count++;
    if (count * 4 > bucketCount * 3) { // more than 75% full , so we grow
        rehash();
    }
    return true;
}

bool HashSet::remove(int value) {
    int index = indexFor(value, bucketCount);
    Node* current = buckets[index];
    Node* previous = 0;
    while (current != 0) {
        if (current->value == value) {
            if (previous == 0) {
                buckets[index] = current->next;   // it was the first node
            } else {
                previous->next = current->next;   // skip over it
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

void HashSet::rehash() {
    int newBucketCount = bucketCount * 2;
    Node** newBuckets = new Node*[newBucketCount];
    for (int i = 0; i < newBucketCount; i++) {
        newBuckets[i] = 0;
    }
    // Move every node into its place in the bigger table.
    for (int i = 0; i < bucketCount; i++) {
        Node* current = buckets[i];
        while (current != 0) {
            Node* nextNode = current->next;               // save before changing
            int newIndex = indexFor(current->value, newBucketCount);
            current->next = newBuckets[newIndex];
            newBuckets[newIndex] = current;
            current = nextNode;
        }
    }
    delete[] buckets;
    buckets = newBuckets;
    bucketCount = newBucketCount;
}

void HashSet::clear() {
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

int HashSet::size() const {
    return count;
}

bool HashSet::isEmpty() const {
    return count == 0;
}
