void _start() {
    char* msg = "KalsangOS Syscall Test: SUCCESS\n";
    
    //trigger Syscall 1 (SYS_PRINT)
    //pass the message pointer in EBX
    asm volatile (
        "mov $1, %%eax\n\t"
        "mov %0, %%ebx\n\t"
        "int $0x80"
        : : "r"(msg) : "eax", "ebx"
    );

    //stay alive or return
    while(1); 
}
