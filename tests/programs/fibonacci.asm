; fibonacci.asm - prints the first 15 Fibonacci numbers
        .EQU COUNT, 15

start:  CLR  R1             ; a = 0
        LDI  R2, 1          ; b = 1
        LDI  R3, COUNT      ; counter
loop:   OUT  R1
        ADD  R4, R1, R2     ; t = a + b
        MOV  R1, R2         ; a = b
        MOV  R2, R4         ; b = t
        DEC  R3             ; sets Z when counter reaches 0
        BNE  loop
        HALT
