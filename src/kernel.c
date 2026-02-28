void kernel_main() {
    short* video_memory = (short*)0xB8000;
    const char* str = "Welcome to My OS - Seminar Project";

    for (int i = 0; i < 80 * 25; i++) {
        video_memory[i] = (short)0x0F20; 
    }
    
    for (int i = 0; str[i] != '\0'; i++) {
        video_memory[i] = (short)str[i] | 0x0F00;
    }
}
