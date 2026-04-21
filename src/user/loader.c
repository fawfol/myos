#include "kalsang_libc.h"

void __attribute__((section(".text.entry"))) _start() {
    print("==================================\n");
    print(" KALSANG OS SYSTEM LOGIN\n");
    print("==================================\n");
    print("Enter password to launch 'hello.kx':\n> ");

    char buffer[64];
    //read from keyboard (FD 0)
    int bytes = read(0, buffer, 64);

    // Simple password check for "osdev"
    if (buffer[0] == 'o' && buffer[1] == 's' && buffer[2] == 'd' && 
        buffer[3] == 'e' && buffer[4] == 'v') {
        
        print("Access Granted! Spawning child process...\n");
        
        //execute the other program!
        exec("bin/hello.kx"); 
        
    } else {
        print("Access Denied. Terminating.\n");
        exit();
    }

    //keeps GCC happy
    for (;;);
}
