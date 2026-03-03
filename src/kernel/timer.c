#include "timer.h"
#include "io.h"

uint32_t timer_ticks = 0;

void init_timer(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

void timer_handler() {
    timer_ticks++;
}


void sleep(uint32_t seconds) {
    uint32_t start_ticks = timer_ticks;
    uint32_t target_ticks = start_ticks + (seconds * 100); 
    
    while (timer_ticks < target_ticks) {
        asm volatile("sti"); 
        asm volatile("hlt"); 
    }
}
