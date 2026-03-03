#include <stdint.h>
#include "io.h" 

typedef struct {
    uint32_t ds;                                     
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; 
    uint32_t int_no, err_code;                       
    uint32_t eip, cs, eflags, useresp, ss;           
} registers_t;

//terminal state
uint32_t terminal_index = 160; 
uint16_t* terminal_buffer = (uint16_t*) 0xB8000;

//keyboard buffer state
#define BUFFER_SIZE 256
char key_buffer[BUFFER_SIZE];
int key_index = 0;

//US QWERTY Map
const char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',   
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,   
  '*',    0,  ' ',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   
    0,    0,   0,   0,   0,   0, '-',   0,   0,   0, '+',   0,   0,   
    0,    0,   0,   0,   0,   0,   0,   0,   0   
};

// --- Custom C LibFunc ---

//cpmpare two strings returns 0 for match
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

//print a string to the screen
void terminal_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            terminal_index = terminal_index + 80 - (terminal_index % 80);
        } else {
            terminal_buffer[terminal_index++] = (uint16_t) str[i] | (uint16_t) 0x0F << 8;
        }
        if (terminal_index >= 2000) terminal_index = 0;
    }
}

//clear the entire screen
void terminal_clear() {
    for (int i = 0; i < 2000; i++) {
        terminal_buffer[i] = (uint16_t) ' ' | (uint16_t) 0x0F << 8;
    }
    terminal_index = 0;
}

// --- shell logic ---

void execute_command() {
    //drop down a line before printing the output
    terminal_index = terminal_index + 80 - (terminal_index % 80);

    //terminate the string so strcmp knows where it ends
    key_buffer[key_index] = '\0'; 

    if (key_index == 0) {
        //user just pressed enter without typing anything
    } 
    else if (strcmp(key_buffer, "help") == 0) {
        terminal_print("TenzinOs Commands: \n");
        terminal_print("- help : Shows this menu\n");
        terminal_print("- clear: Clears the screen");
    } 
    else if (strcmp(key_buffer, "clear") == 0) {
        terminal_clear();
    } 
    else {
        terminal_print("Unknown command: ");
        terminal_print(key_buffer);
    }

    //reset buffer for the next command
    key_index = 0;
    for(int i = 0; i < BUFFER_SIZE; i++) key_buffer[i] = 0;
    
    // Print the prompt again
    terminal_print("\nTenzinOs> ");
}

// --- int handler ---

void isr_handler(registers_t regs) {
    if (regs.int_no == 33) {
        uint8_t scancode = inb(0x60);

        if (!(scancode & 0x80)) {
            char c = kbd_us[scancode];
            
            if (c != 0) {
                //BACKSPACE
                if (c == '\b') {
                    if (key_index > 0) {
                        key_index--; //remove from buffer
                        terminal_index--; //move cursor back
                        terminal_buffer[terminal_index] = (uint16_t) ' ' | (uint16_t) 0x0F << 8; 
                    }
                } 
                //ENTER
                else if (c == '\n') {
                    execute_command();
                } 
                //NORMAL CHARACTER
                else {
                    if (key_index < BUFFER_SIZE - 1) {
                        key_buffer[key_index++] = c; //save to buffer
                        terminal_buffer[terminal_index++] = (uint16_t) c | (uint16_t) 0x0F << 8; //print
                    }
                }

                if (terminal_index >= 2000) terminal_index = 0;
            }
        }
        outb(0x20, 0x20); //send EOI
    }
}
