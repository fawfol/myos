#include "kalsang_libc.h"

void _start() {
    print("User-Land: Requesting dynamic memory via malloc()...\n");

    //ask the kernel for 32 bytes of heap memory
    char* dynamic_string = (char*)malloc(32);

    if (dynamic_string == 0) {
        print("Error: User malloc failed!\n");
        exit();
    }

    //write into our newly allocated heap
    dynamic_string[0] = 'H';
    dynamic_string[1] = 'e';
    dynamic_string[2] = 'a';
    dynamic_string[3] = 'p';
    dynamic_string[4] = ' ';
    dynamic_string[5] = 'W';
    dynamic_string[6] = 'o';
    dynamic_string[7] = 'r';
    dynamic_string[8] = 'k';
    dynamic_string[9] = 's';
    dynamic_string[10] = '!';
    dynamic_string[11] = '\n';
    dynamic_string[12] = '\0';

    print("Success! Data stored in user heap: ");
    print(dynamic_string);

    exit();
    for (;;);
}
