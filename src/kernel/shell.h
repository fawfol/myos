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

uint32_t emit_print(uint8_t* buf, char* msg);
uint32_t emit_sleep(uint8_t* buf, uint32_t sec);
uint32_t emit_beep(uint8_t* buf);
uint32_t emit_exit(uint8_t* buf);
void build_kx(char* filename, uint8_t* code, uint32_t size);
void compile_kx_from_file(char* src_filename, char* out_filename);
void compile_single_print_kx(char* filename, char* message);

#endif
