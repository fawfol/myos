#include "timer.h"
#include "io.h"

extern void get_mem_stats(uint32_t* used, uint32_t* free_mem);

void update_clock();

volatile uint32_t timer_ticks = 0;

void init_timer(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

void timer_handler() {
    timer_ticks++;
    
    //background task updation
    if (timer_ticks % 10 == 0) {
        update_clock();
    }
}


void sleep(uint32_t seconds) {
    uint32_t start_ticks = timer_ticks;
    uint32_t target_ticks = start_ticks + (seconds * 100); 
    
    while (timer_ticks < target_ticks) {
        asm volatile("sti"); 
        asm volatile("hlt"); 
    }
   
}
void update_clock() {
    uint16_t* vga = (uint16_t*)0xB8000;
    uint32_t used_mem = 0, free_mem = 0;
    get_mem_stats(&used_mem, &free_mem);

    // 1. Clear the left side of the dashboard (columns 0 to 39) so old text doesn't stick
    for (int i = 0; i < 40; i++) {
        vga[i] = (uint16_t)' ' | ((uint16_t)0x0F << 8);
    }

    // 2. Uptime Timer (Columns 40 to 53)
    uint32_t total_seconds = timer_ticks / 100;
    uint32_t s = total_seconds % 60;
    uint32_t m = (total_seconds / 60) % 60;
    uint32_t h = total_seconds / 3600;

    char time_str[] = " UP: 00:00:00 ";
    time_str[5] = '0' + (h / 10); time_str[6] = '0' + (h % 10);
    time_str[8] = '0' + (m / 10); time_str[9] = '0' + (m % 10);
    time_str[11] = '0' + (s / 10); time_str[12] = '0' + (s % 10);

    for (int i = 0; i < 14; i++) {
        vga[40 + i] = (uint16_t)time_str[i] | ((uint16_t)0x0E << 8); // Yellow
    }

    // 3. CPU Placeholder (Columns 54 to 64)
    char cpu_str[] = "| CPU: 05% ";
    for (int i = 0; i < 11; i++) {
        vga[54 + i] = (uint16_t)cpu_str[i] | ((uint16_t)0x0A << 8); // Light Green
    }

    // 4. Memory Usage (Columns 65 to 79) - Draws backwards from the right edge
    uint32_t temp = used_mem; 
    int pos = 79; 
    
    vga[pos--] = (uint16_t)'B' | ((uint16_t)0x0B << 8); // Light Cyan
    vga[pos--] = (uint16_t)' ' | ((uint16_t)0x0B << 8);

    if (temp == 0) {
        vga[pos--] = (uint16_t)'0' | ((uint16_t)0x0B << 8);
    }

    while (temp > 0 && pos >= 71) { // Leave room for "| MEM: " label
        vga[pos--] = (uint16_t)('0' + (temp % 10)) | ((uint16_t)0x0B << 8);
        temp /= 10;
    }

    // Add the "| MEM: " label
    char mem_lbl[] = "| MEM: ";
    for (int i = 6; i >= 0; i--) {
        vga[pos--] = (uint16_t)mem_lbl[i] | ((uint16_t)0x0B << 8);
    }
}
//play a sound at a specific frequency
void play_sound(uint32_t nFrequence) {
    uint32_t Div;
    uint8_t tmp;

    //set the PIT to the desired frequency
    Div = 1193180 / nFrequence;
    outb(0x43, 0xb6);
    outb(0x42, (uint8_t) (Div) );
    outb(0x42, (uint8_t) (Div >> 8));

    //play the sound using the PC speaker
    tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
}

//shut up the speaker
void nosound() {
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

//simple beep
void beep(uint32_t freq, uint32_t duration_ms) {
    play_sound(freq);
    sleep(duration_ms / 10); //convert ms to ticks (assuming 100Hz)
    nosound();
}
