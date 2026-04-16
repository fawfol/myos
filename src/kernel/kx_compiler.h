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
void build_kx(char* filename, uint8_t* code, uint32_t code_size, uint8_t* data, uint32_t data_size);

uint32_t emit_prologue(uint8_t* buf, uint32_t code_size);
uint32_t emit_store_imm(uint8_t* buf, uint32_t data_offset, uint32_t value);
uint32_t emit_add_imm(uint8_t* buf, uint32_t data_offset, uint32_t value);
uint32_t emit_sub_imm(uint8_t* buf, uint32_t data_offset, uint32_t value);
uint32_t emit_print_var(uint8_t* buf, uint32_t data_offset);
uint32_t emit_sleep_var(uint8_t* buf, uint32_t data_offset);
uint32_t emit_push_var(uint8_t* buf, uint32_t data_offset);
uint32_t emit_pop_var(uint8_t* buf, uint32_t data_offset);
uint32_t emit_mov_var(uint8_t* buf, uint32_t dst_offset, uint32_t src_offset);
uint32_t emit_cmp_zero(uint8_t* buf, uint32_t data_offset);
uint32_t emit_add_var(uint8_t* buf, uint32_t dst_offset, uint32_t src_offset);
uint32_t emit_sub_var(uint8_t* buf, uint32_t dst_offset, uint32_t src_offset);
uint32_t emit_cmp_var_var(uint8_t* buf, uint32_t left_offset, uint32_t right_offset);
uint32_t emit_cmp_var_imm(uint8_t* buf, uint32_t left_offset, uint32_t imm);
uint32_t emit_je(uint8_t* buf, int32_t rel);
uint32_t emit_jl(uint8_t* buf, int32_t rel);
uint32_t emit_jg(uint8_t* buf, int32_t rel);
uint32_t emit_call(uint8_t* buf, int32_t rel);
uint32_t emit_ret(uint8_t* buf);

#endif
