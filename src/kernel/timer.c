#include "timer.h"
#include "io.h"

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
    
    //clear the area first (column 70 to 79)
    for(int i = 70; i < 80; i++) {
        vga[i] = (uint16_t)' ' | (uint16_t)0x0F << 8;
    }

    //convert the tick count to a string manually use a simplified version of your print_number
    uint32_t temp = timer_ticks / 100; //show seconds (roughly)
    int pos = 79;
    
    if (temp == 0) {
        vga[pos] = (uint16_t)'0' | (uint16_t)0x0E << 8;
    }

    while (temp > 0 && pos > 70) {
        vga[pos--] = (uint16_t)('0' + (temp % 10)) | (uint16_t)0x0E << 8;
        temp /= 10;
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
