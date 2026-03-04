.section .text
.extern isr_handler      /*C function call */

.global isr33            /*shoudlbe visible to idt.c*/
.global isr32

/*stub for the keyboard int 33*/
isr33:
    cli                  /*disable interrupts while we handle this one */
    push $0              /*push dummy error code to keep the stack aligned*/
    push $33             /*push  int number so C knows who called*/
    jmp isr_common_stub

isr32:
    cli                  
    push $0              
    push $32             
    jmp isr_common_stub

/*common handler that saves the CPU state */
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

/*exceptions that dont push error codes (we push a dummy 0)*/
.macro ISR_NOERRCODE num
.global isr\num
isr\num:
    cli
    push $0
    push $\num
    jmp isr_common_stub
.endm

/* Exceptions that DO push error codes */
.macro ISR_ERRCODE num
.global isr\num
isr\num:
    cli
    push $\num
    jmp isr_common_stub
.endm

/*exceptions */
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31
