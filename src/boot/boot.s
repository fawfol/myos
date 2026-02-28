.set ALIGN,    1<<0             /*align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /*provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /*multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /*let bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above to prove we are multiboot */

/*multiboot header */
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/*set up stack for C code */
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

/*entry point */
.section .text
.global _start
_start:
	mov $stack_top, %esp
	call kernel_main
	cli
1:	hlt
	jmp 1b
