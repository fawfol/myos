#include <stdint.h>
#include "io.h" // We need this to use inb() and outb()

typedef struct {
    uint32_t ds;                                     
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; 
    uint32_t int_no, err_code;                       
    uint32_t eip, cs, eflags, useresp, ss;           
} registers_t;

//global tracker for where the next letter should be printed
uint32_t terminal_index = 160; 

//scan Code Set 1 (US QWERTY)
//maps the raw hardware integers to their ASCII characters.
const char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',   
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,   
  '*',    0,  ' ',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   
    0,    0,   0,   0,   0,   0, '-',   0,   0,   0, '+',   0,   0,   
    0,    0,   0,   0,   0,   0,   0,   0,   0   
};

void isr_handler(registers_t regs) {
    
    //check if interrupt is from keyboard (IRQ 1 -> Int 33)
    if (regs.int_no == 33) {
        
        // 1. read raw Scan Code from the keyboard's data port
        uint8_t scancode = inb(0x60);

        // 2. determine if its a make code (key pressed) or break code (key released)
        // if highest bit (0x80) is 0, the key was pressed down
        if (!(scancode & 0x80)) {
            char c = kbd_us[scancode];
            
            // 3. if its a printable charactr then put it on the screen
            if (c != 0) {
                uint16_t* terminal_buffer = (uint16_t*) 0xB8000;
                
                //print the character in white text (0x0F)
                terminal_buffer[terminal_index] = (uint16_t) c | (uint16_t) 0x0F << 8;
                
                //move the cursor forward
                terminal_index++;
            }
        }

        // 4. send the eoi signal to the Master PIC (Port 0x20)
        // tells hardware for next key press
        outb(0x20, 0x20);
    }
}
