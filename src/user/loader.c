#include "kalsang_libc.h"

// Remember: We must use our VIP entry section!
void __attribute__((section(".text.entry"))) _start() {
    
    print("==================================\n");
    print(" KALSANG OS INTERACTIVE TERMINAL\n");
    print("==================================\n");
    print("What is your name, User?\n> ");

    // Allocate a buffer for the user's input
    char name_buffer[64];
    
    // Read from FD 0 (STDIN). This will pause execution until you hit Enter!
    int bytes = read(0, name_buffer, 64);

    print("Hello, ");
    print(name_buffer);
    print("! You are executing code in Ring 3!\n");

    exit();

    for (;;);
}
