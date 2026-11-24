; nested.asm - nested CALL/RET with PUSH/POP and a value kept on the stack
; Expected output: 33, 22, 11
start:  LDI R1, 11
        CALL outer
        OUT R1
        HALT
outer:  PUSH R1
        LDI R1, 22
        CALL inner
        OUT R1
        POP R1
        RET
inner:  PUSH R1
        LDI R1, 33
        OUT R1
        POP R1
        RET
