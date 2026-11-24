; bubble.asm - bubble sort of an 8-element array, then prints it
        .EQU LEN, 8

start:  LDI  R1, LEN - 1    ; comparisons in this pass
outer:  LDI  R3, arr        ; p = &arr[0]
        MOV  R2, R1         ; inner counter
inner:  LW   R4, 0(R3)      ; arr[j]
        LW   R5, 1(R3)      ; arr[j+1]
        CMP  R4, R5
        BLE  noswap
        SW   R5, 0(R3)      ; swap
        SW   R4, 1(R3)
noswap: INC  R3
        DEC  R2
        BNE  inner
        DEC  R1
        BNE  outer

        LDI  R3, arr        ; print the sorted array
        LDI  R2, LEN
print:  LW   R4, 0(R3)
        OUT  R4
        INC  R3
        DEC  R2
        BNE  print
        HALT

arr:    .WORD 42, -7, 300, 0, 15, -128, 99, 5
