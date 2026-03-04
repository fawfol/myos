.section .text
.align 4
/*manual multiboot header */
.long 0x1BADB002         
.long 0x00000003         /*flags*/
.long 0xE4524FFB         /*checksum*/

.global _start
.global gdt_flush        
.global idt_flush

_start:
    mov $stack_top, %esp
    
    # EBX contains the physical address of the multiboot info structure
    # We push it as an argument for kernel_main(uint32_t mboot_ptr)
    push %ebx 
    
    call kernel_main
1:  hlt
    jmp 1b

/* func : gdt_flush */
gdt_flush:
    mov 4(%esp), %eax    
    lgdt (%eax)           
    
    mov $0x10, %ax       
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    
    ljmp $0x08, $.flush_cs

.flush_cs:
    ret

/* func : idt_flush */
idt_flush:
    mov 4(%esp), %eax
    lidt (%eax)        
    ret

.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:
