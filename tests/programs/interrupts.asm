; interrupts.asm - main loop counts to 50; interrupt line 0 prints the counter
start:  CLR  R1
        EI                  ; enable interrupts
loop:   INC  R1
        LDI  R2, 50
        CMP  R1, R2
        BLT  loop
        DI
        OUT  R1
        HALT

isr0:   OUT  R1             ; interrupt service routine
        RETI

        .ORG 0xFC           ; interrupt vector table
        .WORD isr0, isr0, isr0, isr0
