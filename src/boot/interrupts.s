.section .text
.extern isr_handler
.extern syscall_dispatcher

.global isr33
.global isr32
.global syscall_handler 

/* keyboard ISR */
isr33:
    cli
    push $0
    push $33
    jmp isr_common_stub

/* timer ISR */
isr32:
    cli
    push $0
    push $32
    jmp isr_common_stub

/* common handler */
isr_common_stub:
    pusha
    mov %ds, %ax
    push %eax

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    call isr_handler

    pop %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    popa
    add $8, %esp
    sti
    iret

/* --- SYSCALL HANDLER --- */
syscall_handler:
    pusha                /*save all gp reg*/
    
    push %ds             /*save ds */
    push %es
    push %fs
    push %gs

    mov $0x10, %ax       /*load ernel ds (0x10) */
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    push %esp            /*push pointer to registers for C dispatcher */
    call syscall_dispatcher
    add $4, %esp         /*clean up the pushed ESP */

    pop %gs              /*restore segments */
    pop %fs
    pop %es
    pop %ds

    popa                 /*retore all registers */
    iret                 /*return to user/shell */

/*exceptions Macros */
.macro ISR_NOERRCODE num
.global isr\num
isr\num:
    cli
    push $0
    push $\num
    jmp isr_common_stub
.endm

.macro ISR_ERRCODE num
.global isr\num
isr\num:
    cli
    push $\num
    jmp isr_common_stub
.endm

/*registered exceptions*/
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
