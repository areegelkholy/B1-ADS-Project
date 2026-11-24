# TISA-16 Assembler, Simulator and Debugger

CSCE 2200 Applied Data Structures, Fall 2026, Project B1 (Mini Assembler and Instruction-Set Simulator).
The American University in Cairo.

This project is a two-pass assembler, an instruction-set simulator and a debugger for **TISA-16**, the
teaching ISA from the course reference sheet (16-bit words, 256 words of memory, 8 registers). It is written
in C++11. Every container and algorithm the program relies on (dynamic array, stack, queue, hash map, hash
set, min-heap, heap sort) is our own code. We do not use STL containers or STL algorithms. The only library
class we use is `std::string` for text, plus `iostream` and `fstream` for console and files.

**Team:** Amina Elshafeii 900231877, Farida El Alaily 900232141, Areeg Elkholy 900231292, Rofaida El-Gazzar 900235494, Abdelrahman Emara 9234017
**Repository:** <https://github.com/areegelkholy/B1-ADS-Project.git>

## Contents

1. [Quick start](#1-quick-start)
2. [The TISA-16 machine](#2-the-tisa-16-machine)
3. [Using the tools](#3-using-the-tools)
4. [The assembly language](#4-the-assembly-language)
5. [How the code is organized](#5-how-the-code-is-organized)
6. [Walkthrough: what happens to a program](#6-walkthrough-what-happens-to-a-program)
7. [Data structures](#7-data-structures)
8. [Extras: macros, profiler, pipeline, benchmark](#8-extras-macros-profiler-pipeline-benchmark)
9. [Testing](#9-testing)
10. [Design notes, assumptions and limits](#10-design-notes-assumptions-and-limits)
11. [Changes since Milestone 1](#11-changes-since-milestone-1)

---

## 1. Quick start

You need `g++` with C++11 support and `make`.

```
make            # builds the two programs: asm and sim
make test       # builds and runs every test (data structures + application)
make bench      # symbol lookup benchmark: hash map vs linear list
make clean
```

Without `make`, compile the two programs by hand (the file lists are the same except for the `main` file):

```
g++ -std=c++11 -o asm app/asm_main.cpp app/Format.cpp app/Isa.cpp app/Tokenizer.cpp app/Expression.cpp app/Macro.cpp app/Assembler.cpp app/Simulator.cpp app/Debugger.cpp app/Profiler.cpp app/Pipeline.cpp ds/HashSet.cpp
g++ -std=c++11 -o sim app/sim_main.cpp app/Format.cpp app/Isa.cpp app/Tokenizer.cpp app/Expression.cpp app/Macro.cpp app/Assembler.cpp app/Simulator.cpp app/Debugger.cpp app/Profiler.cpp app/Pipeline.cpp ds/HashSet.cpp
```

First run:

```
$ ./asm tests/programs/fibonacci.asm
assembled 10 words, wrote tests/programs/fibonacci.hex and tests/programs/fibonacci.lst
$ ./sim tests/programs/fibonacci.hex
0
1
1
...
377
HALT at 09 after 94 instructions
PC=0A SP=FC Z=1 N=0 IE=0
R0=0000 R1=0262 R2=03DB R3=0000
R4=03DB R5=0000 R6=0000 R7=0000
prefetch flushes: 14
```

## 2. The TISA-16 machine

The reference sheet from the TAs is the full specification. This is the short version needed to read the code.

| Part | Description |
|---|---|
| Word | 16 bits, every instruction is exactly one word |
| Memory | 256 words, word-addressed (`0x00` to `0xFF`) |
| Registers | `R0` to `R7`, 16 bits. `R0` always reads 0, writes to it are ignored |
| `PC`, `SP` | 8 bits each. `SP` is not a general register |
| Flags | `Z` (zero) and `N` (negative / less than). `IE` is the interrupt enable bit |
| Numbers | 16-bit two's complement, arithmetic wraps around |
| Output | the `OUT` instruction prints a register as a signed decimal on its own line |

Memory map:

| Addresses | Use |
|---|---|
| `0x00` upward | program code and data. Execution starts at `0x00` |
| up to `0xFB` | stack, grows downward from `0xFB` (`SP` = `0xFC` means empty) |
| `0xFC` to `0xFF` | interrupt vector table, word `0xFC + n` is the handler of interrupt line `n` |

Instruction set:

| Group | Instructions |
|---|---|
| Arithmetic and logic | `ADD`, `SUB`, `AND`, `OR`, `XOR` (three registers), `ADDI` (register + 6-bit immediate) |
| Data movement | `LDI` (9-bit immediate), `LW`, `SW` (`off(rs)` addressing) |
| Compare | `CMP rs, rt` sets `Z` and `N` using a signed comparison |
| Branches | `JMP`, `BEQ`, `BNE`, `BLT`, `BGE`, `BGT`, `BLE` (absolute 8-bit address) |
| Subroutines and stack | `CALL`, `RET`, `PUSH`, `POP` |
| System | `NOP`, `HALT`, `OUT`, `EI`, `DI`, `RETI` |
| Pseudo-instructions | `MOV`, `INC`, `DEC`, `CLR` (each becomes exactly one real instruction) |

## 3. Using the tools

### asm: the assembler

```
./asm program.asm        # writes program.hex and program.lst
```

- `program.hex` has one line per assembled word: `AA: WWWW` (address and word in uppercase hex).
- `program.lst` is the listing: address, word, source line number and the source text, followed by the
  symbol table sorted by name.
- All errors are reported, not only the first, and they are sorted by line number. If there is any error,
  no `.hex` file is written.

```
$ ./asm tests/programs/errors/multiple.asm
line 1: error: invalid label '1bad'
line 2: error: unknown mnemonic 'BAD'
line 3: error: undefined symbol 'nothing'
line 4: error: invalid register 'R9' (use R0 to R7)
4 error(s), no output written
```

Start of the listing of `fibonacci.asm`:

```
             1  ; fibonacci.asm - prints the first 15 Fibonacci numbers
             2          .EQU COUNT, 15
             3
00  0200     4  start:  CLR  R1             ; a = 0
01  6401     5          LDI  R2, 1          ; b = 1
02  660F     6          LDI  R3, COUNT      ; counter
03  F042     7  loop:   OUT  R1
...
COUNT = 0F
loop = 03
```

### sim: the simulator

```
./sim program.hex [interrupts.txt]         # run until HALT
./sim -d program.hex [interrupts.txt]      # run under the debugger
```

| Option | Effect |
|---|---|
| `-d` | start the debugger prompt before the first instruction |
| `-l N` | stop after N instructions (default 1,000,000, `0` means no limit) |
| `-p K` | after the run, print the K most executed instructions (profiler) |
| `--pipeline` | after the run, report how many cycles a 5-stage pipeline would need |
| `--forwarding` | same, with result forwarding between stages |
| `--pipeline-log N` | also print the pipeline stages of the first N cycles as a table |

The optional interrupt file has one interrupt per line, `<cycle> <line>`, both decimal.

On `HALT` the simulator prints the halt report (see the first run above). On a runtime error it prints
`runtime error at <PC>: <message>` and the same register dump. Runtime errors are: invalid branch condition
or system function, stack overflow, stack underflow, `LW`/`SW` outside `0x00` to `0xFF`, and the PC moving
past `0xFF`.

Exit codes: `0` after `HALT`, `1` after a runtime error or when the instruction limit is hit, `2` for wrong
usage or an unreadable file.

### The debugger

| Command | Action |
|---|---|
| `break <addr>` / `b` | set a breakpoint (hex address, with or without `0x`) |
| `delete <addr>` / `d` | remove a breakpoint |
| `breaks` | list the breakpoints |
| `run` / `r` | run until a breakpoint (stops before that instruction), `HALT` or an error |
| `step [n]` / `s` | execute 1 or n instructions, ignoring breakpoints, then show the next one |
| `regs` | print `PC SP Z N IE` and `R0` to `R7` |
| `mem <addr> [count]` | print count words (default 8) from addr |
| `stack` | print the words on the stack, top first |
| `quit` / `q` | exit |

```
$ ./sim -d tests/programs/fibonacci.hex
TISA-16 debugger. Commands: break, delete, breaks, run, step, regs, mem, stack, quit
00: 0200 ADD R1, R0, R0
(tisa) break 07
breakpoint set at 07
(tisa) run
0
breakpoint at 07
07: 56FF ADDI R3, R3, -1
(tisa) regs
PC=07 SP=FC Z=0 N=0 IE=0
R0=0000 R1=0001 R2=0001 R3=000F
R4=0001 R5=0000 R6=0000 R7=0000
(tisa) step 2
03: F042 OUT R1
(tisa) mem 00 4
00: 0200
01: 6401
02: 660F
03: F042
(tisa) stack
the stack is empty
(tisa) quit
```

The debugger shows real instructions only: the source line `DEC R3` is displayed as `ADDI R3, R3, -1`.

## 4. The assembly language

One source line looks like this, and every part is optional:

```
[label:] [mnemonic or directive operand, operand, ...] [; comment]
```

- Mnemonics, directives and register names are case-insensitive. Labels are case-sensitive.
- A label is a letter or `_` followed by letters, digits or `_`. Its value is its address.
- Labels and `.EQU` names share one symbol table, so using the same name for both is a duplicate error.
- Forward references work, which is the reason for the two passes.

Directives:

| Directive | Effect |
|---|---|
| `.ORG expr` | set the assembly address |
| `.EQU name, expr` | define a constant (no memory used) |
| `.WORD e1, e2, ...` | store one 16-bit word per value |
| `.SPACE expr` | reserve `expr` words initialised to 0 |

Constant expressions are allowed wherever a number is expected: decimal (`42`), hex (`0x2A`), binary
(`0b101010`), symbols, the operators `+ - * / %` with parentheses and unary minus. `/` and `%` truncate
toward zero. Expressions in `.ORG`, `.EQU` and `.SPACE` can only use symbols defined earlier in the file.

Macros (an extra, see section 8):

```
.MACRO swap, x, y        ; name, then the parameters
    MOV R7, x
    MOV x, y
    MOV y, R7
.ENDM
        swap R1, R2      ; expands to the three MOV lines with x = R1 and y = R2
```

`@@` inside a macro body becomes a number that is unique per call, so `done@@:` can be used as a label
in a macro that is called more than once.

## 5. How the code is organized

```
ds/                    our data structures (no application knowledge in here)
  DynamicArray.h       resizable array
  Stack.h              LIFO stack with an optional size limit
  Queue.h              FIFO queue (linked list), optional size limit, searchable
  HashMap.h            string -> value, open addressing with double hashing
  HashSet.h/.cpp       set of ints, same technique
  Primes.h             prime numbers for the table sizes
  MinHeap.h            binary min-heap stored in an array
  Sort.h               heap sort built on MinHeap
app/                   the application, uses ds/ only through public functions
  Format.h/.cpp        text helpers: hex printing, number parsing, trimming, file read/write
  Isa.h/.cpp           the instruction set: opcode table, register names, disassembler
  Tokenizer.h/.cpp     splits one source line into label, mnemonic and operands
  Expression.h/.cpp    constant expressions with the shunting-yard algorithm
  Macro.h/.cpp         macro expansion with a stack of active macros
  Assembler.h/.cpp     pass 1, pass 2, hex and listing output, error list
  Simulator.h/.cpp     the machine: fetch, decode, execute, prefetch queue, interrupts
  Debugger.h/.cpp      the command prompt and the breakpoints
  Profiler.h/.cpp      top-K most executed instructions
  Pipeline.h/.cpp      5-stage pipeline timing model
  asm_main.cpp         the "asm" program
  sim_main.cpp         the "sim" program
tests/
  test_ds.cpp          tests for every data structure
  test_app.cpp         tests for the application
  programs/            test programs and their expected output
bench/
  bench_symbols.cpp    hash map vs linear list benchmark
docs/
  DESIGN.md            architecture, algorithms and trade-offs in detail
Makefile
```

There are two layers. `ds/` knows nothing about assemblers or CPUs. `app/` uses it only through the public
functions. Files and the console are touched only in the two `main` files and in the debugger, so
`Assembler` and `Simulator` take and return strings and can be tested without any file.

`DynamicArray`, `Stack`, `Queue`, `HashMap`, `MinHeap` and `heapSort` are templates, so they live entirely
in their `.h` file (the compiler needs the full code to build a version for a given type). `HashSet` is a
normal class and is split into `.h` and `.cpp`.

**Suggested reading order:** `ds/DynamicArray.h`, `ds/Stack.h`, `ds/Queue.h`, `ds/HashMap.h`, `ds/MinHeap.h`,
then `app/Isa.h`, `app/Tokenizer.cpp`, `app/Expression.cpp`, `app/Assembler.cpp`, `app/Simulator.cpp`,
`app/Debugger.cpp`, and finally the two `main` files.

## 6. Walkthrough: what happens to a program

### Assembling (`asm`)

1. `asm_main.cpp` reads the `.asm` file into a string and calls `Assembler::assemble(text)`.
2. **Macros** are expanded first (`MacroExpander`), so the two passes never see them.
3. **Tokenizer.** Each line is cut into a label, a mnemonic (made upper case) and a list of operands. The
   comment after `;` is dropped and spaces inside operands are removed.
4. **Pass 1** walks over the lines with a location counter. Labels go into the symbol table (a `HashMap`),
   `.ORG`, `.EQU`, `.WORD` and `.SPACE` are processed, and every instruction takes one word. After this
   pass every label has an address.
5. **Pass 2** walks over the lines again. The opcode table (`HashMap`, mnemonic to opcode, format and
   extra bits) says how to read the operands, and one `switch` on the format packs the fields into the 16-bit
   word. Operand expressions are evaluated with the shunting-yard algorithm in `Expression.cpp`. Register
   names, operand counts, field ranges and addresses are checked here, and errors are collected.
6. On success `asm_main` writes `hexText()` and `listingText()`. On failure it prints `errorText()` and writes
   nothing. Errors and the symbol table are ordered with `heapSort`.

Assembly is O(n) in the number of source lines, because each line is visited once per pass and every lookup is
O(1) on average.

### Running (`sim`)

1. `sim_main.cpp` reads the `.hex` text and calls `Simulator::loadHexText`, then optionally `loadInterruptText`.
2. `run()` repeats `step()` until `HALT`, a runtime error or the instruction limit. One `step()` is one cycle:
   1. **Interrupt check.** Interrupts whose cycle has come move into the pending-interrupt queue. If `IE = 1`
      and the queue is not empty, one is taken: push `PC`, push the flags word, `IE = 0`, `PC = M[0xFC + n]`.
   2. **Fetch** the word at `PC` from the prefetch buffer.
   3. **Decode** the fields with shifts and masks.
   4. **Execute**, update registers, flags, memory and the counters.
3. The CPU stack lives inside the simulated memory (`SP` and `M[SP]`), not in our `Stack` class, because
   programs can read it with `LW` and overwrite it with `SW`.
4. The **prefetch buffer** is a queue of at most 4 `(address, word)` pairs. It is emptied whenever execution
   does not continue at `PC + 1` (taken branch, `CALL`, `RET`, `RETI`, interrupt entry) and when a `SW` writes
   to an address that is in the buffer. It never changes the result of a program.
5. The debugger and the profiler only read the simulator (`getPc`, `getMemoryWord`, `getExecCount`, ...) and
   call `step()`. They never change the machine directly.

## 7. Data structures

| Structure | Cost | Where it is used |
|---|---|---|
| `DynamicArray` | `pushBack` O(1) amortized, index O(1) | memory and registers of the simulator, source lines, listing entries, error list, execution counters, storage inside `Stack` and `MinHeap` |
| `Stack` | push, pop O(1) | operator and value stacks of the shunting-yard algorithm, the stack of active macro contexts |
| `Queue` | enqueue, dequeue O(1), `contains` and `getAt` O(n) | shunting-yard output queue, pending and incoming interrupts, the 4-word prefetch buffer, one queue per pipeline stage |
| `HashMap` | put, get, remove O(1) average | symbol table, opcode table, macro table |
| `HashSet` | add, contains, remove O(1) average | debugger breakpoints (checked before every instruction) |
| `MinHeap` | insert, extractMin O(log n), peekMin O(1) | profiler (top-K), `heapSort` |
| `heapSort` | O(n log n) | symbol table order in the listing, error messages sorted by line, interrupt file sorted by cycle |

The non-linear structures are the two hash tables (`HashMap`, `HashSet`) and the binary tree `MinHeap`.

**Hash tables.** `HashMap` and `HashSet` use open addressing with double hashing. All entries live in one
array of slots that are `EMPTY`, `USED` or `DELETED`. The probe sequence is
`slot = (start + i * step) mod size`, with `start = hash1(key) mod size` and
`step = 1 + hash2(key) mod (size - 1)`. The table size is always a prime number, so the sequence can reach
every slot. When more than half of the slots are in use (used plus deleted), the table is rebuilt with the
next prime that is at least twice the size. Removing a key leaves a tombstone (`DELETED`) so the keys behind it
can still be found.

**MinHeap.** A binary tree stored in a `DynamicArray`: the parent of index `i` is `(i - 1) / 2` and its
children are `2i + 1` and `2i + 2`. Every parent is smaller than or equal to its children, so the smallest
element is always at index 0. `insert` appends at the end and moves the new item up (`siftUp`). `extractMin`
takes the root, moves the last item to the top and pushes it down (`siftDown`). Both are O(log n). The type
only needs `operator<` and a default constructor.

**heapSort** inserts every element into a `MinHeap` and extracts them back in order. It is O(n log n) in every
case but not stable, so errors and interrupts carry an "order" field to keep ties in file order.

## 8. Extras: macros, profiler, pipeline, benchmark

| Extra | Code | Try it |
|---|---|---|
| Macro expansion (stack of active macro contexts) | `app/Macro.cpp` | `./asm tests/programs/macros.asm && ./sim tests/programs/macros.hex` (prints 5, 3, 30, 25) |
| Profiler: top-K most executed instructions (heap) | `app/Profiler.cpp` | `./sim -p 3 tests/programs/fibonacci.hex` |
| Hash table vs linear list benchmark | `bench/bench_symbols.cpp` | `make bench` |
| 5-stage pipeline with a queue per stage and data hazards | `app/Pipeline.cpp` | `./sim --pipeline-log 12 tests/programs/fibonacci.hex` |

**Macros.** Definitions are stored in a `HashMap`. A call pushes a frame on a `Stack` of active macro
contexts, and the expander always works on the top frame, so macros can call other macros. The stack has a
limit of 16 frames, so a macro that calls itself gives an error instead of looping forever. Errors inside a
body are reported with the line number of the call.

**Profiler.** A `MinHeap` of at most K entries `(count, address)`. Every address that ran is inserted, and when
the heap grows beyond K the smallest entry is removed, so at the end the heap holds the K most executed
addresses. Time is O(n log K) with n = 256 addresses and memory is O(K). On equal counts the lower address is
kept, so the result is always the same.

```
$ ./sim -p 3 tests/programs/fibonacci.hex
...
Top 3 most executed instructions:
03: F042 OUT R1   executed 15 times
04: 0850 ADD R4, R1, R2   executed 15 times
05: 0280 ADD R1, R2, R0   executed 15 times
```

**Pipeline.** The simulator records a trace of the executed instructions and the pipeline replays it one clock
cycle at a time through five stages (IF, ID, EX, MEM, WB), each a queue holding at most one instruction. A
read-after-write hazard stalls the instruction in ID. A taken branch, `CALL`, `RET`, `RETI` or interrupt entry
costs 2 bubble cycles. With `--forwarding` only a load followed immediately by a use still waits. The model
only counts cycles and does not change what the simulator computes. For `fibonacci.asm`:

```
pipeline (5 stages, forwarding off):   cycles 141, CPI 1.50, 15 stall cycles, 28 bubble cycles
pipeline (5 stages, forwarding on):    cycles 126, CPI 1.34, 0 stall cycles, 28 bubble cycles
```

**Benchmark.** Looking up every label once (one example run):

| labels | hash map | linear list |
|---|---|---|
| 1,000 | 0.012 ms | 0.73 ms |
| 5,000 | 0.16 ms | 28.6 ms |
| 20,000 | 0.45 ms | 186 ms |

The hash map grows about linearly with the number of labels and the list about quadratically, which is why the
symbol table is a hash table.

## 9. Testing

`make test` builds and runs two programs.

- `tests/test_ds.cpp` (124 checks) tests every data structure: empty and full cases, growth, duplicates, the
  hash tables with tombstones, the 50% load factor and many adds and removes, the heap with 1,000 scrambled
  values and a top-K example, and heap sort.
- `tests/test_app.cpp` (172 checks) tests the application: the four official programs with exact `OUT` values
  and instruction counts, the exact HALT report, the hex and listing of the worked example from the sheet,
  our own programs, 11 assembler-error programs with their exact messages, 9 runtime-error programs, the
  debugger commands, the profiler, the macro expander and hand-computed pipeline cycle counts.

Both end with `ALL TESTS PASSED` when nothing failed.

`tests/programs/`:

| Program | Expected `OUT` values | Instructions |
|---|---|---|
| `fibonacci.asm` | 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377 | 94 |
| `factorial.asm` | 1, 2, 6, 24, 120, 720, 5040 | 863 |
| `bubble.asm` | -128, -7, 0, 5, 15, 42, 99, 300 | 298 |
| `interrupts.asm` with `interrupts.txt` | 2, 9, 50 | 209 |

Each has a `.expected.txt` next to it. The folder also has our own programs (`features`, `selfmod`, `nested`,
`interrupts2`, `loop_forever`, `macros`), `errors/` with the assembler error programs and `runtime/` with the
runtime error programs.

## 10. Design notes, assumptions and limits

[`docs/DESIGN.md`](docs/DESIGN.md) describes the architecture, every algorithm and the trade-offs in detail,
and lists the assumptions we made where the reference sheet is silent (error output on standard output, the
exact `.lst` columns, what counts as a prefetch flush, exit codes and so on). These should be confirmed with
the TA.

Limits: macros are plain text replacement (no conditional assembly, no default arguments), unary `+` is not
supported, `.SPACE` is limited to 256 words, and the pipeline is a timing model on a trace of the simulator,
not a second simulator.

## 11. Changes since Milestone 1

These follow the professor's comments on Milestone 1:

- **Double hashing instead of separate chaining.** Long linked lists in a hash table mean pointer chasing and
  long chains for big programs. `HashMap` and `HashSet` now keep everything in one array and resolve
  collisions with a second hash function. Table sizes are prime numbers so every slot can be reached.
- **Load factor 50% instead of 75%.** The table is rebuilt as soon as more than half of the slots are used, so
  a lookup needs 1 to 2 probes on average.
- Removing a key leaves a tombstone so other keys can still be found, and the tests were extended for
  tombstones, load factor and many adds and removes.
