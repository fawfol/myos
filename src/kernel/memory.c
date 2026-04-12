#include "memory.h"
#include "shell.h"

// metadata header attached to every block of memory
typedef struct memory_block {
    uint32_t size;
    bool is_free;
    struct memory_block* next;
} memory_block_t;

static memory_block_t* heap_head = NULL;

// ---------- internal helpers ----------

static void coalesce_free_blocks() {
    memory_block_t* current = heap_head;

    while (current != NULL && current->next != NULL) {
        // If current and next are both free, merge them
        if (current->is_free && current->next->is_free) {
            current->size += sizeof(memory_block_t) + current->next->size;
            current->next = current->next->next;
            // do not advance yet; maybe the new next is also free
        } else {
            current = current->next;
        }
    }
}

static uint32_t align4(uint32_t size) {
    return (size + 3) & ~3;
}

// ---------- string / memory utils ----------

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, uint32_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }
    if (n == 0) return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

uint32_t strlen(const char* s) {
    uint32_t len = 0;
    while (s[len]) len++;
    return len;
}

void* memset(void* dest, int val, uint32_t count) {
    uint8_t* temp = (uint8_t*)dest;
    while (count--) {
        *temp++ = (uint8_t)val;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, uint32_t count) {
    uint8_t* dp = (uint8_t*)dest;
    const uint8_t* sp = (const uint8_t*)src;
    while (count--) {
        *dp++ = *sp++;
    }
    return dest;
}

void* k_memset16(void* dest, uint16_t val, uint32_t count) {
    uint16_t* temp = (uint16_t*)dest;
    while (count--) {
        *temp++ = val;
    }
    return dest;
}

void* calloc(uint32_t count, uint32_t size) {
    uint32_t total_size = count * size;
    void* ptr = malloc(total_size);
    if (ptr != NULL) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

// ---------- heap ----------

void init_dynamic_memory(uint32_t start_addr, uint32_t size) {
    heap_head = (memory_block_t*)start_addr;
    heap_head->size = size - sizeof(memory_block_t);
    heap_head->is_free = true;
    heap_head->next = NULL;
}

void* malloc(uint32_t size) {
    if (heap_head == NULL || size == 0) return NULL;

    size = align4(size);

    memory_block_t* current = heap_head;

    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            // Split only if enough room remains for another block
            if (current->size >= size + sizeof(memory_block_t) + 4) {
                memory_block_t* new_block =
                    (memory_block_t*)((uint32_t)current + sizeof(memory_block_t) + size);

                new_block->size = current->size - size - sizeof(memory_block_t);
                new_block->is_free = true;
                new_block->next = current->next;

                current->size = size;
                current->is_free = false;
                current->next = new_block;
            } else {
                current->is_free = false;
            }

            return (void*)((uint32_t)current + sizeof(memory_block_t));
        }

        current = current->next;
    }

    return NULL;
}

void free(void* ptr) {
    if (ptr == NULL) return;
    if (heap_head == NULL) return;

    memory_block_t* block =
        (memory_block_t*)((uint32_t)ptr - sizeof(memory_block_t));

    // basic sanity: avoid double free
    if (block->is_free) {
        return;
    }

    block->is_free = true;

    // merge all neighboring free blocks
    coalesce_free_blocks();
}

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

void debug_heap_dump() {
    memory_block_t* current = heap_head;
    int block_count = 0;

    terminal_print("--- KalsangOS Heap Map ---\n");
    terminal_print("ADDR\t\tSIZE\tSTATE\n");

    while (current != NULL) {
        terminal_print_number((uint32_t)current);
        terminal_print("\t");
        terminal_print_number(current->size);
        terminal_print("\t");

        if (current->is_free) {
            terminal_print("[FREE]\n");
        } else {
            terminal_print("[ACTIVE]\n");
        }

        current = current->next;
        block_count++;
    }

    terminal_print("Total Blocks: ");
    terminal_print_number(block_count);
    terminal_print("\n--------------------------\n");
}
