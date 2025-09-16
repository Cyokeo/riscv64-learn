ENTRY(_start)

MEMORY
{
    RAM : ORIGIN = 0x800c0000, LENGTH = 128M
}

SECTIONS
{
    . = 0x80200000;  /* 内核加载地址 */
    
    .text : ALIGN(4) {
        KEEP(*(.text.start))
        *(.text*)
    } > RAM
    
    .rodata : ALIGN(4) {
        *(.rodata*)
        *(.srodata*)
    } > RAM
    
    .data : ALIGN(4) {
        *(.data*)
        *(.sdata*)
    } > RAM
    
    .bss : ALIGN(8) {
        bss_start = .;
        *(.bss*)
        *(.sbss*)
        *(COMMON)
        . = ALIGN(8);
        bss_end = .;
    } > RAM
    
    _end = .;
    
    /DISCARD/ : {
        *(.comment)
        *(.gnu*)
        *(.note*)
        *(.eh_frame*)
        *(.riscv.attributes)
    }
}