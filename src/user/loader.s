.global _start

.section .text

_start:
    call here
here:
    pop %esi

    # print: "User-Land: Attempting to read 'test.txt'..."
    lea msg1 - here(%esi), %ebx
    mov $1, %eax              # SYS_PRINT
    int $0x80

    # read_file("test.txt", buffer)
    lea filename - here(%esi), %ebx
    lea buffer - here(%esi), %ecx
    mov $2, %eax              # SYS_READ
    int $0x80

    # eax = bytes read
    cmp $0, %eax
    jle read_failed

    # terminate buffer with '\0'
    lea buffer - here(%esi), %edi
    add %eax, %edi
    movb $0, (%edi)

    # print header
    lea msg2 - here(%esi), %ebx
    mov $1, %eax
    int $0x80

    # print buffer
    lea buffer - here(%esi), %ebx
    mov $1, %eax
    int $0x80

    # print footer newline
    lea msg3 - here(%esi), %ebx
    mov $1, %eax
    int $0x80

    jmp done

read_failed:
    lea msg_err - here(%esi), %ebx
    mov $1, %eax
    int $0x80

done:
    mov $4, %eax              # SYS_EXIT
    int $0x80
    ret

.section .data
msg1:
    .ascii "User-Land: Attempting to read 'test.txt'...\n\0"

msg2:
    .ascii "User-Land: File Content Follows:\n------------------------------\n\0"

msg3:
    .ascii "\n------------------------------\n\0"

msg_err:
    .ascii "User-Land Error: Could not read file.\n\0"

filename:
    .ascii "test.txt\0"

    .align 4
buffer:
    .space 256
