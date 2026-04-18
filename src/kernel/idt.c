#include <stdint.h>
#include <stdbool.h>
#include "idt.h"
#include "shell.h"
#include "memory.h"
#include "vfs.h"   
#include "timer.h"
#include "io.h" 

struct idt_entry_struct idt_entries[256];
struct idt_ptr_struct   idt_ptr;

typedef struct registers {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

extern volatile bool shell_is_blocking;

//external assembly functions
extern void idt_flush(uint32_t);
extern void isr33();
extern void isr32();
extern void syscall_handler();
extern char keyboard_get_last_char();
extern volatile bool char_available;

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
    // butttttt 0xEE = ring 3 interrupt gatec // 0xEF = ring 3 trap gate
    idt_set_gate(0x80, (uint32_t)syscall_handler, 0x08, 0xEF);
}

/*dispatcher bridge */ 
void syscall_dispatcher(registers_t *regs) {
    switch (regs->eax) {
        case 1: // SYS_PRINT
            terminal_print((char*)regs->ebx);
            break;
        case 2: { // SYS_READ: ebx = filename, ecx = destination buffer
            char* name = (char*)regs->ebx;
            void* buffer = (void*)regs->ecx;
            vfs_node_t* file = vfs_find(vfs_root, name);
            if (file) {
                memcpy(buffer, (void*)file->ptr, file->length);
                regs->eax = file->length; // return size read
            } else {
                regs->eax = 0; // file not found
            }
            break;
        }
        case 3: { // SYS_WRITE: ebx = filename, ecx = data pointer, edx = size
            char* name = (char*)regs->ebx;
            char* data = (char*)regs->ecx;
            uint32_t size = regs->edx;
            vfs_create(name, data, size);
            regs->eax = 1;
            break;
        }
        case 4: { // SYS_EXIT
			terminal_print("\n[Program exited]\n");
			break;
		}
		case 5: { // SYS_SLEEP: ebx = seconds
			uint32_t seconds = regs->ebx;
			sleep(seconds);
			regs->eax = 1;
			break;
		}
		case 6: { // SYS_BEEP
			beep(750, 200);
			regs->eax = 1;
			break;
		}
		case 7: { // SYS_PRINT_NUM: ebx = number
            terminal_print_number(regs->ebx);
            regs->eax = 1;
            break;
        }
        case 8: { // SYS_INPUTNUM
            char buf[32];
            int idx = 0;

            for (int i = 0; i < 32; i++) {
                buf[i] = 0;
            }

            // stop shell from also handling these keys
            shell_is_blocking = true;

            // flush any stale key left over (especially Enter used to launch the program)
            while (char_available) {
                keyboard_get_last_char();
            }

            terminal_print("> ");

            while (1) {
                while (!char_available) {
                    // wait for a key
                }

                char c = keyboard_get_last_char();

                if (c == '\n' || c == '\r') {
                    if (idx == 0) {
                        // ignore empty newline until user actually types digits
                        continue;
                    }
                    break;
                }

                if (c == '\b') {
                    if (idx > 0) {
                        idx--;
                    }
                    continue;
                }

                if (c >= '0' && c <= '9') {
                    if (idx < 31) {
                        buf[idx++] = c;

                        char out[2];
                        out[0] = c;
                        out[1] = '\0';
                        terminal_print(out);
                    }
                }
            }

            terminal_print("\n");

            uint32_t value = 0;
            for (int i = 0; i < idx; i++) {
                value = value * 10 + (buf[i] - '0');
            }

            shell_is_blocking = false;

            regs->ebx = value;
            regs->eax = 1;
            break;
        }
        default:
            terminal_print("KalsangOS: Unknown Syscall\n");
            break;
    }
}
