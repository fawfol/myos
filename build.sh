#!/bin/bash
set -e

echo "cleaning previous build..."
rm -rf obj bin isodir KalsangOS.iso initrd.tar
mkdir -p obj bin isodir/boot/grub src/user

# === BUILD USER LAND ===
echo "building user land programs..."

mkdir -p tools

# assemble hello
as --32 src/user/hello.s -o obj/hello_user.o
ld -m elf_i386 -N -e _start -Ttext 0x00000000 \
    obj/hello_user.o -o bin/hello.bin --oformat binary
python3 tools/make_kx.py bin/hello.bin bin/hello.kx

# compile loader.c
gcc -m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector \
    -Isrc/user -c src/user/loader.c -o obj/loader.o
ld -m elf_i386 -N -e _start -Ttext 0x00000000 \
    obj/loader.o -o bin/loader.bin --oformat binary
python3 tools/make_kx.py bin/loader.bin bin/loader.kx

# assemble writer
as --32 src/user/writer.s -o obj/writer_user.o
ld -m elf_i386 -N -e _start -Ttext 0x00000000 \
    obj/writer_user.o -o bin/writer.bin --oformat binary
python3 tools/make_kx.py bin/writer.bin bin/writer.kx

# assemble generator
as --32 src/user/generator.s -o obj/generator_user.o
ld -m elf_i386 -N -e _start -Ttext 0x00000000 \
    obj/generator_user.o -o bin/generator.bin --oformat binary
python3 tools/make_kx.py bin/generator.bin bin/generator.kx


# === CREATE RAMDISK ===
echo "creating ramdisk..."
echo "KALSANG_OS_DISK_V1: Self-Hosting test file." > test.txt

tar -cvf initrd.tar \
    test.txt \
    bin/hello.kx \
    bin/loader.kx \
    bin/writer.kx \
    bin/generator.kx \
    --format=ustar --owner=root --group=root

cp initrd.tar isodir/boot/

# === BUILD KERNEL ===
echo "assembling boot and ints..."
as --32 src/boot/boot.s -o obj/boot.o
as --32 src/boot/interrupts.s -o obj/interrupts.o

echo "compiling kernel base..."
for file in kernel gdt idt isr pic shell timer paging memory ramdisk mouse ata fat32 kx_loader; do
    gcc -m32 -c src/kernel/$file.c -o obj/$file.o \
        -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector
done

echo "linking KalsangOS..."
ld -m elf_i386 --no-warn-rwx-segments -T src/linker.ld -o isodir/boot/myos.bin \
    obj/boot.o \
    obj/interrupts.o \
    obj/kernel.o \
    obj/gdt.o \
    obj/idt.o \
    obj/isr.o \
    obj/mouse.o \
    obj/pic.o \
    obj/shell.o \
    obj/timer.o \
    obj/paging.o \
    obj/memory.o \
    obj/ramdisk.o \
    obj/ata.o \
    obj/fat32.o \
    obj/kx_loader.o

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
qemu-system-i386 \
    -cdrom KalsangOS.iso \
    -drive file=disk.img,format=raw,index=0,media=disk \
    -boot d
