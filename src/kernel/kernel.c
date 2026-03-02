#include <stdint.h>
#include <stddef.h>

//VGA Color Constants
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

void kernel_main(void) 
{
    uint16_t* terminal_buffer = (uint16_t*) 0xB8000;
    uint8_t color = vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    
    for (int y = 0; y < 25; y++) {
        for (int x = 0; x < 80; x++) {
            const int index = y * 80 + x;
            terminal_buffer[index] = vga_entry(' ', color);
        }
    }

    const char* str = "TenzinOs Kernel v0.1 Loaded Successfully.";
    for (int i = 0; str[i] != '\0'; i++) {
        terminal_buffer[i] = vga_entry(str[i], color);
    }

    while (1);
}
