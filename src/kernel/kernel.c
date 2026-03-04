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
#include "vfs.h"

void kernel_main(uint32_t mboot_ptr) {
    multiboot_info_t* mbi = (multiboot_info_t*)mboot_ptr;

    init_gdt();
    init_idt();
    pic_remap(0x20, 0x28);
    pic_enable_keyboard();
    init_timer(100);
    init_paging();

    //define where our heap starts (8MB is a safe zone)
    uint32_t heap_start = 0x00800000;
    uint32_t heap_size = (mbi->mem_upper * 1024) - (3 * 1024 * 1024);// extrmeemly precize for heap not running off physical ram //default 2MB fallback

    
    //pass real hardware values to the allocator
    init_dynamic_memory(heap_start, heap_size);

    init_shell();
    
    //check if GRUB loaded any modules (files)
	if (mbi->flags & MULTIBOOT_FLAG_MODS && mbi->mods_count > 0) {
		multiboot_module_t* mod = (multiboot_module_t*)mbi->mods_addr;
		
		terminal_print("Module 0 Start: ");
		terminal_print_number(mod->mod_start);
		terminal_print("\n");

		//check if there is actual data there ('ustar' magic string)
		char* magic = (char*)(mod->mod_start + 257); 
		if (magic[0] == 'u' && magic[1] == 's') {
		    terminal_print("TARmagic present\n");
		} else {
		    terminal_print("ERROR: Nno TAR magic found at module start\n");
		}

		init_ramdisk(mod->mod_start);
	}
	
    asm volatile("sti");

    while (1);
}

