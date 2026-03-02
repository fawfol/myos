#include <stdint.h>

// Structure for a GDT entry
struct gdt_entry_struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

//structure for the GDT pointer
struct gdt_ptr_struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

//3 enrtries:  NULL  Code   Data
struct gdt_entry_struct gdt_entries[3];
struct gdt_ptr_struct   gdt_ptr;

// eternal function from boot.s
extern void gdt_flush(uint32_t);

//internal helper to set GDT gate values
static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

void init_gdt() {
    gdt_ptr.limit = (sizeof(struct gdt_entry_struct) * 3) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    //1 null segment req
    gdt_set_gate(0, 0, 0, 0, 0);                
    
    //2 cs: Base 0, Limit 4GB, type: exe/read, ring 0
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); 
    
    //3ds: Base 0, Limit 4GB, Type: read/write, ring 0
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); 

    gdt_flush((uint32_t)&gdt_ptr);
}
