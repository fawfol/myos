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
#include "ata.h"
#include "fat32.h"

typedef struct
{
    uint8_t status;
    uint8_t chs_first[3];
    uint8_t type;
    uint8_t chs_last[3];
    uint32_t lba_start;
    uint32_t sector_count;
} __attribute__((packed)) mbr_partition_t;


void kernel_main(uint32_t mboot_ptr) {
    multiboot_info_t* mbi = (multiboot_info_t*)mboot_ptr;

    init_gdt();
    init_idt();
    init_syscalls();
    pic_remap(0x20, 0x28);
    pic_enable_hardware();
    ata_init();
		uint8_t buffer[512];

	terminal_print("Reading sector...\n");

	ata_read_sector(0, buffer);

	/* ---- MBR signature check ---- */

	uint16_t signature = buffer[510] | (buffer[511] << 8);

	terminal_print("Boot sig: ");
	terminal_print_hex(signature);
	terminal_print("\n");

	/* ---- Partition table ---- */
    mbr_partition_t* p = (mbr_partition_t*)(buffer + 446);

    for(int i=0;i<4;i++)
	{
		terminal_print("Partition ");
		terminal_print_hex(i);
		terminal_print("\n");

		terminal_print("Type: ");
		terminal_print_hex(p[i].type);
		terminal_print("\n");

		terminal_print("Start LBA: ");
		terminal_print_number(p[i].lba_start);
		terminal_print("\n\n");

        //type 0x0B and 0x0C are FAT32 with LBA
        if (p[i].type == 0x0B || p[i].type == 0x0C) {
            init_fat32(p[i].lba_start);
            break; //found our drive so stop searching the MBR
        }
	
	sleep(1);
    
    extern void init_mouse();
    init_mouse();
    
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
		    terminal_print("TAR magic present\n");
		} else {
		    terminal_print("ERROR: No TAR magic found at module start\n");
		}

		init_ramdisk(mod->mod_start);
	}
	
    asm volatile("sti");

	while (1) {
        shell_update(); // Check if the user pressed enter
        asm volatile("hlt"); // Rest the CPU until the next interrupt
    }
}

