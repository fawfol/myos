#include "kalsang_libc.h"

void _start() {
    char* filename = "test.txt";
    char buffer[128]; //small buffer to hold the file content
    
    print("User-Land: Attempting to read 'test.txt'...\n");

    //call Syscall 2 (SYS_READ)
    int bytes_read = read_file(filename, buffer);

    if (bytes_read > 0) {
        //ensure the string is null-terminated for printing
        buffer[bytes_read] = '\0'; 
        print("User-Land: File Content Follows:\n");
        print("------------------------------\n");
        print(buffer);
        print("\n------------------------------\n");
    } else {
        print("User-Land Error: Could not read file.\n");
    }

    //retrn control to the OS (halt for now)
    while(1);
}
