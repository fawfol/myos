.section .text
.align 4
/*manual multiboot header */
.long 0x1BADB002         
.long 0x00000003         /*flags*/
.long 0xE4524FFB         /*checksum*/

.global _start
.global gdt_flush        /*allow C to call this function */

_start:
    mov $stack_top, %esp
    call kernel_main
    cli
1:  hlt
    jmp 1b

/* func : gdt_flush
   loads the GDT pointer and performs a far jump 
   to reload the cs register
*/
gdt_flush:
    mov 4(%esp), %eax    /*gets the pointer to the GDT passed from C*/
    lgdt (%eax)           /*load the GDT into the CPU register*/
    
    /*reload ds registers with the 0x10 offset*/
    mov $0x10, %ax       
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    
    /*perform a far jump to reload the cs (0x08) */
    ljmp $0x08, $.flush_cs

.flush_cs:
    ret

.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:
