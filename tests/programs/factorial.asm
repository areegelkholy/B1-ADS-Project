; factorial.asm - prints 1! .. 7! using a recursive subroutine
; Uses CALL/RET, PUSH/POP and a multiply subroutine (TISA-16 has no MUL).
        .EQU N, 7

start:  LDI  R1, 1          ; i = 1
loop:   MOV  R2, R1         ; argument n = i
        CALL fact           ; R3 = fact(n)
        OUT  R3
        INC  R1
        LDI  R4, N
        CMP  R1, R4
        BLE  loop           ; while i <= N
        HALT

; fact: R3 = R2!   (R2 preserved)
fact:   PUSH R2
        LDI  R4, 1
        CMP  R2, R4
        BGT  recur          ; n > 1 -> recursive case
        LDI  R3, 1          ; base case: 0! = 1! = 1
        POP  R2
        RET
recur:  DEC  R2
        CALL fact           ; R3 = (n-1)!
        INC  R2             ; R2 = n again
        MOV  R5, R3
        MOV  R6, R2
        CALL mul            ; R3 = (n-1)! * n
        POP  R2
        RET

; mul: R3 = R5 * R6   (R6 >= 0, R6 preserved)
mul:    PUSH R6
        CLR  R3
mloop:  CMP  R6, R0
        BEQ  mdone
        ADD  R3, R3, R5
        DEC  R6
        JMP  mloop
mdone:  POP  R6
        RET
