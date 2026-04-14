#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include <stdbool.h>

void init_shell();
void shell_handle_keypress(char c);
void terminal_print(const char* str);
void terminal_print_number(uint32_t num);
void terminal_print_hex(uint8_t value);
void terminal_clear();


void execute_command();
void run_script(char* filename);
void terminal_scroll();

char* shell_readline();
void shell_update();

void run_kx_file(char* filename);

#endif
