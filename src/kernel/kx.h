#ifndef KX_H
#define KX_H

#include <stdint.h>

#define KX_MAGIC 0x314B584B  // "KXK1" (little endian)

typedef struct {
    uint32_t magic;       // must be KX_MAGIC
    uint32_t entry;       // entry offset
    uint32_t code_size;   // size of code section
    uint32_t data_size;   // (unused for now)
} __attribute__((packed)) kx_header_t;

#endif
