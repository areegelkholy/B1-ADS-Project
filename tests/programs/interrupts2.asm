; interrupts2.asm - two interrupts arrive in the same cycle (file: interrupts2.txt)
; Line 0 is served first. While its handler runs IE = 0, so line 1 waits in the
; pending queue and is served after RETI.
; Expected output: 100, 200, 30
start:  CLR R1
        EI
loop:   INC R1
        LDI R2, 30
        CMP R1, R2
        BLT loop
        DI
        OUT R1
        HALT
h0:     LDI R6, 100
        OUT R6
        RETI
h1:     LDI R6, 200
        OUT R6
        RETI
        .ORG 0xFC                ; vector table
        .WORD h0, h1, h0, h1
