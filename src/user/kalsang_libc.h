#include <stdint.h>

void print(char* msg) {
    asm volatile("mov $1, %%eax; mov %0, %%ebx; int $0x80" : : "r"(msg) : "eax", "ebx");
}

int read_file(char* name, void* buffer) {
    int res;
    asm volatile("mov $2, %%eax; mov %1, %%ebx; mov %2, %%ecx; int $0x80; mov %%eax, %0" 
                 : "=r"(res) : "r"(name), "r"(buffer) : "eax", "ebx", "ecx");
    return res;
}
