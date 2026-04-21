#include "kx.h"
#include "memory.h"
#include "shell.h"
#include "vfs.h"
extern uint32_t current_user_brk;
extern volatile bool char_available;
extern volatile uint8_t last_char;
extern volatile bool shell_is_blocking;

extern void execute_ring3(uint32_t entry_point, uint32_t user_stack);

#include "kx.h"
#include "memory.h"
#include "shell.h"
#include "vfs.h"

extern uint32_t current_user_brk;
extern void execute_ring3(uint32_t entry_point, uint32_t user_stack);

// Hardware & System locks
extern volatile bool shell_is_blocking;
extern volatile bool char_available;
extern volatile uint8_t last_char;
char chain_program_name[64] = {0}; 

void run_kx_file(char* filename, char* args) {
    vfs_node_t* file = vfs_find(vfs_root, filename);

    if (!file) {
        terminal_print("Error: file not found\n");
        return;
    }

    if (file->length < sizeof(kx_header_t)) {
        terminal_print("Error: invalid executable\n");
        return;
    }

    kx_header_t* header = (kx_header_t*)file->ptr;

    if (header->magic != KX_MAGIC) {
        terminal_print("Error: not a KX executable\n");
        return;
    }

    uint32_t total_size = header->code_size + header->data_size;

    // --- FIXED EXECUTION ADDRESS ---
    void* mem = (void*)0x02000000; 
    memset(mem, 0, total_size); 

    uint8_t* src = (uint8_t*)file->ptr + sizeof(kx_header_t);
    uint8_t* dst = (uint8_t*)mem;

    memcpy(dst, src, header->code_size);

    if (header->data_size > 0) {
        memcpy(dst + header->code_size, src + header->code_size, header->data_size);
    }

    // Give the user program its own 4KB memory stack
    void* user_stack = malloc(4096); 
    if (!user_stack) {
        terminal_print("Error: out of memory for user stack\n");
        return;
    }

	current_user_brk = 0; 
    uint32_t entry_point = (uint32_t)mem + header->entry;
    uint32_t stack_top = (uint32_t)user_stack + 4096; 

    // --- FORGE THE POSIX C STACK ---
    // 1. cpy the argument string to the very top of the stack
    int arg_len = 0;
    if (args) while (args[arg_len]) arg_len++;
    
    stack_top -= (arg_len + 1);
    char* user_args_str = (char*)stack_top;
    if (args) memcpy(user_args_str, args, arg_len + 1);
    else user_args_str[0] = '\0';

    // 2. cpy the filename string right below it
    int file_len = 0;
    while (filename[file_len]) file_len++;
    
    stack_top -= (file_len + 1);
    char* user_file_str = (char*)stack_top;
    memcpy(user_file_str, filename, file_len + 1);

    // 3. align the stack pointer to 4 bytes (Critical for x86 CPUs)
    stack_top &= ~3;

    // 4. create the argv array: [ argv[0] = filename, argv[1] = args ]
    stack_top -= 8;
    uint32_t* argv = (uint32_t*)stack_top;
    argv[0] = (uint32_t)user_file_str;
    argv[1] = (uint32_t)user_args_str;

    // 5. Push the argv pointer
    stack_top -= 4;
    *(uint32_t*)stack_top = (uint32_t)argv;

    // 6. Push argc (We are passing 2 elements: the program name and the args)
    stack_top -= 4;
    *(uint32_t*)stack_top = 2;

    // 7. Push a dummy Return Address (0) so the program safely crashes if it forgets to call exit()
    stack_top -= 4;
    *(uint32_t*)stack_top = 0;

    terminal_print("Launching KX program in Ring 3...\n");

    // --- LOCK KEYBOARD & FLUSH GHOST ENTER ---
    shell_is_blocking = true;
    char_available = false;
    last_char = 0;

    // Jump to User Space! 
    execute_ring3(entry_point, stack_top);

    // --- WAKE SHELL & CLEAN UP ---
    shell_is_blocking = false;
    char_available = false; // Reset this so the shell doesn't auto-run
    last_char = 0;

    terminal_print("\nProgram finished. Memory cleaned.");
    free(user_stack);

    // --- EXEC CHAINING ---
    if (chain_program_name[0] != '\0') {
        terminal_print("Process requested EXEC. Spawning: ");
        terminal_print(chain_program_name);
        terminal_print("\n");
        
        char next_prog[64];
        memcpy(next_prog, chain_program_name, 64);
        chain_program_name[0] = '\0'; 
        
        run_kx_file(next_prog, ""); 
    }
}
