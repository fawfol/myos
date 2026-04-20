#include "gdt.h"
#include "memory.h"

//6 entries: NULL, Kernel Code, Kernel Data, User Code, User Data, TSS
struct gdt_entry_struct gdt_entries[6];
struct gdt_ptr_struct   gdt_ptr;
tss_entry_t             tss_entry;

//external assembly functions
extern void gdt_flush(uint32_t);
extern void tss_flush();

static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

static void write_tss(int32_t num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = base + sizeof(tss_entry);

    //add the TSS descriptor to the GDT
    gdt_set_gate(num, base, limit, 0xE9, 0x00);

    //ensure the TSS is initially zero'd
    memset(&tss_entry, 0, sizeof(tss_entry));

    tss_entry.ss0  = ss0;  //set the kernel stack segment
    tss_entry.esp0 = esp0; //set the kernel stack pointer

    // set the cs, ss, ds, es, fs and gs entries in the TSS
    //specifies what segments should be loaded when the processor switches to kernel mode 
    tss_entry.cs   = 0x0b;
    tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x13;
    tss_entry.iomap_base = sizeof(tss_entry);
}

//function to update the ESP0 when we switch tasks later on
void set_kernel_stack(uint32_t stack) {
    tss_entry.esp0 = stack;
}

void init_gdt() {
    gdt_ptr.limit = (sizeof(struct gdt_entry_struct) * 6) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    // 0: Null segment
    gdt_set_gate(0, 0, 0, 0, 0);                
    
    // 1: Kernel Code: Base 0, Limit 4GB, type: exe/read, ring 0
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); 
    
    // 2: Kernel Data: Base 0, Limit 4GB, type: read/write, ring 0
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); 

    // 3: User Code: Base 0, Limit 4GB, type: exe/read, ring 3
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    // 4: User Data: Base 0, Limit 4GB, type: read/write, ring 3
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    // 5: TSS
    // We pass 0x10 (Kernel Data Segment) as the SS0. ESP0 is 0 for now.
    write_tss(5, 0x10, 0x0);

    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush(); // Tell the CPU about the new TSS
}
