.686P
.model  flat, c

.code

__bochsbreak proc
    xchg    bx, bx          ; bochs magic breakpoint.
    ret
__bochsbreak endp


end
