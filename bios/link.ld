/* RISC-V64 BIOS 链接脚本 */

OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
    /* QEMU virt机器的flash起始地址 */
    FLASH (rx) : ORIGIN = 0x20000000, LENGTH = 32M
    RAM (rwx)  : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS
{
    /* 代码段 - 放在Flash中 */
    .text : {
        *(.text)           /* 主要代码 */
        *(.text.*)
        *(.text.init)      /* 初始化代码 */
    } > FLASH
    
    /* 只读数据段 */
    .rodata : {
        *(.rodata)
        *(.rodata.*)
        *(.string)
    } > FLASH
    
    /* 数据段 */
    .data : {
        *(.data)
        *(.data.*)
    } > FLASH
    
    /* BSS段 - 未初始化数据 */
    .bss : {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        _bss_end = .;
    } > RAM
    
    /* 栈段 */
    .stack : {
        . = ALIGN(16);
        *(.stack)
    } > RAM
    
    /* 丢弃不需要的段 */
    /DISCARD/ : {
        *(.comment)
        *(.note)
        *(.eh_frame)
    }
}

/* 定义一些有用的符号 */
_text_start = ADDR(.text);
_text_end = ADDR(.text) + SIZEOF(.text);
_data_start = ADDR(.data);
_data_end = ADDR(.data) + SIZEOF(.data);