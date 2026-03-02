#include <stdint.h>

struct idt_entry_struct {
    uint16_t base_lo;             //lower 16 bits of the address to jump to
    uint16_t sel;                 //kernel segment selector 0x08 from gdt
    uint8_t  always0;             //must be zero
    uint8_t  flags;               //flags:IDT Gate type : Privilege level
    uint16_t base_hi;             //upper 16 bits of the address
} __attribute__((packed));

struct idt_ptr_struct {
    uint16_t limit;
    uint32_t base;                //address of the first element in idt_entry_t array
} __attribute__((packed));

void init_idt();
