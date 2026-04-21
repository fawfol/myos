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

    uint32_t total_size = header->code_size + header->data_size;

    void* mem = malloc(total_size);
    if (!mem) {
        terminal_print("Error: out of memory for program\n");
        return;
    }

    uint8_t* src = (uint8_t*)file->ptr + sizeof(kx_header_t);
    uint8_t* dst = (uint8_t*)mem;

    // copy code section
    memcpy(dst, src, header->code_size);

    // copy data section
    if (header->data_size > 0) {
        memcpy(dst + header->code_size, src + header->code_size, header->data_size);
    }

    // --- NEW RING 3 ISOLATION CODE ---
    //give user program its own 4KB memory stack
    void* user_stack = malloc(4096); 
    if (!user_stack) {
        terminal_print("Error: out of memory for user stack\n");
        free(mem);
        return;
    }

    uint32_t entry_point = (uint32_t)mem + header->entry;
    uint32_t stack_top = (uint32_t)user_stack + 4096; //stack grows downwards

    terminal_print("Launching KX program in Ring 3...\n");


	//debug
	uint8_t* code_check = (uint8_t*)entry_point;
    terminal_print("First byte of code: ");
    terminal_print_hex(code_check[0]);
    terminal_print("\n");
    current_user_brk = 0; //reset the User Heap tracker for new program
    //jump to User Space (Kernel execution will pause here until SYS_EXIT is called)
    execute_ring3(entry_point, stack_top);

    // When SYS_EXIT is triggered, the kernel teleports right back here
    terminal_print("\nProgram finished. Memory cleaned.\n");
    free(user_stack);
    free(mem);
}
