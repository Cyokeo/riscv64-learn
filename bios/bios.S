# RISC-V64 简单BIOS固件 - Hello World
# 目标：在QEMU virt机器上输出Hello World

.section .text
.global _start

.global system_reboot

# Memory map for QEMU virt machine
.equ UART_BASE,     0x10000000
.equ PLIC_BASE,     0x0C000000
.equ PLIC_PRIORITY, 0x0C000000
.equ PLIC_PENDING,  0x0C001000
.equ PLIC_ENABLE,   0x0C002000
.equ PLIC_THRESHOLD, 0x0C200000
.equ PLIC_CLAIM,    0x0C200004

_start:
    # Disable interrupts during initialization
    csrci mstatus, 0x8
    # 设置机器模式
    li t0, 0x1800          # MPP=11 (机器模式)
    csrw mstatus, t0
    # Set up stack pointer - use high memory area
    li sp, 0x80200000

    # Clear BSS section
    call clear_bss
    
    # Initialize UART with default baud rate
    li a0, 115200
    call uart_init
    # 输出Hello World字符串
    la a0, welcome_msg
    call print_string
    # Setup interrupt system
    call setup_interrupts

    # Print initial prompt
    la a0, prompt_msg
    call uart_puts
    
    # Enable global interrupts
    csrsi mstatus, 0x8
    
    # Main loop - just wait for interrupts
halt:
    wfi
    j halt

# Clear BSS section
clear_bss:
    la t0, __bss_start
    la t1, __bss_end
clear_loop:
    bge t0, t1, clear_done
    sd zero, 0(t0)
    addi t0, t0, 8
    j clear_loop
clear_done:
    ret

main:
    # 保存返回地址
    addi sp, sp, -16
    sd ra, 8(sp)
    
    # 输出Hello World字符串
    la a0, hello_msg
    call print_string
    
    # 恢复返回地址并返回
    ld ra, 8(sp)
    addi sp, sp, 16
    ret

# Setup interrupt handling system
setup_interrupts:
    # Set up machine trap vector (direct mode)
    la t0, trap_vector
    csrw mtvec, t0
    
    # Configure PLIC for UART interrupt (ID 10)
    li t0, PLIC_PRIORITY + 40  # Priority register for interrupt 10 (10*4)
    li t1, 7                   # Set high priority
    sw t1, 0(t0)
    
    # Enable UART interrupt in PLIC
    li t0, PLIC_ENABLE
    lw t1, 0(t0)
    ori t1, t1, 0x400          # Set bit 10 for UART interrupt
    sw t1, 0(t0)
    
    # Set PLIC threshold to 0 (accept all priorities)
    li t0, PLIC_THRESHOLD
    sw zero, 0(t0)
    
    # Enable machine external interrupts in MIE
    li t0, 0x800               # MIE.MEIE bit
    csrs mie, t0
    
    ret

# Optimized trap vector - minimal overhead
.align 2
trap_vector:
    # Quick context save (only save what we need)
    addi sp, sp, -32
    sd ra, 0(sp)
    sd t0, 8(sp)
    sd t1, 16(sp)
    sd t2, 24(sp)
    
    # Check interrupt cause
    csrr t0, mcause
    li t1, 0x8000000000000000
    and t2, t0, t1
    beqz t2, handle_exception    # Not an interrupt
    
    # Check if it's external interrupt (cause = 0x800000000000000B)
    li t1, 11
    andi t0, t0, 0xFF
    bne t0, t1, trap_return      # Not external interrupt
    
    # Handle external interrupt via PLIC
    call handle_external_interrupt

    j trap_return

handle_exception:
    # For now, just ignore exceptions
    # In a full implementation, handle page faults, illegal instructions, etc.
    j trap_return

handle_external_interrupt:
    # Save additional registers needed for C function call
    addi sp, sp, -72
    sd a0, 0(sp)
    sd a1, 8(sp)
    sd a2, 16(sp)
    sd a3, 24(sp)
    sd a4, 32(sp)
    sd a5, 40(sp)
    sd a6, 48(sp)
    sd a7, 56(sp)
    sd ra, 64(sp)
    
    # Get interrupt ID from PLIC
    li t0, PLIC_CLAIM
    lw a0, 0(t0)
    
    # Check if it's UART interrupt (ID 10)
    li t1, 10
    bne a0, t1, complete_interrupt
    
    # a0 is caller-saved, while s0-s11 is calee-saved
    mv s2, a0
    # Call C UART interrupt handler
    call uart_interrupt_handler
    
complete_interrupt:
    mv a0, s2
    # Complete the interrupt in PLIC
    li t0, PLIC_CLAIM
    sw a0, 0(t0)               # Write back interrupt ID to complete
    
    # Restore registers
    ld a0, 0(sp)
    ld a1, 8(sp)
    ld a2, 16(sp)
    ld a3, 24(sp)
    ld a4, 32(sp)
    ld a5, 40(sp)
    ld a6, 48(sp)
    ld a7, 56(sp)
    ld ra, 64(sp)
    addi sp, sp, 72
    
    ret

trap_return:
    # Restore minimal context
    ld ra, 0(sp)
    ld t0, 8(sp)
    ld t1, 16(sp)
    ld t2, 24(sp)
    addi sp, sp, 32
    
    mret

# System reboot function (called from C)
system_reboot:
    # In QEMU, we can use the test device to exit
    # For a real system, this would trigger a hardware reset
    li t0, 0x100000            # QEMU test device address
    li t1, 0x5555              # Exit code
    sw t1, 0(t0)               # Write to test device
    
    # If that fails, infinite loop
reboot_loop:
    wfi
    j reboot_loop

# 字符串输出函数
# a0: 字符串地址
print_string:
    addi sp, sp, -32
    sd ra, 24(sp)
    sd s0, 16(sp)
    sd s1, 8(sp)
    
    mv s0, a0              # 保存字符串地址
    
print_loop:
    lb s1, 0(s0)           # 加载字符
    beqz s1, print_done    # 如果是'\0'则结束
    
    mv a0, s1              # 将字符放入a0
    call putchar           # 输出字符
    
    addi s0, s0, 1         # 指向下一个字符
    j print_loop
    
print_done:
    ld s1, 8(sp)
    ld s0, 16(sp)
    ld ra, 24(sp)
    addi sp, sp, 32
    ret

# 字符输出函数 (通过UART)
# a0: 要输出的字符
putchar:
    # QEMU virt机器UART0基地址: 0x10000000
    li t0, 0x10000000
    
    # 等待UART准备就绪
uart_wait:
    lb t1, 5(t0)           # 读取线路状态寄存器(LSR)
    andi t1, t1, 0x20      # 检查发送缓冲区空标志(bit 5)
    beqz t1, uart_wait     # 如果不为空则继续等待
    
    # 发送字符
    sb a0, 0(t0)           # 写入发送缓冲区
    ret

# Data section
.section .data
.align 8

welcome_msg:
    .asciz "RISC-V64 BIOS v2.0 (Mixed ASM/C Implementation)\n"

hello_msg:
    .asciz "Hello World!\n"

info_msg:
    .asciz "Features: UART interrupts, command processing, line editing\n"

exit_msg:
    .asciz "Ready to exit interrupt!\n"

prompt_msg:
    .asciz "BIOS> "

# BSS section
.section .bss
.align 8

# Reserve space for C global variables
.global __bss_start
.global __bss_end
__bss_start:
    .space 4096                # variable space
__bss_end:

# 链接脚本信息
.section .note
    .ascii "RISC-V64 Hello World BIOS v1.0"