// Primes.h
// ---------------------------------------------------------------
// Tiny helpers to find prime numbers.
//
// WHY WE NEED THEM:
//   Our hash tables (HashMap and HashSet) use DOUBLE HASHING. For double
//   hashing to visit every slot of the table, the table size must be a
//   PRIME number (see HashMap.h). When a table grows we therefore need
//   "the first prime that is at least 2 * the old size".
//
// Both functions are marked "inline" so this header can be included from
// several .cpp files without a "defined twice" linker error.
// ---------------------------------------------------------------

#ifndef PRIMES_H
#define PRIMES_H

// True if n is prime. We try every divisor d with d * d <= n, (we only need to check divisor up to the square root of n)
// so the cost is O(sqrt(n)). Our table sizes are small, so this is fast.
inline bool isPrime(int n) {
    if (n < 2) {
        return false;
    }
    for (int d = 2; d * d <= n; d++) {
        if (n % d == 0) {
            return false;
        }
    }
    return true;
}

// Smallest prime that is >= n.
inline int nextPrime(int n) {
    while (!isPrime(n)) {
        n++;
    }
    return n;
}

#endif
