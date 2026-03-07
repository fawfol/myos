#include <stdint.h>
#include <stdbool.h>
#include "io.h" 
#include "shell.h" 
#include "timer.h"

volatile char last_char = 0;
volatile bool char_available = false;
volatile bool shell_is_blocking = false;


char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point",
    "Virtualization",
    "Control Protection",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"
};

typedef struct {
    uint32_t ds;                                     
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; 
    uint32_t int_no, err_code;                       
    uint32_t eip, cs, eflags, useresp, ss;           
} registers_t;

//track if shift key is currently being held down
bool shift_pressed = false;

//standard US QWERTY map lowercase
const char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',   
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,   
  '*',    0,  ' ',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   
    0,    0,   0,   0,   0,   0, '-',   0,   0,   0, '+',   0,   0,   
    0,    0,   0,   0,   0,   0,   0,   0,   0   
};

//shifted US QWERTY map uppercase and symbols
const char kbd_us_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',   
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',   
    0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',   
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0,   
  '*',    0,  ' ',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   
    0,    0,   0,   0,   0,   0, '-',   0,   0,   0, '+',   0,   0,   
    0,    0,   0,   0,   0,   0,   0,   0,   0   
};


//helper for the shell to "consume" a key
char keyboard_get_last_char() {
    if (!char_available) return 0;
    char c = last_char;
    char_available = false; //reset the flag
    return c;
}

void isr_handler(registers_t regs) {
    if (regs.int_no < 32) {
        terminal_clear();
        terminal_print("Exception: ");
        terminal_print(exception_messages[regs.int_no]);
        
        if (regs.int_no == 14) {
            uint32_t faulting_address;
            asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
            terminal_print("\nType: PAGE FAULT");
            terminal_print("\nFaulting Address: ");
            terminal_print_number(faulting_address);
        }
        
        terminal_print("\nEIP: "); 
        terminal_print_number(regs.eip);
        
        //system stops here
        asm volatile("cli; hlt"); 
    } 
    else {    
        // === TIMER INTERRUPT ===
        if (regs.int_no == 32) {
            timer_handler();
            outb(0x20, 0x20);
        }
        // === KEYBOARD INTERRUPT ===
        // Inside isr.c keyboard section
		else if (regs.int_no == 33) {
            uint8_t scancode = inb(0x60);
            outb(0x20, 0x20); // Send EOI

            // 1. Detect Shift Key Press
            if (scancode == 0x2A || scancode == 0x36) {
                shift_pressed = true;
            } 
            // 2. Detect Shift Key Release
            else if (scancode == 0xAA || scancode == 0xB6) {
                shift_pressed = false;
            } 
            // 3. Process the actual keydown event
            else if (!(scancode & 0x80)) {
                // Apply the shift map if shift is currently held
                char c = shift_pressed ? kbd_us_shift[scancode] : kbd_us[scancode];

                if (c != 0) {
                    last_char = c;
                    char_available = true;

                    // Pass to the main shell ONLY if we aren't in askname/edit
                    if (!shell_is_blocking) {
                        shell_handle_keypress(c);
                    }
                }
            }
        }
    }
}
