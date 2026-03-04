#!/bin/bash
set -e

echo "cleaning previous build..."
rm -rf obj isodir KalsangOS.iso
mkdir -p obj isodir/boot/grub

echo "creating ramdisk..."
echo "KALSANG_OS_DISK_V1" > test.txt
# --format=ustar: Use the legacy simple header format
# --owner=root --group=root: Standardize metadata
tar -cvf initrd.tar test.txt --format=ustar --owner=root --group=root
cp initrd.tar isodir/boot/


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
gcc -m32 -c src/kernel/timer.c -o timer.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector
gcc -m32 -c src/kernel/paging.c -o paging.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector
gcc -m32 -c src/kernel/memory.c -o memory.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector
gcc -m32 -c src/kernel/ramdisk.c -o ramdisk.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector

echo "linking KalsangOS..."
ld -m elf_i386 --no-warn-rwx-segments -T src/linker.ld -o isodir/boot/myos.bin boot.o interrupts.o kernel.o gdt.o idt.o isr.o pic.o shell.o timer.o paging.o memory.o ramdisk.o

echo "forging ISO..."
cat << EOF > isodir/boot/grub/grub.cfg
menuentry "KalsangOS" {
    multiboot /boot/myos.bin
    module /boot/initrd.tar
    boot
}
EOF

grub-mkrescue -o KalsangOS.iso isodir
echo "build complete"
