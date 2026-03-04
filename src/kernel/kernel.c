#include <stdint.h>
#include <stddef.h>
#include "gdt.h"  
#include "idt.h"  
#include "pic.h"  
#include "shell.h" 
#include "timer.h"
#include "paging.h"
#include "memory.h"

void kernel_main(uint32_t mboot_ptr) 
{
    //1. mem foundation
    init_gdt();
    
    //2. int routing table
    init_idt();
    
    //3. hardware interfacing 
    pic_remap(0x20, 0x28);
    pic_enable_keyboard();
    
    //4. start the system clock 100 Hz
    init_timer(100); 
    
    //5. vm enable
    init_paging();
    //start heap allocator
    init_dynamic_memory();
    
    //6. start the ui
    init_shell();

    //7. enable hardware ints
    asm volatile("sti");

    //kernel loop
    while (1);
}
