#include "paging.h"
#include "shell.h"

// 1. declare the dir and table
//compiler attribute forces these arrays to start exactly on a 4KB boundary in RAM
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

void init_paging() {
    
    // 2. clear page dir
    // 0x02 sets"Read/Write" flag but leaves the "Present" flag as 0
    for(int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002;
    }

    // 3. identity map first 4 Mb of RAM
    // map virtual addr 0x00000000 directly to physical addr 0x00000000
    for(int i = 0; i < 1024; i++) {
        // i * 4096 gives physical address for this page
        // bitwise OR (| 3) to set bottom two bits to 1
        // bit 0 = "Present", Bit 1 = "Read/Write"
        first_page_table[i] = (i * 4096) | 3; 
    }

    // 4.put our completed page table into first slot of dir
    page_directory[0] = ((uint32_t)first_page_table) | 3;

    // 5. comm to direct cpu har
    asm volatile(
        // Step 1 load physical address of our dir into CR3 register
        "mov %0, %%cr3\n\t"
        
        // Step 2 read current state of the CR0 register
        "mov %%cr0, %%eax\n\t"
        
        // Step 3 flip 31st bit PG or Paging Enable bit to 1
        "or $0x80000000, %%eax\n\t"
        
        // Step 4 write altered state back to CR0 to activate VM
        "mov %%eax, %%cr0"
        : 
        : "r"(page_directory) 
        : "eax"
    );

    terminal_print("Memory Paging: ONLINE\n");
}
