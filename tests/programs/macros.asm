; Macro test: expected output 5, 3, 30, 25
        .EQU  A, 5
        .MACRO swap, x, y        ; swaps two registers (R7 is scratch)
            MOV R7, x
            MOV x, y
            MOV y, R7
        .ENDM
        .MACRO showboth, p, q    ; nested: calls swap and prints both
            OUT p
            OUT q
        .ENDM
        .MACRO absval, r         ; r = |r|, uses a unique label per call
            CMP r, R0
            BGE done@@
            SUB r, R0, r
    done@@:
        .ENDM
        .MACRO twice, r          ; a macro that calls another macro
            absval r
            ADD r, r, r
        .ENDM

        LDI  R1, A
        LDI  R2, 3
        swap R1, R2              ; now R1 = 3, R2 = 5
        showboth R2, R1          ; prints 5 then 3
        LDI  R3, -15
        twice R3                 ; |-15| * 2 = 30
        OUT  R3
        LDI  R4, 25
        absval R4                ; stays 25
        OUT  R4
        HALT
