#include "memory.h"

//start heap at 2 Megabyte mark in physical RAM
//safely bypasses kernel code and vga text buffer
//define HEAP_START 0x00200000
//define HEAP_SIZE  0x00200000 //for OS 2MB of dynamic memory

//metadata header attached to every block of memory
typedef struct memory_block {
    uint32_t size;
    bool is_free;
    struct memory_block* next;
} memory_block_t;


memory_block_t* heap_head;
//pointer to the very beginning of our Heap
//memory_block_t* heap_head = (memory_block_t*) HEAP_START;

/*Void init_dynamic_memory() {
    //when OS boots the entire 2MB heap is just one massive free block
    heap_head->size = HEAP_SIZE - sizeof(memory_block_t);
    heap_head->is_free = true;
    heap_head->next = NULL;
}*/
void init_dynamic_memory(uint32_t start_addr, uint32_t size) {
    heap_head = (memory_block_t*) start_addr;
    heap_head->size = size - sizeof(memory_block_t);
    heap_head->is_free = true;
    heap_head->next = NULL;
}

void* malloc(uint32_t size) {
    memory_block_t* current = heap_head;
    
    //search linked list for free block that is big enough
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            
            //if block is way bigger than we need split it into two blocks
            if (current->size > size + sizeof(memory_block_t) + 1) {
                memory_block_t* new_block = (memory_block_t*)((uint32_t)current + sizeof(memory_block_t) + size);
                new_block->is_free = true;
                new_block->size = current->size - size - sizeof(memory_block_t);
                new_block->next = current->next;

                current->next = new_block;
                current->size = size;
            }
            
            //mark block as used and hand it to the program
            current->is_free = false;
            
            //retunr memory address immediately after metadata header
            return (void*)((uint32_t)current + sizeof(memory_block_t));
        }
        current = current->next;
    }
    return NULL; //mark out of memory
}

void free(void* ptr) {
    if (ptr == NULL) return;
    
    //to free the memory we do math backward to find our metadata header
    memory_block_t* block = (memory_block_t*)((uint32_t)ptr - sizeof(memory_block_t));
    
    //simply mark it as free so malloc() can use it again later
    block->is_free = true;
}

//fill a block of memory with a specific value
void* k_memset(void* dest, uint8_t val, uint32_t count) {
    uint8_t* temp = (uint8_t*)dest;
    for (; count != 0; count--) {
        *temp++ = val;
    }
    return dest;
}

// Rename to k_memset16
void* k_memset16(void* dest, uint16_t val, uint32_t count) {
    uint16_t* temp = (uint16_t*)dest;
    for(; count != 0; count--) {
        *temp++ = val;
    }
    return dest;
}

//allocate memory AND wipe it to 0 ..... for arrays and page directories
void* calloc(uint32_t count, uint32_t size) {
    uint32_t total_size = count * size;
    void* ptr = malloc(total_size);
    
    if (ptr != NULL) {
        k_memset(ptr, 0, total_size); 
    }
    
    return ptr;
}

// Calculate heap usage statistics
void get_mem_stats(uint32_t* used, uint32_t* free_mem) {
    uint32_t total_used = 0;
    uint32_t total_free = 0;
    
    memory_block_t* current = heap_head;
    
    while (current != NULL) {
        if (current->is_free) {
            total_free += current->size;
        } else {
            total_used += current->size;
        }
        current = current->next;
    }
    
    *used = total_used;
    *free_mem = total_free;
}
