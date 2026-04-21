#ifndef KALSANG_LIBC_H
#define KALSANG_LIBC_H

#include <stdint.h>

//force GCC to ALWAYS inline these so they never become rogue entry points
#define INLINE static inline __attribute__((always_inline))

INLINE void print(char* msg) {
    asm volatile("int $0x80" : : "a"(1), "b"(msg) : "memory");
}

INLINE int read_file(char* name, void* buffer) {
    int res;
    asm volatile("int $0x80" : "=a"(res) : "a"(2), "b"(name), "c"(buffer) : "memory");
    return res;
}

INLINE void write_file(char* name, void* data, uint32_t size) {
    asm volatile("int $0x80" : : "a"(3), "b"(name), "c"(data), "d"(size) : "memory");
}

INLINE void exit() {
    asm volatile("int $0x80" : : "a"(4) : "memory");
}

INLINE int open(char* name) {
    int fd;
    asm volatile("int $0x80" : "=a"(fd) : "a"(20), "b"(name) : "memory");
    return fd;
}

INLINE void close(int fd) {
    asm volatile("int $0x80" : : "a"(21), "b"(fd) : "memory");
}

INLINE int read(int fd, void* buffer, uint32_t size) {
    int bytes_read;
    asm volatile("int $0x80" : "=a"(bytes_read) : "a"(22), "b"(fd), "c"(buffer), "d"(size) : "memory");
    return bytes_read;
}

INLINE void write(int fd, void* data, uint32_t size) {
    asm volatile("int $0x80" : : "a"(23), "b"(fd), "c"(data), "d"(size) : "memory");
}

INLINE void* sbrk(int increment) {
    void* res;
    asm volatile("int $0x80" : "=a"(res) : "a"(24), "b"(increment) : "memory");
    return res;
}

INLINE void* malloc(uint32_t size) {
    if (size == 0) return 0;
    
    //align to 4 bytes
    size = (size + 3) & ~3;
    
    void* mem = sbrk(size);
    if ((int)mem == -1) return 0; 
    
    return mem;
}

INLINE void free(void* ptr) {
    (void)ptr; 
}

#endif
