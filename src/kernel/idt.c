#include <stdint.h>
#include "idt.h"
#include "shell.h"
#include "memory.h"
#include "vfs.h"    

struct idt_entry_struct idt_entries[256];
struct idt_ptr_struct   idt_ptr;

typedef struct registers {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

//external assembly functions
extern void idt_flush(uint32_t);
extern void isr33();
extern void isr32();
extern void syscall_handler();

//exception handlers
extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3();
extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr9(); extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();
extern void isr44();


/*helper fcuntion */
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags;
}

/*standard IDT init */
void init_idt() {
    idt_ptr.limit = sizeof(struct idt_entry_struct) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    memset(&idt_entries, 0, sizeof(struct idt_entry_struct) * 256);
    
    //exceptions
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)isr44, 0x08, 0x8E);

    idt_flush((uint32_t)&idt_ptr);
}

/*system call gate init */
void init_syscalls() {
    // 0xEE sets Ring 3 privilege so user programs can use it
    idt_set_gate(0x80, (uint32_t)syscall_handler, 0x08, 0xEE);
}

/*dispatcher bridge */ 
void syscall_dispatcher(registers_t *regs) {
    switch (regs->eax) {
        case 1: //SYS_PRINT
            terminal_print((char*)regs->ebx);
            break;

        case 2: { //SYS_READ: ebx = filename, ecx = destination buffer
            char* name = (char*)regs->ebx;
            void* buffer = (void*)regs->ecx;
            vfs_node_t* file = vfs_find(vfs_root, name);

            if (file) {
                //use your kernel memcpy to move data to the user buffer
                memcpy(buffer, (void*)file->ptr, file->length);
                regs->eax = file->length; // Return size read
            } else {
                regs->eax = 0; // File not found
            }
            break;
        }

        case 3: { //SYS_WRITE: ebx = filename, ecx = data pointer
			char* name = (char*)regs->ebx;
			char* data = (char*)regs->ecx;
			uint32_t size = regs->edx; // Assume size is passed in EDX

			vfs_create(name, data, size);
			break;
		}

        default:
            terminal_print("KalsangOS: Unknown Syscall\n");
            break;
    }
}
