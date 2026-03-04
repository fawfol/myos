#include "paging.h"

//move tables far away from the action (16MB mark)
uint32_t* page_directory = (uint32_t*)0x01000000; 
uint32_t* page_tables    = (uint32_t*)0x01001000; 

void init_paging() {
    //clear dir
    for(int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002; 
    }

    //identity map 256MB
    for(int i = 0; i < 65536; i++) {
        // Map Virtual (i*4096) to Physical (i*4096)
        page_tables[i] = (uint32_t)(i * 4096) | 3; 
    }

    //link dir to tables
    for(int t = 0; t < 64; t++) {
        //tables are spaced 4096 bytes apart in physical RAM
        uint32_t phys_table_addr = 0x01001000 + (t * 4096);
        page_directory[t] = phys_table_addr | 3;
    }

    //enable pagin(Point CR3 to 16MB)
    asm volatile(
        "mov %0, %%cr3\n\t"
        "mov %%cr0, %%eax\n\t"
        "or $0x80000000, %%eax\n\t"
        "mov %%eax, %%cr0"
        : : "r"(0x01000000) : "eax"
    );
}
