.global tss_flush
.global execute_ring3
.global kernel_esp_save
.extern set_kernel_stack

# Load the Task Register
tss_flush:
    mov $0x2B, %ax      # 5th descriptor (index 5 * 8 = 40). RPL=3. 0x28 | 3 = 0x2B
    ltr %ax             
    ret

.section .bss
kernel_esp_save:
    .long 0

.section .text
# void execute_ring3(uint32_t entry_point, uint32_t user_stack);
execute_ring3:
    push %ebp
    mov %esp, %ebp

    # Save kernel state for when SYS_EXIT returns
    push %ebx
    push %esi
    push %edi

    # Save the current kernel ESP so SYS_EXIT knows where to teleport
    mov %esp, (kernel_esp_save)

    # IMPORTANT: Call the C function FIRST before loading arguments!
    push %esp
    call set_kernel_stack
    add $4, %esp

    # NOW it is safe to read our arguments from C
    mov 8(%ebp), %eax    # %eax = entry_point
    mov 12(%ebp), %ecx   # %ecx = user_stack

    cli                  # Disable interrupts while setting up the jump

    # Load User Data Segment selectors
    mov $0x23, %dx      
    mov %dx, %ds
    mov %dx, %es
    mov %dx, %fs
    mov %dx, %gs

    # Forge the Interrupt Stack Frame for IRET
    push $0x23           # SS
    push %ecx            # ESP
    pushf                # EFLAGS
    
    pop %edx
    or $0x200, %edx
    push %edx
    
    push $0x1B           # CS
    push %eax            # EIP

    iret                 # BLAST OFF TO RING 3!
    
.global return_to_kernel

return_to_kernel:
    # Ensure Kernel Data Segments are safely loaded
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    # Teleport back to the saved kernel stack pointer
    mov (kernel_esp_save), %esp
    
    # Restore the callee-saved registers exactly as we pushed them
    pop %edi
    pop %esi
    pop %ebx
    pop %ebp
    
    # Execute the return. Because we restored ESP, this returns directly to run_kx_file!
    ret
