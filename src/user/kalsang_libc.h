#ifndef KALSANG_LIBC_H
#define KALSANG_LIBC_H

#include <stdint.h>

#define SYS_PRINT_NUM 7
#define SYS_PRINT 1
#define SYS_READ  2
#define SYS_WRITE 3
#define SYS_EXIT  4

static inline void print(char* msg) {
    asm volatile(
        "mov %0, %%ebx\n\t"
        "mov $1, %%eax\n\t"
        "int $0x80"
        :
        : "r"(msg)
        : "eax", "ebx"
    );
}

static inline int read_file(char* name, void* buffer) {
    int res;
    asm volatile(
        "mov %1, %%ebx\n\t"
        "mov %2, %%ecx\n\t"
        "mov $2, %%eax\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0"
        : "=r"(res)
        : "r"(name), "r"(buffer)
        : "eax", "ebx", "ecx"
    );
    return res;
}

static inline void write_file(char* name, void* data, uint32_t size) {
    asm volatile(
        "mov %0, %%ebx\n\t"
        "mov %1, %%ecx\n\t"
        "mov %2, %%edx\n\t"
        "mov $3, %%eax\n\t"
        "int $0x80"
        :
        : "r"(name), "r"(data), "r"(size)
        : "eax", "ebx", "ecx", "edx"
    );
}

static inline int open(char* name) {
    int fd;
    asm volatile(
        "int $0x80"
        : "=a"(fd)               //output: EAX goes into 'fd'
        : "a"(20), "b"(name)     //inputs: EAX=20, EBX=name
        : "memory"               //clobbers
    );
    return fd;
}

static inline void close(int fd) {
    asm volatile(
        "int $0x80"
        :                        //no output
        : "a"(21), "b"(fd)       //inputs: EAX=21, EBX=fd
        : "memory"
    );
}

static inline int read(int fd, void* buffer, uint32_t size) {
    int bytes_read;
    asm volatile(
        "int $0x80"
        : "=a"(bytes_read)                           //output: EAX goes to 'bytes_read'
        : "a"(22), "b"(fd), "c"(buffer), "d"(size)   //inputs: EAX=22, EBX=fd, ECX=buffer, EDX=size
        : "memory"
    );
    return bytes_read;
}

static inline void write(int fd, void* data, uint32_t size) {
    asm volatile(
        "int $0x80"
        :                                            //no output
        : "a"(23), "b"(fd), "c"(data), "d"(size)     //inputs: EAX=23, EBX=fd, ECX=data, EDX=size
        : "memory"
    );
}

static inline void exit() {
    asm volatile(
        "mov $4, %%eax\n\t"
        "int $0x80"
        :
        :
        : "eax"
    );
}

#endif

static inline void* sbrk(int increment) {
    void* res;
    asm volatile(
        "int $0x80"
        : "=a"(res)
        : "a"(24), "b"(increment)
        : "memory"
    );
    return res;
}

//Simple User-Space Bump Allocator
static inline void* malloc(uint32_t size) {
    if (size == 0) return 0;
    
    //align the requested size to 4 bytes for CPU efficiency
    size = (size + 3) & ~3;
    
    void* mem = sbrk(size);
    if ((int)mem == -1) return 0; // Kernel refused (Out of Memory)
    
    return mem;
}

//bump allocator cant really free memory but we define it 
//so ported C programs dont fail to compile
static inline void free(void* ptr) {
    (void)ptr; 
}
