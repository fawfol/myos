.section .text
.extern isr_handler      /*C function call */

.global isr33            /*shoudlbe visible to idt.c*/

/*stub for the keyboard int 33*/
isr33:
    cli                  /*disable interrupts while we handle this one */
    push $0              /*push dummy error code to keep the stack aligned*/
    push $33             /*push  int number so C knows who called*/
    jmp isr_common_stub

/* The common handler that saves the CPU state */
isr_common_stub:
    pusha                /*pushes edi,esi,ebp,esp,ebx,edx,ecx,eax */
    
    mov %ds, %ax         /*lower 16-bits of eax = ds. */
    push %eax            /*save ds descriptor */

    mov $0x10, %ax       /*load the kernel data segment descriptor from gdt */
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    call isr_handler     /*call C code*/

    pop %eax             /*relaod original ds descpt */
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    popa                 /*pops edi,esi,ebp back into their registers*/
    add $8, %esp         /*cleans up the pushed error code and isr num*/
    sti                  /*re enable int*/
    iret                 /*pops CS, EIP, EFLAGS, SS, and ESP automatically*/
