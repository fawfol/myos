void kernel_main() {
short video_memory = (short)0xB8000;
const char* str = "Welcome to My OS - Seminar Project";
for (int i = 0; str[i] != '\0'; i++) {
video_memory[i] = (video_memory[i] & 0xFF00) | str[i];
}
}
