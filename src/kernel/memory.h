#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

//standard C functions from memory.c
void* memset(void* dest, int val, uint32_t count);
void* memcpy(void* dest, const void* src, uint32_t count);
void* k_memset16(void* dest, uint16_t val, uint32_t count);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, uint32_t n);

//heap functions
void init_dynamic_memory(uint32_t start_addr, uint32_t size);
void* malloc(uint32_t size);
void free(void* ptr);
void get_mem_stats(uint32_t* used, uint32_t* free_mem);

#endif
