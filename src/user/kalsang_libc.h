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
