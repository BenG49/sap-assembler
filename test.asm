; fib
; x = 0
; y = 0
; z = 1
; while z < 255:
;     print(z)
;     x = z
;     z += y
;     y = x

; x addr = 0
; y addr = 1
; z addr = 2

start:
    lda 0
    sta 0
    sta 1

    lda 1
    sta 2

loop:
    lda [2]
    out     ; print(z)
    sta 0   ; x = z

    add [1] ; z += y

    jc start    ; if z > 255, restart

    sta 2   ; move a reg to z

    lda [0]
    sta 1   ; y = x
