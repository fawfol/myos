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

void run_kx_file(char* filename) {
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

    //calculate total size first
    uint32_t total_size = header->code_size + header->data_size;

    //load all Ring 3 programs exactly at the 32MB mark in RAM.
    void* mem = (void*)0x02000000; 
    
    // Clear out any leftovers from the previous program
    memset(mem, 0, total_size); 

    uint8_t* src = (uint8_t*)file->ptr + sizeof(kx_header_t);
    uint8_t* dst = (uint8_t*)mem;

    // copy code section
    memcpy(dst, src, header->code_size);

    // copy data section
    if (header->data_size > 0) {
        memcpy(dst + header->code_size, src + header->code_size, header->data_size);
    }

    //give user program its own 4KB memory stack (Stacks can still use malloc)
    void* user_stack = malloc(4096); 
    if (!user_stack) {
        terminal_print("Error: out of memory for user stack\n");
        return;
    }

    //reset the User Heap tracker for the new program's sbrk
    current_user_brk = 0; 

    uint32_t entry_point = (uint32_t)mem + header->entry;
    uint32_t stack_top = (uint32_t)user_stack + 4096; 

    terminal_print("Launching KX program in Ring 3...\n");

    // --- LOCK THE KEYBOARD TO THE USER PROGRAM ---
    shell_is_blocking = true;
    char_available = false;
    last_char = 0;

    //jumps to user space 
    execute_ring3(entry_point, stack_top);

    // --- WAKE THE SHELL BACK UP ---
    shell_is_blocking = false;

    terminal_print("\nProgram finished. Memory cleaned.\n");
    free(user_stack);
    
    //note:  no longer free(mem) here, because mem is a fixed physical address
}
