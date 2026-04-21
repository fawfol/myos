ENTRY(_start)
SECTIONS
{
    /* Load everything at the 32MB mark */
    . = 0x02000000;

    /* 1. The Entry Function goes absolutely FIRST */
    .text.entry : {
        *(.text.entry)
    }

    /* 2. Rest of the Code */
    .text : ALIGN(4) {
        *(.text)
    }

    /* 3. Read-only data (strings) */
    .rodata : ALIGN(4) {
        *(.rodata)
        *(.rodata.*)
    }

    /* 4. Initialized variables */
    .data : ALIGN(4) {
        *(.data)
    }

    /* 5. Uninitialized variables */
    .bss : ALIGN(4) {
        *(COMMON)
        *(.bss)
    }
}
