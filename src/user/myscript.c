#include "kalsang_libc.h"

void _start() {
    print("--- KalsangOS Self-Hosting Program ---\n");

    char* filename = "auto.kx";
    char* content = "This is a test of persistent storage.";

    print("Step 1: Writing file to FAT32 disk...\n");
    // Syscall 23
    write_file(filename, content, 37); 

    print("Step 2: Verifying file existence...\n");
    char buffer[64];
    // Syscall 2
    read_file(filename, buffer);
    
    print("Content Read: ");
    print(buffer);
    print("\n");

    print("Step 3: Self-Hosting Success!\n");
    exit();
}
