#include "vfs.h"
#include "memory.h"
#include "shell.h"

vfs_node_t *vfs_root = 0;
vfs_node_t ramdisk_nodes[32];
int node_count = 0;

typedef struct {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];      // "ustar"
    char version[2];
} __attribute__((packed)) tar_header_t;

uint32_t octal_to_int(char *str) {
    uint32_t size = 0;
    for (int i = 0; str[i] >= '0' && str[i] <= '7'; i++) {
        size = size * 8 + (str[i] - '0');
    }
    return size;
}

void init_ramdisk(uint32_t location) {
    uint32_t address = location;
    node_count = 0;
    vfs_root = 0;

    tar_header_t *header = (tar_header_t*)address;

    terminal_print("First 4 bytes of module: ");
    for (int i = 0; i < 4; i++) {
        char buf[2] = { ((char*)location)[i], '\0' };
        if (buf[0] == 0) terminal_print(".");
        else terminal_print(buf);
    }
    terminal_print("\n");

    terminal_print("Magic field contains: ");
    terminal_print(header->magic);
    terminal_print("\n");

    vfs_node_t* prev = NULL;

    while (node_count < 32) {
        header = (tar_header_t*)address;

        // End of tar archive
        if (header->name[0] == 0) {
            break;
        }

        vfs_node_t *node = &ramdisk_nodes[node_count];
        memset(node, 0, sizeof(vfs_node_t));

        // Copy name safely and ensure null-termination
        for (int i = 0; i < 99 && header->name[i] != 0; i++) {
            node->name[i] = header->name[i];
        }
        node->name[99] = '\0';

        node->length = octal_to_int(header->size);
        node->ptr = (vfs_node_t*)(address + 512); // file data starts after tar header
        node->flags = VFS_FILE;
        node->next = NULL;

        if (prev == NULL) {
            vfs_root = node;
        } else {
            prev->next = node;
        }
        prev = node;

        address += ((node->length + 511) & ~511) + 512;
        node_count++;
    }

    terminal_print("RAMDisk loaded: ");
    terminal_print_number(node_count);
    terminal_print(" files\n");
}

void vfs_create(char* name, char* data, uint32_t size) {
    vfs_node_t* new_node = (vfs_node_t*)malloc(sizeof(vfs_node_t));
    if (!new_node) {
        terminal_print("Error: Out of memory\n");
        return;
    }

    memset(new_node, 0, sizeof(vfs_node_t));

    for (int i = 0; i < 127 && name[i] != '\0'; i++) {
        new_node->name[i] = name[i];
    }

    new_node->length = size;
    new_node->flags = VFS_FILE;

    void* file_copy = malloc(size);
    if (!file_copy) {
        terminal_print("Error: Out of memory\n");
        free(new_node);
        return;
    }

    memcpy(file_copy, data, size);
    new_node->ptr = (vfs_node_t*)file_copy;

    new_node->next = vfs_root;
    vfs_root = new_node;
    node_count++;
}

vfs_node_t* vfs_find(vfs_node_t* root, char* name) {
    vfs_node_t* current = root;

    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}
