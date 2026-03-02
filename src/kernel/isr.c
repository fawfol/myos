#include <stdint.h>

typedef struct {
    uint32_t ds;                                     //ds selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; //pushed by pusha
    uint32_t int_no, err_code;                       //int num and error code our push;
    uint32_t eip, cs, eflags, useresp, ss;           //pushed by processor automatically
} registers_t;

void isr_handler(registers_t regs) {
    
    //check if its int 33
    if (regs.int_no == 33) {
        //read keyboard port
        
        //
        
        uint16_t* terminal_buffer = (uint16_t*) 0xB8000;
        terminal_buffer[0] = (uint16_t) 'K' | (uint16_t) 0x0C << 8;
    }
}
