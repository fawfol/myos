#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

extern volatile uint32_t timer_ticks;

void init_timer(uint32_t frequency);
void timer_handler();

void sleep(uint32_t seconds);

void play_sound(uint32_t nFrequence);
void nosound();
void beep(uint32_t freq, uint32_t duration_ms);

#endif
