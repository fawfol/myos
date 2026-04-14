#include <stdint.h>
#include "kx_compiler.h"
#include "kx.h"
#include "memory.h"
#include "shell.h"
#include "vfs.h"

typedef struct {
    char name[32];
    uint32_t value;
    uint32_t data_offset;
} kx_var_t;

typedef struct {
    char name[32];
    uint32_t offset;
} kx_label_t;

uint32_t str_to_uint(char* str) {
    uint32_t res = 0;
    for (int i = 0; str[i] >= '0' && str[i] <= '9'; i++) {
        res = res * 10 + (str[i] - '0');
    }
    return res;
}

int find_label(kx_label_t* labels, int count, char* name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(labels[i].name, name) == 0)
            return i;
    }
    return -1;
}

void set_label(kx_label_t* labels, int* count, char* name, uint32_t offset) {
    if (*count < 64) {
        memset(labels[*count].name, 0, 32);

        uint32_t len = strlen(name);
        if (len > 31) len = 31;

        memcpy(labels[*count].name, name, len);
        labels[*count].name[len] = '\0';
        labels[*count].offset = offset;
        (*count)++;
    }
}

int is_number(char* s) {
    if (!s || s[0] == '\0') return 0;

    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9') return 0;
    }
    return 1;
}

int find_var(kx_var_t* vars, int var_count, char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void set_var(kx_var_t* vars, int* var_count, char* name, uint32_t value) {
    int idx = find_var(vars, *var_count, name);

    if (idx >= 0) {
        vars[idx].value = value;
        return;
    }

    if (*var_count < 64) {
        memset(vars[*var_count].name, 0, 32);

        uint32_t len = strlen(name);
        if (len > 31) len = 31;

        memcpy(vars[*var_count].name, name, len);
        vars[*var_count].name[len] = '\0';

        vars[*var_count].value = value;
        (*var_count)++;
    }
}

void assign_var_offsets(kx_var_t* vars, int var_count) {
    for (int i = 0; i < var_count; i++) {
        vars[i].data_offset = i * 4;
    }
}

uint32_t emit_sleep(uint8_t* buf, uint32_t sec) {
    uint8_t code[] = {
        0xBB,0,0,0,0,
        0xB8,5,0,0,0,
        0xCD,0x80
    };

    memcpy(buf, code, sizeof(code));
    memcpy(buf + 1, &sec, 4);

    return sizeof(code);
}

uint32_t emit_beep(uint8_t* buf) {
    uint8_t code[] = {
        0xB8,6,0,0,0,
        0xCD,0x80
    };

    memcpy(buf, code, sizeof(code));
    return sizeof(code);
}

uint32_t emit_print(uint8_t* buf, char* msg) {
    uint32_t len = strlen(msg) + 1;

    uint8_t template[] = {
        0xE8,0,0,0,0,        // call next
        0x5E,                // pop esi
        0x8D,0x9E,0,0,0,0,   // lea disp32(%esi), %ebx
        0xB8,1,0,0,0,        // mov eax, 1
        0xCD,0x80,           // int 0x80
        0xE9,0,0,0,0         // jmp over message
    };

    memcpy(buf, template, sizeof(template));

    uint32_t disp_to_msg = sizeof(template) - 5;
    memcpy(buf + 8, &disp_to_msg, 4);

    uint32_t jump_over_msg = len;
    memcpy(buf + 20, &jump_over_msg, 4);

    memcpy(buf + sizeof(template), msg, len);

    return sizeof(template) + len;
}

uint32_t emit_print_num(uint8_t* buf, uint32_t value) {
    char num_str[16];
    int i = 0;

    if (value == 0) {
        num_str[i++] = '0';
    } else {
        char temp[16];
        int j = 0;

        while (value > 0) {
            temp[j++] = (value % 10) + '0';
            value /= 10;
        }

        while (j > 0) {
            num_str[i++] = temp[--j];
        }
    }

    num_str[i] = '\0';
    return emit_print(buf, num_str);
}

uint32_t emit_prologue(uint8_t* buf, uint32_t code_size) {
    (void)code_size;

    // push ebx
    // push esi
    // push edi
    // push ebp
    // call next
    // pop edx
    // add edx, imm32   <-- patched later with (final_code_size - 9)
    uint8_t code[] = {
        0x53,                   // push ebx
        0x56,                   // push esi
        0x57,                   // push edi
        0x55,                   // push ebp
        0xE8, 0,0,0,0,          // call next
        0x5A,                   // pop edx
        0x81, 0xC2, 0,0,0,0     // add edx, imm32
    };

    memcpy(buf, code, sizeof(code));
    return sizeof(code); // 16 bytes
}

uint32_t emit_store_imm(uint8_t* buf, uint32_t data_offset, uint32_t value) {
    // mov dword ptr [edx+disp32], imm32
    buf[0] = 0xC7;
    buf[1] = 0x82;
    memcpy(buf + 2, &data_offset, 4);
    memcpy(buf + 6, &value, 4);
    return 10;
}

uint32_t emit_add_imm(uint8_t* buf, uint32_t data_offset, uint32_t value) {
    // add dword ptr [edx+disp32], imm32
    buf[0] = 0x81;
    buf[1] = 0x82;
    memcpy(buf + 2, &data_offset, 4);
    memcpy(buf + 6, &value, 4);
    return 10;
}

uint32_t emit_sub_imm(uint8_t* buf, uint32_t data_offset, uint32_t value) {
    // sub dword ptr [edx+disp32], imm32
    buf[0] = 0x81;
    buf[1] = 0xAA;
    memcpy(buf + 2, &data_offset, 4);
    memcpy(buf + 6, &value, 4);
    return 10;
}

uint32_t emit_print_var(uint8_t* buf, uint32_t data_offset) {
    // mov ebx, [edx+disp32]
    // mov eax, 7
    // int 0x80
    uint8_t code[] = {
        0x8B, 0x9A, 0,0,0,0,
        0xB8, 0x07,0x00,0x00,0x00,
        0xCD, 0x80
    };

    memcpy(buf, code, sizeof(code));
    memcpy(buf + 2, &data_offset, 4);

    return sizeof(code);
}

uint32_t emit_sleep_var(uint8_t* buf, uint32_t data_offset) {
    // mov ebx, [edx+disp32]
    // mov eax, 5
    // int 0x80
    uint8_t code[] = {
        0x8B, 0x9A, 0,0,0,0,
        0xB8, 0x05,0x00,0x00,0x00,
        0xCD, 0x80
    };

    memcpy(buf, code, sizeof(code));
    memcpy(buf + 2, &data_offset, 4);

    return sizeof(code);
}

uint32_t emit_cmp_zero(uint8_t* buf, uint32_t data_offset) {
    // cmp dword ptr [edx+disp32], 0
    buf[0] = 0x81;
    buf[1] = 0xBA;
    memcpy(buf + 2, &data_offset, 4);

    uint32_t zero = 0;
    memcpy(buf + 6, &zero, 4);

    return 10;
}

uint32_t emit_je(uint8_t* buf, int32_t rel) {
    // 0F 84 rel32
    buf[0] = 0x0F;
    buf[1] = 0x84;
    memcpy(buf + 2, &rel, 4);
    return 6;
}

uint32_t emit_jump(uint8_t* buf, int32_t rel) {
    buf[0] = 0xE9;
    memcpy(buf + 1, &rel, 4);
    return 5;
}

uint32_t emit_exit(uint8_t* buf) {
    uint8_t code[] = {
        0xB8,4,0,0,0,   // mov eax, 4
        0xCD,0x80,      // int 0x80
        0x5D,           // pop ebp
        0x5F,           // pop edi
        0x5E,           // pop esi
        0x5B,           // pop ebx
        0xC3            // ret
    };

    memcpy(buf, code, sizeof(code));
    return sizeof(code); // 12 bytes
}

void build_kx(char* filename, uint8_t* code, uint32_t code_size, uint8_t* data, uint32_t data_size) {
    uint32_t total = sizeof(kx_header_t) + code_size + data_size;

    uint8_t* bin = (uint8_t*)malloc(total);
    if (!bin) {
        terminal_print("Compiler Error: Out of memory\n");
        return;
    }

    kx_header_t* header = (kx_header_t*)bin;
    header->magic = KX_MAGIC;
    header->entry = 0;
    header->code_size = code_size;
    header->data_size = data_size;

    memcpy(bin + sizeof(kx_header_t), code, code_size);

    if (data_size > 0) {
        memcpy(bin + sizeof(kx_header_t) + code_size, data, data_size);
    }

    vfs_node_t* existing = vfs_find(vfs_root, filename);
    if (existing) {
        void* new_copy = malloc(total);
        if (!new_copy) {
            free(bin);
            terminal_print("Compiler Error: Out of memory\n");
            return;
        }

        memcpy(new_copy, bin, total);
        existing->ptr = (vfs_node_t*)new_copy;
        existing->length = total;
    } else {
        vfs_create(filename, (char*)bin, total);
    }

    free(bin);
}

void compile_single_print_kx(char* filename, char* message) {
    if (!filename || !message || filename[0] == '\0' || message[0] == '\0') {
        terminal_print("Usage: mkhello <file> <message>\n");
        return;
    }

    uint8_t* code = (uint8_t*)malloc(4096);
    if (!code) {
        terminal_print("Error: Out of memory\n");
        return;
    }

    uint32_t offset = 0;
    offset += emit_print(code + offset, message);
    offset += emit_exit(code + offset);

    build_kx(filename, code, offset, 0, 0);
    free(code);

    terminal_print("Built executable: ");
    terminal_print(filename);
    terminal_print("\n");
}

void compile_kx_from_file(char* src_filename, char* out_filename) {
    vfs_node_t* src = vfs_find(vfs_root, src_filename);
    if (!src) {
        terminal_print("Source not found\n");
        return;
    }

    char* data = (char*)src->ptr;
    uint8_t* code = (uint8_t*)malloc(4096);
    if (!code) {
        terminal_print("Error: Out of memory\n");
        return;
    }

    uint32_t offset = 0;
    char line[256];
    int idx = 0;

    kx_var_t vars[64];
    int var_count = 0;

    kx_label_t labels[64];
    int label_count = 0;

    memset(vars, 0, sizeof(vars));
    memset(labels, 0, sizeof(labels));

    // =========================
    // PASS 1: labels + sizes
    // =========================
    uint32_t temp_offset = 16;
    idx = 0;

    for (uint32_t i = 0; i < src->length; i++) {
        char c = data[i];

        if (c == '\n' || i == src->length - 1) {
            if (c != '\n' && c != '\r' && idx < 255) {
                line[idx++] = c;
            }
            line[idx] = '\0';

            int len = strlen(line);

            // label
            if (len > 0 && line[len - 1] == ':') {
                line[len - 1] = '\0';
                set_label(labels, &label_count, line, temp_offset);
            }

            // let
            else if (strncmp(line, "let ", 4) == 0) {
                char* p = line + 4;

                while (*p == ' ') p++;
                char* name = p;

                while (*p != '\0' && *p != ' ') p++;

                if (*p != '\0') {
                    *p = '\0';
                    p++;

                    while (*p == ' ') p++;
                    char* value_str = p;

                    if (name[0] != '\0' && value_str[0] != '\0' && is_number(value_str)) {
                        set_var(vars, &var_count, name, str_to_uint(value_str));
                        temp_offset += 10;
                    }
                }
            }
            
            // add
            else if (strncmp(line, "add ", 4) == 0) {
                char* p = line + 4;

                while (*p == ' ') p++;
                char* name = p;

                while (*p != '\0' && *p != ' ') p++;

                if (*p != '\0') {
                    *p = '\0';
                    p++;

                    while (*p == ' ') p++;
                    char* arg = p;

                    int dst = find_var(vars, var_count, name);
                    if (dst >= 0) {
                        if (is_number(arg)) {
                            temp_offset += 10;
                        }
                    }
                }
            }

            // sub
            else if (strncmp(line, "sub ", 4) == 0) {
                char* p = line + 4;

                while (*p == ' ') p++;
                char* name = p;

                while (*p != '\0' && *p != ' ') p++;

                if (*p != '\0') {
                    *p = '\0';
                    p++;

                    while (*p == ' ') p++;
                    char* arg = p;

                    int dst = find_var(vars, var_count, name);
                    if (dst >= 0) {
                        if (is_number(arg)) {
                            temp_offset += 10;
                        }
                    }
                }
            }

            // print
            else if (strncmp(line, "print ", 6) == 0) {
                char* msg = line + 6;
                while (*msg == ' ') msg++;

                int var_idx = find_var(vars, var_count, msg);
                if (var_idx >= 0) {
                    temp_offset += 13;
                } else {
                    temp_offset += 24 + strlen(msg) + 1;
                }
            }

            // sleep
            else if (strncmp(line, "sleep ", 6) == 0) {
                char* arg = line + 6;
                while (*arg == ' ') arg++;

                if (is_number(arg)) {
                    temp_offset += 12;
                } else {
                    int var_idx = find_var(vars, var_count, arg);
                    if (var_idx >= 0) {
                        temp_offset += 13;
                    }
                }
            }

            // beep
            else if (strcmp(line, "beep") == 0) {
                temp_offset += 7;
            }

            // jump
            else if (strncmp(line, "jump ", 5) == 0) {
                temp_offset += 5;
            }
            
            else if (strncmp(line, "ifzero ", 7) == 0) {
                char* p = line + 7;

                while (*p == ' ') p++;
                char* varname = p;

                while (*p != '\0' && *p != ' ') p++;

                if (*p != '\0') {
                    *p = '\0';
                    p++;

                    while (*p == ' ') p++;
                    char* label = p;

                    if (varname[0] != '\0' && label[0] != '\0') {
                        temp_offset += 10 + 6; // cmp + je
                    }
                }
            }

            idx = 0;
        } else if (c != '\r') {
            if (idx < 255) {
                line[idx++] = c;
            }
        }
    }
	    assign_var_offsets(vars, var_count);

    uint32_t data_size = var_count * 4;
    uint8_t* data_section = 0;

    if (data_size > 0) {
        data_section = (uint8_t*)malloc(data_size);
        if (!data_section) {
            free(code);
            terminal_print("Error: Out of memory\n");
            return;
        }

        memset(data_section, 0, data_size);

        for (int i = 0; i < var_count; i++) {
            memcpy(data_section + vars[i].data_offset, &vars[i].value, 4);
        }
    }


    // =========================
    // PASS 2: real codegen
    // =========================
    idx = 0;
    offset = 0;
    offset += emit_prologue(code + offset, temp_offset);

    for (uint32_t i = 0; i < src->length; i++) {
        char c = data[i];

        if (c == '\n' || i == src->length - 1) {
            if (c != '\n' && c != '\r' && idx < 255) {
                line[idx++] = c;
            }
            line[idx] = '\0';

            int len = strlen(line);

            // skip labels in pass 2
            if (len > 0 && line[len - 1] == ':') {
                idx = 0;
                continue;
            }

            // let
            if (strncmp(line, "let ", 4) == 0) {
                char* p = line + 4;

                while (*p == ' ') p++;
                char* name = p;

                while (*p != '\0' && *p != ' ') p++;

                if (*p == '\0') {
                    terminal_print("Compiler Error: Invalid let syntax\n");
                    idx = 0;
                    continue;
                }

                *p = '\0';
                p++;

                while (*p == ' ') p++;
                char* value_str = p;

                if (name[0] == '\0' || value_str[0] == '\0' || !is_number(value_str)) {
                    terminal_print("Compiler Error: Invalid let syntax\n");
                    idx = 0;
                    continue;
                }

                int var_idx = find_var(vars, var_count, name);
                if (var_idx < 0) {
                    terminal_print("Compiler Error: Unknown variable\n");
                    idx = 0;
                    continue;
                }

                offset += emit_store_imm(code + offset, vars[var_idx].data_offset, str_to_uint(value_str));
            }

            // add
            else if (strncmp(line, "add ", 4) == 0) {
                char* p = line + 4;

                while (*p == ' ') p++;
                char* name = p;

                while (*p != '\0' && *p != ' ') p++;

                if (*p == '\0') {
                    terminal_print("Compiler Error: Invalid add syntax\n");
                    idx = 0;
                    continue;
                }

                *p = '\0';
                p++;

                while (*p == ' ') p++;
                char* arg = p;

                if (!is_number(arg)) {
                    terminal_print("Compiler Error: add currently supports only numbers\n");
                    idx = 0;
                    continue;
                }

                int var_idx = find_var(vars, var_count, name);
                if (var_idx < 0) {
                    terminal_print("Compiler Error: Unknown variable\n");
                    idx = 0;
                    continue;
                }

                offset += emit_add_imm(code + offset, vars[var_idx].data_offset, str_to_uint(arg));
            }

            // sub
            else if (strncmp(line, "sub ", 4) == 0) {
                char* p = line + 4;

                while (*p == ' ') p++;
                char* name = p;

                while (*p != '\0' && *p != ' ') p++;

                if (*p == '\0') {
                    terminal_print("Compiler Error: Invalid sub syntax\n");
                    idx = 0;
                    continue;
                }

                *p = '\0';
                p++;

                while (*p == ' ') p++;
                char* arg = p;

                if (!is_number(arg)) {
                    terminal_print("Compiler Error: sub currently supports only numbers\n");
                    idx = 0;
                    continue;
                }

                int var_idx = find_var(vars, var_count, name);
                if (var_idx < 0) {
                    terminal_print("Compiler Error: Unknown variable\n");
                    idx = 0;
                    continue;
                }

                offset += emit_sub_imm(code + offset, vars[var_idx].data_offset, str_to_uint(arg));
            }
            // print
            else if (strncmp(line, "print ", 6) == 0) {
                char* arg = line + 6;
                while (*arg == ' ') arg++;

                int var_idx = find_var(vars, var_count, arg);

                if (var_idx >= 0) {
                    offset += emit_print_var(code + offset, vars[var_idx].data_offset);
                } else {
                    offset += emit_print(code + offset, arg);
                }
            }

            // sleep
            else if (strncmp(line, "sleep ", 6) == 0) {
                char* arg = line + 6;
                while (*arg == ' ') arg++;

                if (is_number(arg)) {
                    offset += emit_sleep(code + offset, str_to_uint(arg));
                } else {
                    int var_idx = find_var(vars, var_count, arg);
                    if (var_idx < 0) {
                        terminal_print("Compiler Error: Unknown variable\n");
                        idx = 0;
                        continue;
                    }

                    offset += emit_sleep_var(code + offset, vars[var_idx].data_offset);
                }
            }
            // jump
            else if (strncmp(line, "jump ", 5) == 0) {
                char* label = line + 5;
                while (*label == ' ') label++;

                int label_idx = find_label(labels, label_count, label);
                if (label_idx < 0) {
                    terminal_print("Compiler Error: Unknown label\n");
                    idx = 0;
                    continue;
                }

                int32_t rel = (int32_t)labels[label_idx].offset - (int32_t)(offset + 5);
                offset += emit_jump(code + offset, rel);
            }
            
            else if (strncmp(line, "ifzero ", 7) == 0) {
                char* p = line + 7;

                while (*p == ' ') p++;
                char* varname = p;

                while (*p != '\0' && *p != ' ') p++;

                if (*p == '\0') {
                    terminal_print("Compiler Error: Invalid ifzero syntax\n");
                    idx = 0;
                    continue;
                }

                *p = '\0';
                p++;

                while (*p == ' ') p++;
                char* label = p;

                if (varname[0] == '\0' || label[0] == '\0') {
                    terminal_print("Compiler Error: Invalid ifzero syntax\n");
                    idx = 0;
                    continue;
                }

                int var_idx = find_var(vars, var_count, varname);
                if (var_idx < 0) {
                    terminal_print("Compiler Error: Unknown variable\n");
                    idx = 0;
                    continue;
                }

                int label_idx = find_label(labels, label_count, label);
                if (label_idx < 0) {
                    terminal_print("Compiler Error: Unknown label\n");
                    idx = 0;
                    continue;
                }

                offset += emit_cmp_zero(code + offset, vars[var_idx].data_offset);

                int32_t rel = (int32_t)labels[label_idx].offset - (int32_t)(offset + 6);
                offset += emit_je(code + offset, rel);
            }

            // beep
            else if (strcmp(line, "beep") == 0) {
                offset += emit_beep(code + offset);
            }

            idx = 0;
        } else if (c != '\r') {
            if (idx < 255) {
                line[idx++] = c;
            }
        }
    }

    offset += emit_exit(code + offset);

	// Patch prologue so EDI points to start of data section.
	// After call/pop, EDI = base + 5
	// So we need to add (final_code_size - 5)
	uint32_t prologue_addend = offset - 9;
	memcpy(code + 12, &prologue_addend, 4);
	build_kx(out_filename, code, offset, data_section, data_size);

	if (data_section) {
		free(data_section);
	}

	free(code);
}
