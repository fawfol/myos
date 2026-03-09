#ifndef VFS_H
#define VFS_H

#include <stdint.h>

#define VFS_FILE      0x01
#define VFS_DIRECTORY 0x02

struct vfs_node;

typedef uint32_t (*read_type_t)(struct vfs_node*, uint32_t, uint32_t, uint8_t*);
typedef uint32_t (*write_type_t)(struct vfs_node*, uint32_t, uint32_t, uint8_t*);
typedef void (*open_type_t)(struct vfs_node*);
typedef void (*close_type_t)(struct vfs_node*);

typedef struct vfs_node {
    char name[128];
    uint32_t mask;
    uint32_t uid;
    uint32_t gid;
    uint32_t flags;
    uint32_t length;

    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;

    struct vfs_node *ptr;
    
    struct vfs_node *next; 
} vfs_node_t;

extern vfs_node_t *vfs_root;
extern vfs_node_t ramdisk_nodes[32];
extern int node_count;

void init_ramdisk(uint32_t location);
void vfs_create(char* name, char* data, uint32_t size);
vfs_node_t* vfs_find(vfs_node_t* root, char* name);

#endif
