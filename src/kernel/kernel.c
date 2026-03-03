#include <stdint.h>
#include <stddef.h>
#include "gdt.h"  
#include "idt.h"  
#include "pic.h" 

void kernel_main(void) 
{
    // 1. mem fouindatuon
    init_gdt();
    
    // 2. int Routing Table
    init_idt();
    
    // 3. hardware interfacing (remap Master to 32, slave to 40)
    pic_remap(0x20, 0x28);
    
    // 4. unmask the keyboard specifically
    pic_enable_keyboard();
    
    // 5. safely reenable CPU ints (sti)
    asm volatile("sti");

    //success message
    uint16_t* terminal_buffer = (uint16_t*) 0xB8000;
    const char* str = "KalsangOS : Hardware Interrupts Online";
    for (int i = 0; str[i] != '\0'; i++) {
        terminal_buffer[i] = (uint16_t) str[i] | (uint16_t) 0x0A << 8; //green text
    }

    while (1);
}
