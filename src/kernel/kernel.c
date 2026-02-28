/*check if the compiler is targeting the wrong operating system*/
#if defined(__linux__)
#error "You are not using a cross-compiler, you will run into trouble"
#endif

void kernel_main(void) 
{
	terminal_buffer = (uint16_t*) 0xB8000;

	const char* str = "Welcome to TenzinOs!";
	
	for (int i = 0; str[i] != '\0'; i++) {
		terminal_buffer[i] = (uint16_t) str[i] | (uint16_t) 0x07 << 8;
	}

	while (1);
}
