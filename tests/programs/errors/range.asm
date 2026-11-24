start: LDI R1, 300
       ADDI R1, R1, 40
       LW R2, 40(R1)
       JMP 300
       .WORD 70000
       HALT
