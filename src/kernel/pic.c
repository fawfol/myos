#include "io.h"
#include <stdint.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define ICW1_INIT    0x10
#define ICW1_ICW4    0x01
#define ICW4_8086    0x01

//offset 1 is for the Master PIC (32 / 0x20)
//offset 2 is for the Slave PIC (40 / 0x28)
void pic_remap(int offset1, int offset2) {
    unsigned char a1, a2;

    //save current interrupt masks
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    //start initialization sequence
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4); io_wait();

    //definne new offsets
    outb(PIC1_DATA, offset1); io_wait();
    outb(PIC2_DATA, offset2); io_wait();

    //tell Master PIC there is a Slave PIC at IRQ2
    outb(PIC1_DATA, 4); io_wait();
    //tell Slave PIC its cascade identity
    outb(PIC2_DATA, 2); io_wait();

    //set both to 8086/88 (x86) mode
    outb(PIC1_DATA, ICW4_8086); io_wait();
    outb(PIC2_DATA, ICW4_8086); io_wait();

    //restore the saved masks
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

//enable Timer, Keyboard, and Mouse interrupts
void pic_enable_hardware() {
    // Master PIC (PIC1):
    // Bit 0: Timer (IRQ 0) = 0
    // Bit 1: Keyboard (IRQ 1) = 0
    // Bit 2: Cascade/Slave PIC (IRQ 2) = 0
    // Binary: 1111 1000 = 0xF8
    outb(PIC1_DATA, 0xF8);

    // Slave PIC (PIC2):
    // Bit 4: Mouse (IRQ 12) = 0
    // Binary: 1110 1111 = 0xEF
    outb(PIC2_DATA, 0xEF);
}
