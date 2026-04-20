.global tss_flush
.global jump_usermode

# Load the Task Register
tss_flush:
    mov $0x2B, %ax      # 5th descriptor (index 5 * 8 = 40 = 0x28). RPL=3. 0x28 | 3 = 0x2B
    ltr %ax             # Laod Task Register
    ret

# The magic trick: Pretend we are returning from an interrupt to Ring 3
jump_usermode:
    cli                 #disable interrupts during the switch

    # Load User Data Segment selectors
    mov $0x23, %ax      # User DS (4 * 8 = 32 = 0x20). RPL=3. 0x20 | 3 = 0x23
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    # We need to push the fake interrupt stack frame so 'iret' can pop it
    mov %esp, %eax      # Save current stack pointer

    push $0x23          # Push SS (User Data Segment)
    push %eax           # Push ESP (We keep the same stack for this quick test)
    pushf               # Push EFLAGS
    
    # We must explicitly enable interrupts in the pushed EFLAGS
    pop %eax
    or $0x200, %eax     # Set IF (Interrupt Flag)
    push %eax
    
    push $0x1B          #push CS (User Code Segment: 3 * 8 = 24 = 0x18 | 3 = 0x1B)
    push $1f            #push EIP (Instruction Pointer - jump to label '1')
    
    iret                # return from interrupt... into Ring 3

1:
    # WE ARE NOW IN RING 3!
    ret
