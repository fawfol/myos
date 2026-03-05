#include "vfs.h"
#include "memory.h"
#include "shell.h"
#include "memory.h"

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
    char magic[6];      //ustar
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
    
    tar_header_t *header = (tar_header_t*)address;

    terminal_print("First 4 bytes of module: ");
    for(int i = 0; i < 4; i++) {
        char buf[2] = { ((char*)location)[i], '\0' };
        if(buf[0] == 0) terminal_print(".");
        else terminal_print(buf);
    }
    terminal_print("\n");

    // DEBUG: Let's see what is at the magic offset (257)
    terminal_print("Magic field contains: ");
    terminal_print(header->magic); 
    terminal_print("\n");
    terminal_print("KalsangOS>");

    while (node_count < 32) {
        tar_header_t *header = (tar_header_t*)address;

        //check for ustar magic to confirm this is a valid TAR header
        //some tar creators put "ustar " (with a space) or "ustar\0"
        if (header->magic[0] != 'u' || header->magic[1] != 's') {
            break; 
        }

        vfs_node_t *node = &ramdisk_nodes[node_count];
        
        //copy name safely
        for(int i = 0; i < 100 && header->name[i] != '\0'; i++) {
            node->name[i] = header->name[i];
        }

        node->length = octal_to_int(header->size);
        node->ptr = (vfs_node_t*)(address + 512); //data follows header
        node->flags = VFS_FILE;

        if (node_count == 0) vfs_root = node;

        //TAR blocks are always 512 bytes. 
        //move: 512 (header) + size (rounded up to nearest 512)
        address += ((node->length + 511) & ~511) + 512;
        node_count++;
    }
}

vfs_node_t* vfs_find(vfs_node_t* root, char* name) {
    //assume ramdisk_nodes is global array from earlier
    (void)root; //do nithing empty for now    
    extern vfs_node_t ramdisk_nodes[32];
    extern int node_count;

    for (int i = 0; i < node_count; i++) {
        if (strcmp(ramdisk_nodes[i].name, name) == 0) {
            return &ramdisk_nodes[i];
        }
    }
    return NULL;
}
