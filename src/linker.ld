ENTRY(_start)
SECTIONS
{
    . = 2M; 

    .text : { *(.text) }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
}
