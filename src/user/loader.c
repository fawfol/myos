#include "kalsang_libc.h"

static char buffer[256];
static char filename[] = "test.txt";

void _start() {
    print("User-Land (C): Reading test.txt...\n");

    int bytes = read_file(filename, buffer);

    if (bytes > 0) {
        if (bytes >= 255) {
            bytes = 255;
        }

        buffer[bytes] = '\0';

        print("File content:\n");
        print("------------------\n");
        print(buffer);
        print("\n------------------\n");
    } else {
        print("Error reading file\n");
    }

    exit();

    // keep compiler happy in freestanding mode
    for (;;);
}
