#include <stdint.h>
#include <stddef.h>
#include "gdt.h"  
#include "idt.h"  
#include "pic.h"  
#include "shell.h"

void kernel_main(void) 
{
    init_gdt();
    init_idt();
    pic_remap(0x20, 0x28);
    pic_enable_keyboard();
    
    //start the terminal and shell
    init_shell();

    asm volatile("sti");

    while (1);
}
