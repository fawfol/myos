#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include <stdbool.h>

#define KEY_ARROW_LEFT   0x80
#define KEY_ARROW_RIGHT  0x81
#define KEY_ARROW_UP     0x82
#define KEY_ARROW_DOWN   0x83
#define KEY_DELETE       0x84



void init_shell();
void shell_set_line_start();
void shell_handle_keypress(int c);
void terminal_print(const char* str);
void terminal_print_number(uint32_t num);
void terminal_print_hex(uint8_t value);
void terminal_clear();
void terminal_scroll();
void terminal_backspace();
void update_cursor(int index);

void execute_command();
void run_script(char* filename);

char* shell_readline();
void shell_update();

void run_kx_file(char* filename, char* args);

#endif
