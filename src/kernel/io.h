#ifndef IO_H
#define IO_H

#include "timer.h"
#include <stdint.h>

//send a byte to a hardware port
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

//read a byte from a hardware port
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

//tiny delay to give older hardware time to process commands
static inline void io_wait(void) {
    outb(0x80, 0);
}

//send a 16-bit word to a hardware port
static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// Just the blueprints! No code blocks here.
void play_sound(uint32_t nFrequence);
void nosound();
void beep(uint32_t freq, uint32_t duration_ms);

void reboot();
void shutdown();

#endif
