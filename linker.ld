/*
* @file linker.ld
*/

OUTPUT_FORMAT(elf32-i386) /* executable & linkable format*/
ENTRY(_start)
SECTIONS
{
    . = 0x100000; /* location counter = kernel's code start*/
    .text : { *(.text) } /* all .text input sections from all input files */
    .data : { *(.data) }
    .bss  : { *(.bss)  }
    /* location counter = 0x100000 + size of text, data, bss */
}