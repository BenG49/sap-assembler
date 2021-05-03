; comments
loop:
    lda 0x1
    lda [0b11]

    sta 0

    jmp loop
