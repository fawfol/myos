#include <stdint.h>
#include <string.h>
#include "idt.h"

struct idt_entry_struct idt_entries[256];
struct idt_ptr_struct   idt_ptr;

//external assembly function to load idt
extern void idt_flush(uint32_t);
extern void isr33();
extern void isr32();

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    //0x8E = 10001110 :: present : ring 0 : lower bits 1110 for 32-bit int gate
    idt_entries[num].flags   = flags;
}

void init_idt() {
    idt_ptr.limit = sizeof(struct idt_entry_struct) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    for(int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // Map the Timer (IRQ 0) to Interrupt 32
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E);
    // Map the Keyboard (IRQ 1) to Interrupt 33
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E);

    idt_flush((uint32_t)&idt_ptr);
}
