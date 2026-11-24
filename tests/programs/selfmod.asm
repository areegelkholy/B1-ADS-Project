; selfmod.asm - a store into an address that is waiting in the prefetch
; buffer must flush the buffer, otherwise the OLD instruction would run.
; Expected output: 99   (not 1)
start:  LDI R1, newword
        LW  R2, 0(R1)            ; R2 = the new instruction word
        LDI R3, target
        SW  R2, 0(R3)            ; overwrite the instruction at "target"
target: LDI R4, 1                ; becomes LDI R4, 99
        OUT R4
        HALT
newword: .WORD 0x6863            ; encoding of LDI R4, 99
