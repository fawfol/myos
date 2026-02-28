.section .multiboot
.align 4
.long 0x1BADB002
.long 0x03
.long -(0x1BADB002 + 0x03)

.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

.section .text
.global _start
_start:
mov $stack_top, %esp
call kernel_main
cli
hang:	hlt
jmp hang
