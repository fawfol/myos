#ifndef IO_H
#define IO_H
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

#endif
