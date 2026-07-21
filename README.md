<div align="center">

# KalsangOS
**A 32-bit x86 Monolithic Operating System Built from Scratch**

</div>

> "Sovereignty of Code" — Achieving a zero-dependency environment without relying on external ports, standard C libraries, or third-party disk drivers. 

## 📌 Project Overview

KalsangOS is a custom monolithic 32-bit x86 operating system developed from the ground up as a hands-on exploration of low-level systems engineering. Built around the philosophy of achieving a "Self-Hosting" milestone, the system is designed to compile its own code, manage its own memory, and execute its own native binaries without requiring an external host OS.

Instead of layering heavy abstractions, KalsangOS tackles the machine head-on at the bare-metal level—managing CPU registers, setting up flat memory models, responding to hardware interrupts, and parsing the FAT32 file system directly.

---

## ✨ Core Features

*   **Custom Native Toolchain:** Features the `kx_compiler`, a built-in compiler that parses custom syntax and generates raw x86 machine code entirely within the OS.
*   **The KX Executable Format:** Utilizes a proprietary, streamlined "flat binary" format (`KX`) for fast memory concatenation and loading, dropping the overhead of complex formats like ELF.
*   **Hardware Privilege Isolation:** Enforces strict hardware separation between Supervisor Mode (Ring 0) and User Mode (Ring 3) using the Global Descriptor Table (GDT) and hardware trap system calls.
*   **Virtual File System (VFS):** Implements a VFS layer that bridges an initial RAMDisk (TAR initrd) for rapid, zero-copy memory access with a custom FAT32 driver for persistent hard drive storage.
*   **Custom Memory Management:** Features a two-level virtual paging structure and a dynamic kernel heap allocator (`kmalloc`) utilizing a First-Fit linked-list architecture with automatic block coalescence.
*   **Bare-Metal Device Drivers:** Includes custom-built Programmable Interval Timer (PIT) at 100Hz, PS/2 Keyboard and Mouse drivers, and an ATA Programmed I/O (PIO) mode driver for direct disk communication.

---

## 🏗️ System Architecture

*   **Bootloader:** Multiboot-compliant entry point designed to be loaded by GRUB.
*   **Kernel Design:** Monolithic architecture where memory management, task scheduling, VFS, and hardware drivers reside securely in Ring 0.
*   **System Call Interface:** Uses the universally recognized `int 0x80` interrupt vector to safely transition untrusted user-land applications to privileged kernel services.
*   **Standard Library:** Includes `kalsang_libc.h`, a lightweight, dependency-free wrapper for system calls (e.g., `malloc` over the `SYS_SBRK` syscall).

---

## 🛠️ Building and Emulation

KalsangOS utilizes a deterministic, automated build pipeline designed for Linux environments.

### Prerequisites
Ensure the following cross-compilation tools are installed on your host machine:
*   `nasm` (Netwide Assembler)
*   `i686-elf-gcc` (Cross-compiler for pure x86 freestanding code)
*   `xorriso` (For ISO packaging and GRUB configuration)
*   `qemu-system-i386` (For emulation)

### Build Instructions
1. Clone the repository[cite: 1]:
   ```bash
   git clone [https://github.com/fawfol/myos](https://github.com/fawfol/myos)
   cd myos
