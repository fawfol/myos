#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void init_dynamic_memory();
void* malloc(uint32_t size);
void free(void* ptr);

void get_mem_stats(uint32_t* used, uint32_t* free_mem);

void* k_memset(void* dest, uint8_t val, uint32_t count);
void* k_memset16(void* dest, uint16_t val, uint32_t count);
void* calloc(uint32_t count, uint32_t size);

//use macro so rest of OS can still just call "memset"
#define memset k_memset
#define memset16 k_memset16

#endif
