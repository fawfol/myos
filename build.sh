#!/bin/bash

#assemble
i686-linux-gnu-as src/boot/boot.s -o boot.o

# c ompile
i686-linux-gnu-gcc -c src/kernel/kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra

#linker
i686-linux-gnu-gcc -T src/linker.ld -o isodir/boot/myos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc

# iso
grub-mkrescue -o TenzinOs.iso isodir
