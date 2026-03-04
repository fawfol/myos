#include "paging.h"
#include "shell.h"
//each table is 4096 bytes. 32 tables = 128KB of RAM.
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t page_tables[32][1024] __attribute__((aligned(4096))); 

void init_paging() {
    // 1. clear page dir
    for(int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002; //not present Read/Write
    }

    // 2. identity map the first 128MB
    //loop thru 32tables
    for(int t = 0; t < 32; t++) {
        for(int i = 0; i < 1024; i++) {
            //calculate the physical address for this specific page
            uint32_t address = (t * 1024 + i) * 4096;
            page_tables[t][i] = address | 3; //present Read/Write
        }
        //put this table into dir
        page_directory[t] = ((uint32_t)page_tables[t]) | 3;
    }

    // 3. enable paging 
    asm volatile(
        "mov %0, %%cr3\n\t"
        "mov %%cr0, %%eax\n\t"
        "or $0x80000000, %%eax\n\t"
        "mov %%eax, %%cr0"
        : : "r"(page_directory) : "eax"
    );
}
