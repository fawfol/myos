.global _start

.section .text

_start:
    call get_string
get_string:
    pop %esi

    lea msg - get_string(%esi), %ebx

    mov $1, %eax      # SYS_PRINT
    int $0x80

    mov $4, %eax      # SYS_EXIT
    int $0x80

    ret

msg:
    .ascii "Hello from KalsangOS user program!\n\0"
