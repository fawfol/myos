.global _start

.section .text

_start:
    call here
here:
    pop %esi

    # filename
    lea filename - here(%esi), %ebx

    # content
    lea content - here(%esi), %ecx

    mov $13, %edx

    # SYS_WRITE
    mov $3, %eax
    int $0x80

    # print confirmation
    lea msg - here(%esi), %ebx
    mov $1, %eax
    int $0x80

    # exit
    mov $4, %eax
    int $0x80
    ret

.section .data

filename:
    .ascii "user.txt\0"

content:
    .ascii "Hello_From_OS"

msg:
    .ascii "User program created file!\n\0"
