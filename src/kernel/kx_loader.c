#include "kx.h"
#include "memory.h"
#include "shell.h"
#include "vfs.h"

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
        terminal_print("Error: out of memory\n");
        return;
    }

    uint8_t* file_data = (uint8_t*)file->ptr + sizeof(kx_header_t);

    memcpy(mem, file_data, header->code_size);

    //entry point
    typedef void (*entry_t)();
    entry_t entry = (entry_t)((uint32_t)mem + header->entry);

    terminal_print("Running KX program...\n");

    entry();

    terminal_print("\nProgram finished.\n");

    free(mem);
}
