.global _start

.section .text

_start:
    call here
here:
    pop %esi

    # filename -> EBX
    lea filename - here(%esi), %ebx

    # program blob -> ECX
    lea child_program - here(%esi), %ecx

    # total size -> EDX
    mov $67, %edx

    # SYS_WRITE(filename, child_program, 67)
    mov $3, %eax
    int $0x80

    # print success message
    lea msg - here(%esi), %ebx
    mov $1, %eax
    int $0x80

    # exit
    mov $4, %eax
    int $0x80
    ret

.section .data

filename:
    .ascii "generated.kx\0"

msg:
    .ascii "Generated child executable!\n\0"

# KX child file:
# header = 16 bytes
# code payload = 51 bytes
#
# Child program:
#   call here
# here:
#   pop esi
#   lea msg-here(esi), ebx
#   mov eax,1
#   int 0x80
#   mov eax,4
#   int 0x80
#   ret
# msg:
#   "Child says: Hello again!\n\0"

child_program:
    # ---- KX header ----
    .byte 0x4B, 0x58, 0x4B, 0x31      # magic
    .long 0                           # entry
    .long 51                          # code_size
    .long 0                           # data_size

    # ---- child code ----

    # call here
    .byte 0xE8, 0x00, 0x00, 0x00, 0x00

    # pop %esi
    .byte 0x5E

    # lea 0x10(%esi), %ebx
    .byte 0x8D, 0x5E, 0x10

    # mov $1, %eax
    .byte 0xB8, 0x01, 0x00, 0x00, 0x00

    # int $0x80
    .byte 0xCD, 0x80

    # mov $4, %eax
    .byte 0xB8, 0x04, 0x00, 0x00, 0x00

    # int $0x80
    .byte 0xCD, 0x80

    # ret
    .byte 0xC3

    # ---- child message ----
    .ascii "Child says: Hello again!\n\0"
