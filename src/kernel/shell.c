#include <stdint.h>
#include "io.h"
#include "shell.h"
#include "timer.h"
#include "memory.h"
#include "vfs.h"

#define MAX_HISTORY 256

// --- GLOBAL SCROLLING MEMORY ---
uint16_t history_buffer[MAX_HISTORY][80];//2D array holds 256 rows each 80 characters wide
uint16_t live_screen[25][80];//keeps track of how many total lines have scrolled off the screen
uint32_t history_count = 0; 
uint32_t view_offset = 0; //keeps track of our "camera" (0 = viewing the live prompt, >0 = scrolling back)
// -------------------------------
void redraw_view() {
    uint16_t* vga = (uint16_t*)0xB8000;

    if (view_offset == 0) {
        //restore the live screen when reaching bottom of screen
        for (int i = 0; i < 25 * 80; i++) {
            vga[i] = ((uint16_t*)live_screen)[i];
        }
        return;
    }

    //calculate window when looking into past
    for (int row = 0; row < 25; row++) {
        uint32_t past_line = (history_count - view_offset) + row; 
        
        for (int col = 0; col < 80; col++) {
            if (past_line < history_count) {
                //pull from history buffer
                uint32_t h_idx = past_line % MAX_HISTORY;
                vga[row * 80 + col] = history_buffer[h_idx][col];
            } else {
                //overlapped back into live screen buffer
                uint32_t l_idx = past_line - history_count;
                vga[row * 80 + col] = live_screen[l_idx][col];
            }
        }
    }
}

void terminal_scroll_up() {
    if (history_count == 0) return; //nth to see inpast
    
    if (view_offset == 0) {
    //snapshot befoer we leave
        uint16_t* vga = (uint16_t*)0xB8000;
        for (int i = 0; i < 25 * 80; i++) {
            ((uint16_t*)live_screen)[i] = vga[i];
        }
    }
    
    //move the camera up (limit to how much history we actually have)
    if (view_offset < history_count && view_offset < MAX_HISTORY) {
        view_offset++;
        redraw_view();
    }
}

void terminal_scroll_down() {
    if (view_offset > 0) {
        view_offset--;
        redraw_view();
    }
}

void shell_cmd_edit(char* filename);
extern char keyboard_get_last_char();
volatile bool enter_pressed = false;

extern volatile bool char_available;
extern volatile char last_char;
extern volatile bool shell_is_blocking;

void execute_command();
void run_script(char* filename);
void terminal_scroll();

static char line_buffer[256];


// === === === === === === === === === ===
typedef void (*shell_func_t)(char* args);

typedef struct shell_cmd {
    char name[32];
    shell_func_t function;
    struct shell_cmd* next;
} shell_cmd_t;

//the head of our dynamic command list
shell_cmd_t* cmd_list = NULL;

//dynamicallly live addit of command 
void add_command(char* name, shell_func_t func) {
    shell_cmd_t* new_cmd = (shell_cmd_t*)malloc(sizeof(shell_cmd_t));
    if (!new_cmd) return;

    memset(new_cmd->name, 0, 32);
    memcpy(new_cmd->name, name, 31); // Ensure null termination
    new_cmd->function = func;
    
    // Link it to the start of the list
    new_cmd->next = cmd_list;
    cmd_list = new_cmd;
}

// === === === === === === === === === ===

uint16_t* terminal_buffer = (uint16_t*) 0xB8000;
uint32_t terminal_index = 0;
uint8_t current_color = 0x0F; 

//backup for the active terminal so we dont lose our typing prompt
uint16_t live_screen[25][80];
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

void terminal_clear() {
    uint16_t blank = (uint16_t) ' ' | (uint16_t) current_color << 8;
    k_memset16(terminal_buffer, blank, 2000);
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
        if (terminal_index >= 2000) {
        terminal_scroll();
    }
    }
    update_cursor(terminal_index);
}

void terminal_scroll() {
    uint16_t* vga = (uint16_t*)0xB8000;
    
    // 1. RESCUE OPERATION: Save the top row into our history buffer before it's gone
    int history_index = history_count % MAX_HISTORY; // Math magic to loop back to 0 at 256
    for (int i = 0; i < 80; i++) {
        history_buffer[history_index][i] = vga[i]; // vga[i] is the top row (0 to 79)
    }
    history_count++; // Log that we saved a line

    // 2. Move everything up by one row (80 columns)
    for (int i = 0; i < 24 * 80; i++) {
        vga[i] = vga[i + 80];
    }
    
    // 3. Clear the very last row (Row 24)
    for (int i = 24 * 80; i < 25 * 80; i++) {
        vga[i] = (uint16_t)' ' | (uint16_t)0x0F << 8; // Change 0x0F to current_color if you use it
    }
    
    // 4. Move the cursor back
    terminal_index -= 80;
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

// convert hex string to integer
uint32_t hex_to_int(char* str) {
    uint32_t val = 0;
    while (*str) {
        uint8_t byte = *str;
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <= 'f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <= 'F') byte = byte - 'A' + 10;
        else break;
        val = (val << 4) | (byte & 0xF);
        str++;
    }
    return val;
}

//convert decimal string to integer
uint32_t atoi(char* str) {
    uint32_t res = 0;
    for (int i = 0; str[i] >= '0' && str[i] <= '9'; ++i)
        res = res * 10 + str[i] - '0';
    return res;
}

uint8_t hex_pair_to_byte(char h, char l) {
    uint8_t byte = 0;
    
    // High nibble
    if (h >= '0' && h <= '9') byte |= (h - '0') << 4;
    else if (h >= 'a' && h <= 'f') byte |= (h - 'a' + 10) << 4;
    else if (h >= 'A' && h <= 'F') byte |= (h - 'A' + 10) << 4;

    // Low nibble
    if (l >= '0' && l <= '9') byte |= (l - '0');
    else if (l >= 'a' && l <= 'f') byte |= (l - 'a' + 10);
    else if (l >= 'A' && l <= 'F') byte |= (l - 'A' + 10);
    
    return byte;
}

void run_script(char* filename) {
    // 1.find the script in the VFS
    vfs_node_t* file = vfs_find(vfs_root, filename);
    
    if (!file) {
        terminal_print("Error: Script file not found in RAMDisk.\n");
        return;
    }

    terminal_print("--- Executing Script: ");
    terminal_print(filename);
    terminal_print(" ---\n");

    char* data = (char*)file->ptr;
    char line_buffer[256];
    int line_index = 0;

    // 2.loop through every byte of the file
    for (uint32_t i = 0; i < file->length; i++) {
        char c = data[i];
        //if we hit a newline or the end of the file, execute the line!
        if (c == '\n' || i == file->length - 1) {
            //catch the last character if the file doesnt end with a newline
            if (c != '\n' && c != '\r') {
                line_buffer[line_index++] = c;
            }
            line_buffer[line_index] = '\0'; //cap off the string
            //ignore empty lines
            if (line_index > 0) {
                //hijack the main shells buffer
                memcpy(key_buffer, line_buffer, line_index + 1);
                key_index = line_index;
                //print what command we are running (Echo)
                terminal_print("> ");
                terminal_print(key_buffer);
                //tricks OS into running it
                execute_command(); 
            }
            line_index = 0; 
        } 
        //ignore carriage returns from Windows text formats
        else if (c != '\r') {
            if (line_index < 255) {
                line_buffer[line_index++] = c;
            }
        }
    }
    terminal_print("--- Script Finished ---\n");
}


// --- POWER CONTROLS ---

void reboot() {
    terminal_print("Rebooting KalsangOS...\n");
    uint8_t good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE); //send the reset command to the 8042 controller
    //force a Triple Fault if it fails
    asm volatile("lidt %0; int3" : : "m" ( (uint16_t[3]){0,0,0} ));
}

void shutdown() {
    terminal_print("Shutting down KalsangOS...\n");
    sleep(100);

    // QEMU / Bochs Magic Poweroff Ports
    outw(0x604, 0x2000);  //modern QEMU
    outw(0xB004, 0x2000); //older QEMU / Bochs
    
    // If running on real hardware and ACPI fails, halt the CPU
    terminal_print("It is now safe to turn off your computer.\n");
    asm volatile("cli; hlt");
}


/////////HELPHLEPHLEP
void print_help() {
    terminal_print("\n================ KALSANG OS v0.1 =================\n");
    
    terminal_print("SYSTEM & UTILITIES:\n");
    terminal_print("  help            - Show this command manual\n");
    terminal_print("  sysinfo         - Display CPU and OS hardware details\n");
    terminal_print("  uptime          - Show system uptime in seconds\n");
    terminal_print("  sleep <sec>     - Pause the CPU for X seconds\n");
    terminal_print("  echo <text>     - Print dynamic text back to the screen\n");
    terminal_print("  color <name>    - Change text (red, green, blue, white, matrix)\n");
    terminal_print("  clear           - Clear the terminal screen\n");
    terminal_print("  beep            - Play a test tone on the PC speaker\n");
    terminal_print("  reboot          - Restart KalsangOS\n");
    terminal_print("  shutdown/exit   - Power off the system safely\n\n");

    terminal_print("FILE SYSTEM:\n");
    terminal_print("  ls              - List all files in the RAMDisk\n");
    terminal_print("  cat <file>      - Read and display a text file\n");
    terminal_print("  edit <file>     - Open the text editor to create/modify a file\n\n");

    terminal_print("EXECUTION & SCRIPTING:\n");
    terminal_print("  run <file>      - Execute a compiled machine-code binary\n");
    terminal_print("  run_script <f>  - Execute a plain-text KalsangOS script\n");
    terminal_print("  make_bin <file> - Convert raw hex string into an executable\n\n");

    terminal_print("MEMORY & PROCESSES:\n");
    terminal_print("  memstat         - View total, used, and free Heap memory\n");
    terminal_print("  ps              - Show active memory blocks (running tasks)\n");
    terminal_print("  kill <addr>     - Force-free a specific memory address\n\n");

    terminal_print("DEBUGGING & KERNEL TESTS:\n");
    terminal_print("  peek <addr>     - View 8 bytes of raw hex at an address\n");
    terminal_print("  scan <addr>     - Scan memory for 'ustar' magic signatures\n");
    terminal_print("  testcrash       - Simulate a Division-by-Zero CPU exception\n");
    terminal_print("  syscall_test    - Test Interrupt 0x80 (SYS_PRINT) mechanism\n");
    terminal_print("  exec_test       - Inject and execute raw code in User Space\n");
    terminal_print("  write_test      - Simulate a compiler saving an output file\n");
    terminal_print("  malloc          - Test dynamic heap memory allocation\n");

    terminal_print("==================================================\n\n");
}


// === SHELL LOGIC ===
void execute_command() {
    terminal_print("\n");
    key_buffer[key_index] = '\0';

    if (key_index == 0) {
        terminal_print("KalsangOS> ");
        return;
    }

    //search for dynamic commands first
    shell_cmd_t* current = cmd_list;
    while (current != NULL) {
        if (strcmp(key_buffer, current->name) == 0) {
            current->function(NULL); //execute the live-added code
            goto done;
        }
        current = current->next;
    }
    
	if (strcmp(key_buffer, "help") == 0) {
        print_help();
    }
    else if (strcmp(key_buffer, "clear") == 0) {
        terminal_clear();
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
            terminal_print("Unknown color. Do: red, green, blue, white, matrix\n");
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
        uint32_t seconds = timer_ticks / 100;
        
        terminal_print("KalsangOS Uptime: ");
        terminal_print_number(seconds);
        terminal_print(" seconds\n");
    } 
    /*
    else if (strcmp(key_buffer, "reboot") == 0) {
        terminal_print("Rebooting KalsangOS...\n");
        outb(0x64, 0xFE);
        asm volatile("cli");
        asm volatile("hlt");
    } */
    // === POWER COMMANDS ===
    else if (strcmp(key_buffer, "reboot") == 0) {
        reboot();
    }
    else if (strcmp(key_buffer, "shutdown") == 0 || strcmp(key_buffer, "exit") == 0) {
        shutdown();
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
            terminal_print("Usage: sleep <seconds> (e.g : sleep 3)\n");
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
        terminal_print_number(mem_used+mem_free); 
        terminal_print(" bytes\n");
        
        terminal_print("Heap Used:  ");
        terminal_print_number(mem_used);
        terminal_print(" bytes\n");
        
        terminal_print("Heap Free:  ");
        terminal_print_number(mem_free);
        terminal_print(" bytes\n");
    }
    
    // === LS comd ===
    else if (strcmp(key_buffer, "ls") == 0) {
		extern vfs_node_t ramdisk_nodes[];
		extern int node_count;

		terminal_print("Files in Initrd:\n");
		for (int i = 0; i < node_count; i++) {
		    terminal_print("- ");
		    terminal_print(ramdisk_nodes[i].name);
		    terminal_print(" (");
		    terminal_print_number(ramdisk_nodes[i].length);
		    terminal_print(" bytes)\n");
		}
	}
	
	// === cat ===
	else if (strncmp(key_buffer, "cat ", 4) == 0) {
		const char* filename = key_buffer + 4;
		bool found = false;

		for (int i = 0; i < node_count; i++) {
		    if (strcmp(ramdisk_nodes[i].name, filename) == 0) {
		        char* file_data = (char*)ramdisk_nodes[i].ptr;
		        
		        for (uint32_t j = 0; j < ramdisk_nodes[i].length; j++) {
		            //creating a tiny string to print char by char
		            char buf[2] = {file_data[j], '\0'};
		            terminal_print(buf);
		        }
		        terminal_print("\n");
		        found = true;
		        break;
		    }
		}
		if (!found) {
		    terminal_print("File not found.\n");
		}
	}
	else if (strncmp(key_buffer, "peek ", 5) == 0) {
		char *addr_str = key_buffer + 5;
		uint32_t addr = hex_to_int(addr_str);
		uint8_t *ptr = (uint8_t*)addr;

		terminal_print("Raw Bytes at ");
		terminal_print(addr_str);
		terminal_print(": ");

		// Print the first 8 bytes
		char hex_chars[] = "0123456789ABCDEF";
        for (int i = 0; i < 8; i++) {
            uint8_t b = ptr[i];
            char out[3];
            out[0] = hex_chars[(b >> 4) & 0x0F];
            out[1] = hex_chars[b & 0x0F];
            out[2] = ' ';
            terminal_print(out); 
        }
		terminal_print("\n");
	}
	else if (strncmp(key_buffer, "scan ", 5) == 0) {
		char *addr_str = key_buffer + 5;
		uint32_t start_addr = atoi(addr_str); // atoi for decimal addresses
		
		terminal_print("Scanning starting at: ");
		terminal_print_number(start_addr);
		terminal_print("\n");

		for (uint32_t i = 0; i < 4096; i++) {
		    char* ptr = (char*)(start_addr + i);
		    if (ptr[0] == 'u' && ptr[1] == 's' && ptr[2] == 't' && ptr[3] == 'a' && ptr[4] == 'r') {
		        terminal_print("Found magic at offset: ");
		        terminal_print_number(i);
		        terminal_print("\n");
		    }
		}
	}
	else if (strcmp(key_buffer, "syscall_test") == 0) {
		terminal_print("Testing KalsangOS Syscall System...\n");
		
		char* test_msg = "SUCCESS: Hello from inside Syscall 0x80 \n";

		asm volatile (
		    "mov $1, %%eax\n\t"  //syscall 1 (SYS_PRINT)
		    "mov %0, %%ebx\n\t"  //argument: ptr to the string
		    "int $0x80"          //trigger intrp
		    : : "r"(test_msg) : "eax", "ebx"
		);
	}
		// === compliler and run === /
	else if (strncmp(key_buffer, "run ", 4) == 0) {
		char* filename = key_buffer + 4;
		vfs_node_t* file = vfs_find(vfs_root, filename);

		if (file) {
		    terminal_print("Loading execution point: ");
		    terminal_print_number((uint32_t)file->ptr);
		    terminal_print("\n");

		    //create a function pointer tofile location in RAM
		    typedef void (*entry_point)();
		    entry_point start = (entry_point)file->ptr;

		    //jump
		    start();
		    
		    terminal_print("\nExecution finished.\n");
		} else {
		    terminal_print("Error: Executable not found.\n");
		}
	}
	
	else if (strcmp(key_buffer, "exec_test") == 0) {
		terminal_print("Allocating User Space...\n");

		//ask our new Memory Manager for 64 bytes
		unsigned char* code_space = (unsigned char*) malloc(64);
		char* test_msg = "Hello from the injected binary!\n";

		if (code_space != NULL) {
		    // inject bin code
		    code_space[0] = 0xB8; code_space[1] = 0x01; //mov eax, 1
		    code_space[2] = 0x00; code_space[3] = 0x00; 
		    code_space[4] = 0x00;
		    
		    code_space[5] = 0xBB; //mov ebx, (address)
		    uint32_t msg_ptr = (uint32_t)test_msg;
		    memcpy(&code_space[6], &msg_ptr, 4);

		    code_space[10] = 0xCD; code_space[11] = 0x80; //int 0x80
		    code_space[12] = 0xC3;                       //ret

		    terminal_print("Jumping to User Code at: ");
		    terminal_print_number((uint32_t)code_space);
		    terminal_print("\n");

		    //leap
		    typedef void (*user_code_t)();
		    user_code_t run_me = (user_code_t)code_space;
		    run_me();
		    //clean up
		    free(code_space);
		    terminal_print("Returned safely to Kernel.\n");
		}
	}

	else if (strcmp(key_buffer, "write_test") == 0) {
		terminal_print("Kernel: Simulating a file save...\n");
		
		char* test_filename = "output.bin";
		char* dummy_data = "KALSANG_COMPILER_OUTPUT_2026";
		uint32_t data_len = 28; //length of dummy_data string

		//call the VFS function you just declared
		vfs_create(test_filename, dummy_data, data_len);

		terminal_print("Kernel: File 'output.bin' created in RAMDisk\n");
		terminal_print("Type 'ls' to verify\n");
	}
	else if (strncmp(key_buffer, "edit ", 5) == 0) {
		char* filename = key_buffer + 5;
		shell_cmd_edit(filename); //editor function 
	}
	// === RUN SCRIPT COMMAND ===
    else if (strncmp(key_buffer, "run_script ", 11) == 0) {
        char* filename = key_buffer + 11;
        run_script(filename);
    }
    // === MAKE BINARY COMMAND ===
    else if (strncmp(key_buffer, "make_bin ", 9) == 0) {
        char* filename = key_buffer + 9;
        terminal_print("Enter raw hex no spaces:\n> ");
        char* hex_str = shell_readline(); 
        uint32_t len = strlen(hex_str);
        uint32_t byte_count = len / 2;
        //allocate permanent heap memory for the binary
        char* bin_data = (char*)malloc(byte_count);
        //convert text to machine code
        for (uint32_t i = 0; i < byte_count; i++) {
            bin_data[i] = hex_pair_to_byte(hex_str[i*2], hex_str[i*2 + 1]);
        }
        //save to RAMDisk
        vfs_node_t* existing_file = vfs_find(vfs_root, filename);
        if (existing_file != NULL) {
            existing_file->ptr = (void*)bin_data;
            existing_file->length = byte_count;
        } else {
            vfs_create(filename, bin_data, byte_count);
        }
        terminal_print("binary code saved to execute with: run ");
        terminal_print(filename);
        terminal_print("\n");
    }
    // === PROCESS STATUS / HEAP MAP ===
    else if (strcmp(key_buffer, "ps") == 0) {
        debug_heap_dump();
    }
    
    // === KILL COMMAND (Free memory by address) ===
    else if (strncmp(key_buffer, "kill ", 5) == 0) {
        //convert the string address (like 8388638) back into a number
        char* addr_str = key_buffer + 5;
        uint32_t addr = 0;
        
        //simple string-to-int conversion
        while (*addr_str >= '0' && *addr_str <= '9') {
            addr = addr * 10 + (*addr_str - '0');
            addr_str++;
        }

        if (addr >= 0x00100000) { // Safety check: don't kill the low kernel memory!
            free((void*)addr);
            terminal_print("Memory at address freed.\n");
        } else {
            terminal_print("Error: Invalid or protected memory address.\n");
        }
    }
    else if (strncmp(key_buffer, "beep", 4) == 0) {
        beep(750, 200); 
    }
	else {
        terminal_print("Unknown command: ");
        terminal_print(key_buffer);
        terminal_print("\n");
    }

done:
    key_index = 0;
    terminal_print("KalsangOS> ");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void shell_handle_keypress(char c) {
    if (c == '\b') {
        if (key_index > 0) {
            key_index--; 
            terminal_index--; 
            terminal_buffer[terminal_index] = (uint16_t) ' ' | (uint16_t) current_color << 8; 
            update_cursor(terminal_index);
        }
    } else if (c == '\n') {\
        enter_pressed = true; 
    } else {
        if (key_index < BUFFER_SIZE - 1) {
            key_buffer[key_index++] = c; 
            terminal_buffer[terminal_index++] = (uint16_t) c | (uint16_t) current_color << 8; 
            update_cursor(terminal_index);
        }
    }
    if (terminal_index >= 2000) terminal_clear();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//// LIVEtemporary heap buffer to store your keystrokes until you decide to commit them to the disk /////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

void shell_cmd_edit(char* filename) {
    if (filename == NULL || filename[0] == '\0') {
        terminal_print("Usage: edit <filename>\n");
        return;
    }

    terminal_print("--- KalsangOS Line Editor ---\n");
    terminal_print("Type your code. Type 'SAVE' on a new line to exit.\n");

    // Allocate 4KB for the new file buffer
    char* edit_buffer = (char*)malloc(4096);
    uint32_t total_len = 0;
    bool editing = true;

    while (editing) {
        terminal_print("> ");
        char* line = shell_readline(); 

        if (strcmp(line, "SAVE") == 0) {
            editing = false;
        } else {
            //append line to buffer
            uint32_t line_len = strlen(line);
            memcpy(edit_buffer + total_len, line, line_len);
            total_len += line_len;
            edit_buffer[total_len++] = '\n';
        }
    }

    //write the buffer to the VFS
    //chck if the file already exists in the RAMDisk
    vfs_node_t* existing_file = vfs_find(vfs_root, filename);

    if (existing_file != NULL) {
        //llocate permanent heap memory for the new file data
        char* permanent_storage = (char*)malloc(total_len);
        memcpy(permanent_storage, edit_buffer, total_len);

        //overwrite old file pointer and length
        existing_file->ptr = (void*)permanent_storage;
        existing_file->length = total_len;
        
        terminal_print("Existing file overwritten and saved.\n");
    } else {
        //if it doesnt exist create it normally
        vfs_create(filename, edit_buffer, total_len);
        terminal_print("New file created and saved to RAMDisk.\n");
    }

    free(edit_buffer);
}

char* shell_readline() {
    shell_is_blocking = true;
    int index = 0;

    // Zero out the buffer
    for(int i = 0; i < 256; i++) line_buffer[i] = 0;

    char_available = false; 
    last_char = 0;

    while (shell_is_blocking) {
        if (char_available) {
            char c = last_char;
            char_available = false;

            if (c == '\n') {
                line_buffer[index] = '\0';
                terminal_print("\n"); // Newline is safe for terminal_print
                shell_is_blocking = false;
                return line_buffer;
            } 
            // === BACKSPACE FIX ===
            else if (c == '\b' && index > 0) {
                index--; // Remove from your string
                
                // Manually erase from the VGA screen
                terminal_index--;
                terminal_buffer[terminal_index] = (uint16_t)' ' | (uint16_t)current_color << 8;
                update_cursor(terminal_index);
            } 
            // === TYPING FIX (Including Capitals) ===
            else if (index < 254 && c >= 32) {
                line_buffer[index++] = c; // Add to your string
                
                // Write directly to VGA screen bypassing terminal_print
                terminal_buffer[terminal_index++] = (uint16_t)c | (uint16_t)current_color << 8;
                update_cursor(terminal_index);
            }
        }
        // Keep the CPU resting slightly so it doesn't overheat
        asm volatile("pause"); 
    }
    return line_buffer;
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

void shell_update() {
    if (enter_pressed) {
        execute_command();
        enter_pressed = false;
    }
}
