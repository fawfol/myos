#include "kalsang_libc.h"

void __attribute__((section(".text.entry"))) _start(int argc, char** argv) {
    
    print("==================================\n");
    print(" KALSANG OS ARGC/ARGV TESTER\n");
    print("==================================\n");
    
    print("Program Name (argv[0]): ");
    print(argv[0]);
    print("\n");

    print("Arguments (argv[1]): ");
    print(argv[1]);
    print("\n");

    if (argv[1][0] == '\0') {
        print("No arguments were passed!\n");
    } else {
        print("Success! The kernel passed the arguments correctly.\n");
    }

    exit();

    for (;;);
}
