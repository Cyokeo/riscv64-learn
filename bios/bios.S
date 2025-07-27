# RISC-V64 简单BIOS固件 - Hello World
# 目标：在QEMU virt机器上输出Hello World

.section .text
.global _start

_start:
    # 初始化栈指针
    la sp, stack_top
    
    # 设置机器模式
    li t0, 0x1800          # MPP=11 (机器模式)
    csrw mstatus, t0
    
    # 调用主程序
    call main
    
    # 无限循环
halt:
    wfi                    # 等待中断
    j halt

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

# 异常处理程序
exception_handler:
    # 简单的异常处理：直接跳转到halt
    j halt

.section .data
hello_msg:
    .string "Hello World from RISC-V BIOS!\n"

.section .bss
    .space 4096            # 4KB栈空间
stack_top:

# 设置异常向量表
.section .text.init
.align 2
.global _trap_vector
_trap_vector:
    j exception_handler    # 所有异常都跳转到异常处理程序

# 链接脚本信息
.section .note
    .ascii "RISC-V64 Hello World BIOS v1.0"