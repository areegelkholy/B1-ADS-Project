; features.asm - expressions, directives, pseudo-instructions
; Expected output: 7, 20, 15, 19, 0, 4
        .EQU LEN, 4
        .EQU TWICE, LEN * 2          ; .EQU may use earlier symbols
start:  LDI R1, TWICE - 1            ; 7
        OUT R1
        LDI R2, table                ; address of the table
        LW  R3, 1(R2)                ; table[1] = 20
        OUT R3
        LDI R4, (3 + 4) * 2 - -1     ; unary minus: 15
        OUT R4
        LDI R5, 0x10 + 0b11          ; hex and binary literals: 19
        OUT R5
        LDI R6, 5
        CLR R6                       ; pseudo-instruction: R6 = 0
        OUT R6
        LDI R7, buffer - table           ; distance between two labels = 4
        OUT R7
        HALT
table:  .WORD 10, 20, 30, LEN + TWICE
buffer: .SPACE 3
