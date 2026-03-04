#include <stdint.h>
#include "io.h"
#include "shell.h"
#include "timer.h"
#include "memory.h"

uint16_t* terminal_buffer = (uint16_t*) 0xB8000;
uint32_t terminal_index = 0;
uint8_t current_color = 0x0F; 

#define BUFFER_SIZE 256
char key_buffer[BUFFER_SIZE];
int key_index = 0;

//force a Division by Zero (Exception 0)
void trigger_divide_by_zero() {
    int a = 5;
    int b = 0;
    int c = a / b; //trigger the exception
    terminal_print_number(c); 
}

//force a Page Fault (Exception 14)
void trigger_page_fault() {
    uint32_t *ptr = (uint32_t*)0xDEADBEEF; //unmapped address
    uint32_t do_fault = *ptr;              //trying to read it
    terminal_print_number(do_fault);
}

// === HARDWARE CURSOR ===
void update_cursor(int index) {
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (index & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((index >> 8) & 0xFF));
}

// === C LIB HELPERS ===
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char *s1, const char *s2, int n) {
    while (n && *s1 && (*s1 == *s2)) { ++s1; ++s2; --n; }
    if (n == 0) return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

void terminal_clear() {
    uint16_t blank = (uint16_t) ' ' | (uint16_t) current_color << 8;
    memset16(terminal_buffer, blank, 2000); 
    terminal_index = 0;
    update_cursor(terminal_index);
}

void terminal_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            terminal_index = terminal_index + 80 - (terminal_index % 80);
        } else {
            terminal_buffer[terminal_index++] = (uint16_t) str[i] | (uint16_t) current_color << 8;
        }
        if (terminal_index >= 2000) terminal_clear();
    }
    update_cursor(terminal_index);
}

//helper to print numbers (needed for uptime)
void terminal_print_number(uint32_t num) {
    if (num == 0) {
        terminal_print("0");
        return;
    }
    char buf[16];
    int i = 0;
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    char str[16];
    int j = 0;
    while (i > 0) {
        str[j++] = buf[--i];
    }
    str[j] = '\0';
    terminal_print(str);
}

uint32_t string_to_int(const char* str) {
    uint32_t res = 0;
    for (int i = 0; str[i] != '\0'; ++i) {
        if (str[i] >= '0' && str[i] <= '9') {
            res = res * 10 + str[i] - '0';
        } else {
            break; 
        }
    }
    return res;
}

// === SHELL LOGIC ===
void execute_command() {
    terminal_print("\n");
    key_buffer[key_index] = '\0'; 

    if (key_index == 0) {
        //empty enter press
    } 
    else if (strcmp(key_buffer, "help") == 0) {
        terminal_print("KalsangOS Commands: \n");
        terminal_print("- help    : Shows this menu\n");
        terminal_print("- clear   : Clears the screen\n");
        terminal_print("- color   : Changes text color (red, green, blue, matrix, white)\n");
        terminal_print("- sysinfo : Displays hardware information\n");
        terminal_print("- uptime  : Shows how long the OS has been running\n");
        terminal_print("- reboot  : Restarts the computer\n");
    } 
    else if (strcmp(key_buffer, "clear") == 0) {
        terminal_clear();
        terminal_print("KalsangOS> ");
        return; 
    } 
    else if (strncmp(key_buffer, "color ", 6) == 0) {
        const char* arg = key_buffer + 6; 
        if (strcmp(arg, "red") == 0) current_color = 0x0C;       
        else if (strcmp(arg, "green") == 0) current_color = 0x0A; 
        else if (strcmp(arg, "blue") == 0) current_color = 0x09;  
        else if (strcmp(arg, "white") == 0) current_color = 0x0F; 
        else if (strcmp(arg, "matrix") == 0) {
            current_color = 0x02; 
            terminal_clear();     
        } else {
            terminal_print("Unknown color. Try: red, green, blue, white, matrix\n");
        }
    } 
    else if (strcmp(key_buffer, "testcrash") == 0) {
		terminal_print("Triggering Division by Zero...\n");
		trigger_divide_by_zero();
	}
    // === DYNAMIC MEMORY ECHO COMMAND ===
    else if (strncmp(key_buffer, "echo ", 5) == 0) {
        //point arg to the text immediately following echo
        const char* arg = key_buffer + 5; 
        
        //calculate the exact length of the users sentence
        int len = 0;
        while (arg[len] != '\0') {
            len++;
        }
        
        //ask heap for exactly 'len + 1' bytes of RAM for '\o' null termintor
        char* dynamic_string = (char*) malloc(len + 1);
        
        if (dynamic_string != NULL) {
            //copy string character by character into our newly allocated heap memory
            for (int i = 0; i <= len; i++) {
                dynamic_string[i] = arg[i];
            }
            
            //print it back to the screen
            terminal_print(dynamic_string);
            terminal_print("\n");
            
            //CRITICAL = give memory back so the OS doesnt run out of RAM
            free(dynamic_string);
        } else {
            terminal_print("ERROR: Heap Out of Memory!\n");
        }
    }
    else if (strcmp(key_buffer, "sysinfo") == 0) {
        terminal_print("KalsangOS System Information\n");
        terminal_print("----------------------------\n");
        terminal_print("OS Name   : KalsangOS v0.1\n");
        terminal_print("Kernel    : 32-bit Custom\n");
        
        uint32_t eax, ebx, ecx, edx;
        uint32_t code = 0; 
        __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(code));

        char vendor[13];
        vendor[0] = (ebx >> 0)  & 0xFF; vendor[1] = (ebx >> 8)  & 0xFF;
        vendor[2] = (ebx >> 16) & 0xFF; vendor[3] = (ebx >> 24) & 0xFF;
        vendor[4] = (edx >> 0)  & 0xFF; vendor[5] = (edx >> 8)  & 0xFF;
        vendor[6] = (edx >> 16) & 0xFF; vendor[7] = (edx >> 24) & 0xFF;
        vendor[8]  = (ecx >> 0)  & 0xFF; vendor[9]  = (ecx >> 8)  & 0xFF;
        vendor[10] = (ecx >> 16) & 0xFF; vendor[11] = (ecx >> 24) & 0xFF;
        vendor[12] = '\0'; 

        terminal_print("Processor : ");
        terminal_print(vendor);
        terminal_print("\n");
    } 
    else if (strcmp(key_buffer, "uptime") == 0) {
        //bring in the global timer variable from timer.c
        extern uint32_t timer_ticks;
        uint32_t seconds = timer_ticks / 100;
        
        terminal_print("KalsangOS Uptime: ");
        terminal_print_number(seconds);
        terminal_print(" seconds\n");
    } 
    else if (strcmp(key_buffer, "reboot") == 0) {
        terminal_print("Rebooting KalsangOS...\n");
        outb(0x64, 0xFE);
        asm volatile("cli");
        asm volatile("hlt");
    } 
    else if (strncmp(key_buffer, "sleep ", 6) == 0) {
        const char* arg = key_buffer + 6; 
        uint32_t sec = string_to_int(arg);
        
        if (sec > 0) {
            terminal_print("Sleeping: ");
            
            uint32_t saved_index = terminal_index; 
            
            for (uint32_t i = sec; i > 0; i--) {
                terminal_index = saved_index; 
                
                terminal_print("          "); 
                
                terminal_index = saved_index; 
                
                terminal_print_number(i);
                terminal_print("s");
                sleep(1);
            }
        } else {
            terminal_print("Usage: sleep <seconds> (e.g., sleep 3)\n");
        }
    }
    else if (strcmp(key_buffer, "malloc") == 0) {
        terminal_print("Testing Dynamic Memory...\n");
        
        //ask the OS for 10 bytes of RAM on the fly
        char* dynamic_string = (char*) malloc(10);
        
        if (dynamic_string != NULL) {
            //write data into our newly acquired RAM
            dynamic_string[0] = 'H';
            dynamic_string[1] = 'e';
            dynamic_string[2] = 'l';
            dynamic_string[3] = 'l';
            dynamic_string[4] = 'o';
            dynamic_string[5] = '\0';
            
            terminal_print("Stored in Heap: ");
            terminal_print(dynamic_string);
            terminal_print("\n");
            
            //give RAM back to the OS
            free(dynamic_string);
            terminal_print("Memory freed successfully\n");
        } else {
            terminal_print("ERROR: Out of memory!\n");
        }
    }
    // === MEMSTAT COMMAND ===
    else if (strcmp(key_buffer, "memstat") == 0) {
        uint32_t mem_used = 0;
        uint32_t mem_free = 0;
        
        get_mem_stats(&mem_used, &mem_free);
        
        terminal_print("KalsangOS Memory Statistics:\n");
        terminal_print("----------------------------\n");
        
        terminal_print("Heap Total: ");
        //heap total HEAP_SIZE (2MB) 2097152
        terminal_print_number(2097152); 
        terminal_print(" bytes\n");
        
        terminal_print("Heap Used:  ");
        terminal_print_number(mem_used);
        terminal_print(" bytes\n");
        
        terminal_print("Heap Free:  ");
        terminal_print_number(mem_free);
        terminal_print(" bytes\n");
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
    } else if (c == '\n') {
        execute_command();
    } else {
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
    
    //print boot sequence
    terminal_print("KalsangOS Boot Sequence:\n");
    terminal_print("[OK] Hardware Interrupts\n");
    terminal_print("[OK] System Timer (100Hz)\n");
    terminal_print("[OK] Virtual Memory (Paging)\n");
    terminal_print("--------------------------------\n");
    
    terminal_print("KalsangOS> ");
}
