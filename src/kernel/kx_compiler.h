#ifndef KX_COMPILER_H
#define KX_COMPILER_H

#include <stdint.h>

void compile_kx_from_file(char* src_filename, char* out_filename);
void compile_single_print_kx(char* filename, char* message);

uint32_t emit_print(uint8_t* buf, char* msg);
uint32_t emit_print_num(uint8_t* buf, uint32_t value);
uint32_t emit_sleep(uint8_t* buf, uint32_t sec);
uint32_t emit_beep(uint8_t* buf);
uint32_t emit_exit(uint8_t* buf);
void build_kx(char* filename, uint8_t* code, uint32_t size);

#endif
