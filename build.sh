#!/bin/bash
set -e 

echo "cleaning previous build..."
rm -f *.o isodir/boot/myos.bin KalsangOS.iso

echo "assembling boot and ints..."
as --32 src/boot/boot.s -o boot.o
as --32 src/boot/interrupts.s -o interrupts.o

echo "compiling kernel base..."
gcc -m32 -c src/kernel/kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector
gcc -m32 -c src/kernel/gdt.c -o gdt.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector
gcc -m32 -c src/kernel/idt.c -o idt.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector
gcc -m32 -c src/kernel/isr.c -o isr.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector
gcc -m32 -c src/kernel/pic.c -o pic.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector
gcc -m32 -c src/kernel/shell.c -o shell.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector

echo "linking KalsangOS..."
ld -m elf_i386 --no-warn-rwx-segments -T src/linker.ld -o isodir/boot/myos.bin boot.o interrupts.o kernel.o gdt.o idt.o isr.o pic.o shell.o

echo "forging ISO..."
grub-mkrescue -o KalsangOS.iso isodir

echo "build complete"
