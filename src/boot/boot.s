.section .text
.align 4
/*manual multiboot header */
.long 0x1BADB002         
.long 0x00000003         /*flags */
.long 0xE4524FFB         /*checksum (manual result of -(0x1BADB002 + 0x03)) */

.global _start
_start:
    mov $stack_top, %esp
    call kernel_main
    cli
1:  hlt
    jmp 1b

.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:
