#ifndef VFS_H
#define VFS_H

#include <stdint.h>

#define VFS_FILE      0x01
#define VFS_DIRECTORY 0x02

struct vfs_node;

//function pointers for file operations
typedef uint32_t (*read_type_t)(struct vfs_node*, uint32_t, uint32_t, uint8_t*);
typedef uint32_t (*write_type_t)(struct vfs_node*, uint32_t, uint32_t, uint8_t*);
typedef void (*open_type_t)(struct vfs_node*);
typedef void (*close_type_t)(struct vfs_node*);

typedef struct vfs_node {
    char name[128];     //name of the file/dir
    uint32_t mask;      //permissions
    uint32_t uid;       //user ID
    uint32_t gid;       //group ID
    uint32_t flags;     //node type (File/Dir)
    uint32_t length;    //size of file in bytes
    
    //callbacks provided by the specific filesystem (eg Ramdisk)
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;
    
    struct vfs_node *ptr; //used for mount points or directory pointers
} vfs_node_t;

extern vfs_node_t *vfs_root; //the "/" of OS
extern vfs_node_t ramdisk_nodes[32];
extern int node_count;
extern vfs_node_t *vfs_root;

void init_ramdisk(uint32_t location);

#endif
