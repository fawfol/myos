#include <stdint.h>
#include <stdbool.h>
#include "idt.h"
#include "shell.h"
#include "memory.h"
#include "vfs.h"   
#include "timer.h"
#include "io.h" 

extern void terminal_print_length(char* str, uint32_t len);
extern void fat32_write_file(char* filename, char* data, uint32_t size);

struct idt_entry_struct idt_entries[256];
struct idt_ptr_struct idt_ptr;


// --- FILE DESCRIPTOR TABLE ---
#define MAX_FDS 32
typedef struct {
    vfs_node_t* node;
    uint32_t offset;
    int in_use;
} file_descriptor_t;

file_descriptor_t process_fd_table[MAX_FDS];
// -----------------------------

typedef struct registers {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

uint32_t current_user_brk = 0;

//external assembly functions
extern uint32_t terminal_index;
extern char chain_program_name[64];
extern void return_to_kernel();
extern volatile bool shell_is_blocking;
extern volatile uint8_t last_char;
extern volatile bool char_available;
extern uint32_t kernel_esp_save;
extern void idt_flush(uint32_t);
extern void isr33();
extern void isr32();
extern void syscall_handler();
extern char keyboard_get_last_char();
extern volatile bool char_available;
extern void return_to_kernel();
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
void init_syscalls() {
    idt_set_gate(0x80, (uint32_t)syscall_handler, 0x08, 0xEF);
    
    //initialize the FD Table
    for(int i = 0; i < MAX_FDS; i++) {
        process_fd_table[i].in_use = 0;
    }
    //reserve POSIX
    process_fd_table[0].in_use = 1; // STDIN  (Keyboard)
    process_fd_table[1].in_use = 1; // STDOUT (Terminal)
    process_fd_table[2].in_use = 1; // STDERR (Terminal Errors)
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
            return_to_kernel();
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
        case 9: { // SYS_INPUTSTR
            char* dest = (char*)regs->ebx;
            int idx = 0;

            shell_is_blocking = true;

            // flush stale key
            while (char_available) {
                keyboard_get_last_char();
            }

            terminal_print("> ");

            while (1) {
                while (!char_available) {
                    // wait
                }

                char c = keyboard_get_last_char();

                if (c == '\n' || c == '\r') {
                    break;
                }

                if (c == '\b') {
                    if (idx > 0) {
                        idx--;
                        dest[idx] = '\0';
                    }
                    continue;
                }

                if (c >= 32 && c <= 126) {
                    if (idx < 127) {
                        dest[idx++] = c;
                        dest[idx] = '\0';

                        char out[2];
                        out[0] = c;
                        out[1] = '\0';
                        terminal_print(out);
                    }
                }
            }

            dest[idx] = '\0';
            terminal_print("\n");

            shell_is_blocking = false;
            regs->eax = 1;
            break;
        }
        case 10: { // SYS_STREQ: ebx = str1, ecx = str2
            char* a = (char*)regs->ebx;
            char* b = (char*)regs->ecx;

            uint32_t equal = 1;
            int i = 0;

            while (1) {
                if (a[i] != b[i]) {
                    equal = 0;
                    break;
                }

                if (a[i] == '\0') {
                    break;
                }

                i++;
            }

            regs->ebx = equal;  // 1 if equal, 0 if not
            regs->eax = 1;
            break;
        }
        case 11: { // SYS_WRITEFILE_STR
            char* filename = (char*)regs->ebx;
            char* content  = (char*)regs->ecx;

            uint32_t len = 0;
            while (content[len] != '\0') {
                len++;
            }

            vfs_create(filename, content, len);
            regs->eax = 1;
            break;
        }
        case 12: { // SYS_READFILE_STR
            char* filename = (char*)regs->ebx;
            char* outbuf   = (char*)regs->ecx;

            vfs_node_t* file = vfs_find(vfs_root, filename);

            if (!file) {
                outbuf[0] = '\0';
                regs->eax = 0;
                break;
            }

            uint32_t len = file->length;
            if (len > 127) len = 127;

            memcpy(outbuf, (char*)file->ptr, len);
            outbuf[len] = '\0';

            regs->eax = 1;
            break;
        }
        case 13: { // SYS_RMFILE
            char* filename = (char*)regs->ebx;

            vfs_delete(filename);

            regs->eax = 1;
            break;
        }
        
        ///////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////for futr//strings///////////////////////////////////
        /////////////////////////////// 14 15 16 17 18 19 /////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////
        
        case 20: { // SYS_OPEN: ebx = filename
            char* name = (char*)regs->ebx;
            vfs_node_t* file = vfs_find(vfs_root, name);
            if (!file) {
                regs->eax = -1; // File not found
                break;
            }
            int fd = -1;
            //find the first free FD slot (starting at 3)
            for (int i = 3; i < MAX_FDS; i++) {
                if (!process_fd_table[i].in_use) {
                    process_fd_table[i].in_use = 1;
                    process_fd_table[i].node = file;
                    process_fd_table[i].offset = 0; //start reading at byte 0
                    fd = i;
                    break;
                }
            }
            regs->eax = fd; //return the FD integer to User Space
            break;
        }
        case 21: { // SYS_CLOSE: ebx = fd
            int fd = regs->ebx;
            if (fd >= 3 && fd < MAX_FDS) {
                process_fd_table[fd].in_use = 0;
                process_fd_table[fd].node = 0;
            }
            regs->eax = 0;
            break;
        }
        case 22: { // SYS_FD_READ: ebx = fd, ecx = buffer, edx = size
            int fd = regs->ebx;
            char* buffer = (char*)regs->ecx;
            uint32_t size = regs->edx;

            if (fd == 0) { 
                // --- STDIN (KEYBOARD) ---
                uint32_t bytes_read = 0;
                
                while (bytes_read < size - 1) {
                    while (!char_available) {
                        // CRITICAL: Syscalls disable interrupts..must briefly 
                        // re-enable them (sti), halt the CPU to save power (hlt), 
                        // and disable them again (cli) when a key wakes us up
                        asm volatile("sti\n\thlt\n\tcli"); 
                    }
                    
                    char c = last_char;
                    char_available = false;

                    if (c == '\n') {
                        terminal_print("\n"); //echo the enter key
                        break;                //stop reading on Enter
                    } else if (c == '\b' && bytes_read > 0) {
                        // Optional: handle backspace (Erase from buffer and screen)
                        bytes_read--;
                        terminal_backspace();
                    }  else if (c >= 32 && c <= 126) { 
                        buffer[bytes_read++] = c;
                        
                        char str[2] = {c, '\0'};
                        terminal_print(str);
                        
                        // Sync the hardware cursor after every character typed in Ring 3
                        update_cursor(terminal_index); 
                    }
                }
                buffer[bytes_read] = '\0'; // Null-terminate the string safely
                regs->eax = bytes_read;    // Return number of bytes typed

            } else if (fd >= 3 && fd < MAX_FDS && process_fd_table[fd].in_use) {
                // --- STANDARD FILE READ ---
                vfs_node_t* file = process_fd_table[fd].node;
                uint32_t available = file->length - process_fd_table[fd].offset;
                uint32_t to_read = (size < available) ? size : available;

                if (to_read > 0) {
                    memcpy(buffer, (uint8_t*)file->ptr + process_fd_table[fd].offset, to_read);
                    process_fd_table[fd].offset += to_read;
                }
                regs->eax = to_read; 
            } else {
                regs->eax = 0; // EOF or Bad FD
            }
            break;
        }
        case 23: { // SYS_FD_WRITE: ebx = fd, ecx = buffer, edx = size
			int fd = regs->ebx;
			char* buffer = (char*)regs->ecx;
			uint32_t size = regs->edx;

			if (fd == 1 || fd == 2) { // STDOUT/STDERR
				terminal_print_length(buffer, size); // Direct to VGA
				regs->eax = size;
			} else if (fd >= 3 && fd < MAX_FDS && process_fd_table[fd].in_use) {
				// Redirect the FD write to the actual FAT32 disk
				vfs_node_t* node = process_fd_table[fd].node;
				fat32_write_file(node->name, buffer, size); 
				regs->eax = size;
			} else {
				regs->eax = -1;
			}
			break;
		}
        case 24: { // SYS_SBRK: ebx = increment
            int increment = regs->ebx;
            
            //if user heap hasnt been initialized, grab a massive 1MB chunk 
            //from the Kernels memory manager to act as the User Heap pool
            if (current_user_brk == 0) {
                current_user_brk = (uint32_t)malloc(1024 * 1024); // 1MB User Heap
                if (!current_user_brk) {
                    regs->eax = -1; //out of memory
                    break;
                }
            }

            //return the OLD break, then increment it (standard sbrk behavior)
            uint32_t old_break = current_user_brk;
            current_user_brk += increment;
            
            regs->eax = old_break;
            break;
        }
        case 25: { // SYS_EXEC: ebx = filename
            char* name = (char*)regs->ebx;
            
            // Copy the requested filename into our global chaining variable
            int i = 0;
            while (name[i] != '\0' && i < 63) {
                chain_program_name[i] = name[i];
                i++;
            }
            chain_program_name[i] = '\0';

            // Teleport back to run_kx_file to clean up and execute the chain!
            return_to_kernel(); 
            break;
        }
        default:
            terminal_print("KalsangOS: Unknown Syscall\n");
            break;
    }
}
