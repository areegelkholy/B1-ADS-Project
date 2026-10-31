# Mini Assembler and Instruction-Set Simulator (CSCE 2200, Project B1)

## Folder layout

```
ds/                  our data structures (Milestone 1)
  DynamicArray.h     resizable array
  Stack.h            LIFO stack (optional size limit)
  Queue.h            FIFO queue, linked list (optional size limit, searchable)
  MyString.h/.cpp    our own string class
  HashMap.h          string -> value hash map (separate chaining)
  HashSet.h/.cpp     set of ints (separate chaining)
  MinHeap.h          binary min-heap (tree stored in an array)
  Sort.h             heap sort built on MinHeap
```

## Initial plan

**Problem.** Programmers write assembly text; a processor runs binary machine
code. We build both tools in between, following the TISA-16 reference sheet exactly:
an assembler that translates text to machine code, and a simulator that runs it. Any
team's assembler output must run on any team's simulator, so the encodings must match
the spec bit for bit.

**Pipeline.**
1. Tokenizer
2. Pass 1: build symbol table, resolve forward references
3. Pass 2: encode instructions, output hex and listing file
4. Fetch–decode–execute loop
5. Stack operations in simulated memory with overflow/underflow detection.
6. Debugger (breakpoints, single-step, register dump)
7. Interrupt queue.

**Test programs.** The four official programs must produce exactly the expected output
and instruction counts from the spec:

| Program | Expected OUT values | Instructions |
|---|---|---|
| fibonacci.asm | 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377 | 94 |
| factorial.asm | 1, 2, 6, 24, 120, 720, 5040 | 863 |
| bubble.asm | -128, -7, 0, 5, 15, 42, 99, 300 | 298 |
| interrupts.asm + interrupts.txt | 2, 9, 50 | 209 |


