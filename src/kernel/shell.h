#include <stdbool.h>
#ifndef SHELL_H
#define SHELL_H


void init_shell();
void shell_handle_keypress(char c);
void terminal_print(const char* str);
void terminal_clear();
void terminal_print_number(uint32_t num);

char* shell_readline();
void shell_update();

#endif
