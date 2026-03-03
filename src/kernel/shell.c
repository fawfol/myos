#include <stdint.h>
#include "io.h"
#include "shell.h"

uint16_t* terminal_buffer = (uint16_t*) 0xB8000;
uint32_t terminal_index = 0;

//active terminal color default: 0x0F = white text on black background
uint8_t current_color = 0x0F; 

#define BUFFER_SIZE 256
char key_buffer[BUFFER_SIZE];
int key_index = 0;

// === HARDWARE CURSOR ===
void update_cursor(int index) {
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (index & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((index >> 8) & 0xFF));
}

// === C LIB HELPERS ===
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

//compares only the first n characters useful for commands with argument
int strncmp(const char *s1, const char *s2, int n) {
    while (n && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }
    if (n == 0) return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

void terminal_clear() {
    for (int i = 0; i < 2000; i++) {
        //now uses current_color instead of hardcoded white
        terminal_buffer[i] = (uint16_t) ' ' | (uint16_t) current_color << 8;
    }
    terminal_index = 0;
    update_cursor(terminal_index);
}

void terminal_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            terminal_index = terminal_index + 80 - (terminal_index % 80);
        } else {
            //now uses current_color instead of hardcoded white
            terminal_buffer[terminal_index++] = (uint16_t) str[i] | (uint16_t) current_color << 8;
        }
        if (terminal_index >= 2000) terminal_clear();
    }
    update_cursor(terminal_index);
}

// === SHELL LOGIC ===
void execute_command() {
    terminal_print("\n");
    key_buffer[key_index] = '\0'; 

    if (key_index == 0) {
        //empty enter press
    } else if (strcmp(key_buffer, "help") == 0) {
        terminal_print("KalsangOS Commands: \n");
        terminal_print("- help : Shows this menu\n");
        terminal_print("- clear: Clears the screen\n");
        terminal_print("- color: Changes text color (red, green, blue, matrix, white)\n");
    } else if (strcmp(key_buffer, "clear") == 0) {
        terminal_clear();
        terminal_print("KalsangOS> ");
        return; 
    } 
    // === NEW COLOR COMMAND LOGIC ===
    else if (strncmp(key_buffer, "color ", 6) == 0) {
        //arg points to the text exactly after color
        const char* arg = key_buffer + 6; 
        
        if (strcmp(arg, "red") == 0) current_color = 0x0C;       //light Red
        else if (strcmp(arg, "green") == 0) current_color = 0x0A; //light Green
        else if (strcmp(arg, "blue") == 0) current_color = 0x09;  //light Blue
        else if (strcmp(arg, "white") == 0) current_color = 0x0F; //bright White
        else if (strcmp(arg, "matrix") == 0) {
            current_color = 0x02; //dark green
            terminal_clear();     //clear screen for full effect
        }
        else {
            terminal_print("Unknown color. Try: red, green, blue, white, matrix\n");
        }
    } 
    else {
        terminal_print("Unknown command: ");
        terminal_print(key_buffer);
        terminal_print("\n");
    }

    key_index = 0;
    for(int i = 0; i < BUFFER_SIZE; i++) key_buffer[i] = 0;
    
    terminal_print("KalsangOS> ");
}

void shell_handle_keypress(char c) {
    if (c == '\b') {
        if (key_index > 0) {
            key_index--; 
            terminal_index--; 
            terminal_buffer[terminal_index] = (uint16_t) ' ' | (uint16_t) current_color << 8; 
            update_cursor(terminal_index);
        }
    } 
    else if (c == '\n') {
        execute_command();
    } 
    else {
        if (key_index < BUFFER_SIZE - 1) {
            key_buffer[key_index++] = c; 
            terminal_buffer[terminal_index++] = (uint16_t) c | (uint16_t) current_color << 8; 
            update_cursor(terminal_index);
        }
    }
    if (terminal_index >= 2000) terminal_clear();
}

void init_shell() {
    terminal_clear();
    terminal_print("KalsangOS:    Hardware Ints Online\n");
    terminal_print("KalsangOS> ");
}
