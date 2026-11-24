; loop_forever.asm - never halts. Run with the instruction limit (-l 1000).
start:  INC R1
        JMP start
