#!/bin/bash
set -e

echo "cleaning previous build..."
rm -rf obj bin isodir KalsangOS.iso initrd.tar
mkdir -p obj bin isodir/boot/grub src/user

# === BUILD USER LAND ===
echo "building use land programs..."

#compile Hello World
gcc -m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -c src/user/hello.c -o obj/hello.o
ld -m elf_i386 -e _start -Ttext 0x40000000 obj/hello.o -o bin/hello.bin --oformat binary

#compile loader test
gcc -m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -Isrc/user -c src/user/loader.c -o obj/loader.o
ld -m elf_i386 -e _start -Ttext 0x40000000 obj/loader.o -o bin/loader.bin --oformat binary

# === CREATE RAMDISK ===
echo "creating ramdisk..."
echo "KALSANG_OS_DISK_V1: Self-Hosting test file." > test.txt

#pack EVERYTHING into ONE tar file
tar -cvf initrd.tar test.txt bin/hello.bin bin/loader.bin src/user/loader.c --format=ustar --owner=root --group=root
cp initrd.tar isodir/boot/

# === BUILD KERNEL ===
echo "assembling boot and ints..."
as --32 src/boot/boot.s -o obj/boot.o
as --32 src/boot/interrupts.s -o obj/interrupts.o

echo "compiling kernel base..."
for file in kernel gdt idt isr pic shell timer paging memory ramdisk mouse ata fat32; do
    gcc -m32 -c src/kernel/$file.c -o obj/$file.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector
done

echo "linking KalsangOS..."
ld -m elf_i386 --no-warn-rwx-segments -T src/linker.ld -o isodir/boot/myos.bin \
    obj/boot.o obj/interrupts.o obj/kernel.o obj/gdt.o obj/idt.o obj/isr.o obj/mouse.o\
    obj/pic.o obj/shell.o obj/timer.o obj/paging.o obj/memory.o obj/ramdisk.o obj/ata.o obj/fat32.o

# === FORGE ISO ===
echo "forging ISO..."
cat << EOF > isodir/boot/grub/grub.cfg
menuentry "KalsangOS" {
    multiboot /boot/myos.bin
    module /boot/initrd.tar
    boot
}
EOF

grub-mkrescue -o KalsangOS.iso isodir
echo "------------------------------------------------"
echo "Build complete! Boot KalsangOS.iso"
echo "WELCOME TO KalsangOS"
echo "------------------------------------------------"

# === CREATE QEMU DISK IF NOT EXISTS ===
if [ ! -f disk.img ]; then
    echo "creating virtual disk..."
    qemu-img create disk.img 64M
fi

# === RUN QEMU ===
echo "starting QEMU..."
qemu-system-i386 -cdrom KalsangOS.iso -drive file=disk.img,format=raw,index=0,media=disk -boot d
                                                                                                                                                                                                                                           
┌──(kalsang㉿kalikalsang)-[~/KalsangOS]
└─$ ./build.sh
cleaning previous build...
building use land programs...
creating ramdisk...
test.txt
bin/hello.bin
bin/loader.bin
src/user/loader.c
assembling boot and ints...
compiling kernel base...
linking KalsangOS...
ld: cannot find obj.fat32.o: No such file or directory

