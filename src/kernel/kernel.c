#include <stdint.h>
#include <stddef.h>
#include "gdt.h"  
#include "idt.h"  
#include "pic.h"  
#include "shell.h" 
#include "timer.h"
#include "paging.h"
#include "memory.h"
#include "multiboot.h"

void kernel_main(uint32_t mboot_ptr) {
    multiboot_info_t* mbi = (multiboot_info_t*)mboot_ptr;

    init_gdt();
    init_idt();
    pic_remap(0x20, 0x28);
    pic_enable_keyboard();
    init_timer(100);
    init_paging(); //now maps 128MB!

    //define where our heap starts (4MB is a safe zone)
    uint32_t heap_start = 0x00400000; 
    uint32_t heap_size = (mbi->mem_upper * 1024) - (3 * 1024 * 1024);// extrmeemly precize for heap not running off physical ram //default 2MB fallback

    //check if Multiboot gave us valid memory info
    if (mbi->flags & MULTIBOOT_FLAG_MEM) {
        //mem_upper is KB above 1MB mark
        heap_size = mbi->mem_upper * 1024; 
    }

    //pass real hardware values to the allocator
    init_dynamic_memory(heap_start, heap_size);

    init_shell();
    asm volatile("sti");

    while (1);
}

