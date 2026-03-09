#include "io.h"
#include <stdint.h>

extern void terminal_scroll_up();
extern void terminal_scroll_down();

void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) { if ((inb(0x64) & 1) == 1) return; } 
    } else {
        while (timeout--) { if ((inb(0x64) & 2) == 0) return; } 
    }
}

void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(0x64, 0xD4); 
    mouse_wait(1);
    outb(0x60, write); 
}

uint8_t mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}

// --- MOUSE INITIALIZATION ---
void init_mouse() {
    uint8_t status;

    // 1.enable auxiliary mouse device
    mouse_wait(1);
    outb(0x64, 0xA8);
    
    // 2.read Compaq Status Byte
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    status = inb(0x60);
    
    // 3.sSet Bit 0 (Keyboard IRQ), Set Bit 1 (Mouse IRQ), Clear Bit 5 (Mouse Clock)
    status = (status | 3) & ~0x20; 
    
    // 4.write back
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);
    
    // 5.default Settings
    mouse_write(0xF6); mouse_read();
    
    // 6.  MAGIC SEQUENCE: Unlock Scroll Wheel (Z-Axis)
    mouse_write(0xF3); mouse_read(); mouse_write(200); mouse_read();
    mouse_write(0xF3); mouse_read(); mouse_write(100); mouse_read();
    mouse_write(0xF3); mouse_read(); mouse_write(80);  mouse_read();
    
    // 7.eable data reporting
    mouse_write(0xF4); mouse_read(); 
}

// --- MOUSE INTERRUPT HANDLER ---
uint8_t mouse_cycle = 0;
int8_t mouse_byte[4]; 

void mouse_handler() {
    uint8_t status = inb(0x64);
    
    //drain buffer as long as there is data
    while (status & 0x01) { 
        if (status & 0x20) { //is it a mouse byte?
            uint8_t d = inb(0x60); 
            
            //build the 4-byte packet
            switch(mouse_cycle) {
                case 0:
                    if (d & 0x08) { //valid packet always has bit 3 set
                        mouse_byte[0] = d;
                        mouse_cycle++;
                    }
                    break;
                case 1:
                    mouse_byte[1] = d; mouse_cycle++; break; //X movement
                case 2:
                    mouse_byte[2] = d; mouse_cycle++; break; //Y movement
                case 3:
                    mouse_byte[3] = d; //Z-axis! (Scroll Wheel)
                    mouse_cycle = 0;   //reset for next packet
                    
                    //trigger the hardware scrolling
                    if (mouse_byte[3] > 0) {
                        terminal_scroll_up();
                    } else if (mouse_byte[3] < 0) {
                        terminal_scroll_down();
                    }
                    break;
            }
        } else {
            break; //keyboard bytes
        }
        status = inb(0x64); 
    }

    outb(0xA0, 0x20); 
    outb(0x20, 0x20); 
}
