.686P
.model  flat, c

cpuid_result struct
    res_a   dd  ?
    res_b   dd  ?
    res_c   dd  ?
    res_d   dd  ?
cpuid_result ends

.code
__cpuidex proc uses eax ebx ecx edx, in_a:dword, in_c:dword, result:ptr
    mov     eax, [in_a]
    mov     ecx, [in_c]
    cpuid
    mov     (cpuid_result ptr [result]).res_a, eax
    mov     (cpuid_result ptr [result]).res_b, ebx
    mov     (cpuid_result ptr [result]).res_c, ecx
    mov     (cpuid_result ptr [result]).res_d, edx
    ret
__cpuidex endp

__cpuid proc, in_a:dword, result:ptr
    invoke __cpuidex, in_a, 0, result
    ret
__cpuid endp


__inb proc uses edx, port:word
    mov     dx, [port]
    in      al, dx
    ret
__inb endp

__inw proc uses edx, port:word
    mov     dx, [port]
    in      ax, dx
    ret
__inw endp

__indw proc uses edx, port:word
    mov     dx, [port]
    in      eax, dx
    ret
__indw endp


__outb proc uses eax edx, port:word, value:byte
    mov     dx, [port]
    mov     al, [value]
    out     dx, al
    ret
__outb endp

__outw proc uses eax edx, port:word, value:word
    mov     dx, [port]
    mov     ax, [value]
    out     dx, ax
    ret
__outw endp

__outdw proc uses eax edx, port:word, value:dword
    mov     dx, [port]
    mov     eax, [value]
    out     dx, eax
    ret
__outdw endp


__bochsbreak proc
    xchg    bx, bx          ; bochs magic breakpoint.
;;    int     3
    ret
__bochsbreak endp


abs proc uses edx, value:dword
    mov     eax, value
    cdq
    xor     eax, edx
    sub     eax, edx
    ret
abs endp

strlen proc uses ecx edi, psz:ptr
    cld                     ; make sure we scan in the right direction.
    xor     eax, eax        ; eax = 0
    xor     ecx, ecx        ; ecx = 0
    not     ecx             ; ecx = 0xFFFFFFFF
    mov     edi, psz        ; edi = psz
    repne scasb             ; search for the 0-terminator
    not     ecx             ; compute the number of chars it advanced based on the counter
    lea     eax, [ecx-1]    ;  /
    ret
strlen endp

end
