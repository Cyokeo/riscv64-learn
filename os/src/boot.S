.section .text.start
.global _start
.global trap_vector

_start:
    # 保存OpenSBI传递的参数
    # a0 = hartid, a1 = fdt_addr
    mv s0, a0      # 保存hartid到s0
    mv s1, a1      # 保存fdt_addr到s1
    
    # 设置栈指针
    la sp, stack_top
    
    # 清零BSS段
    la t0, bss_start
    la t1, bss_end
clear_bss:
    beq t0, t1, clear_bss_done
    sd zero, 0(t0)
    addi t0, t0, 8
    j clear_bss
clear_bss_done:

    # 传递参数给C代码主函数
    mv a0, s0      # hartid
    mv a1, s1      # fdt_addr
    call kernel_main
    
    # 如果主函数返回，进入无限循环
loop:
    wfi
    j loop

# 异常处理向量
.align 4
trap_vector:
    # 保存寄存器上下文（简化版）
    addi sp, sp, -32*8
    sd ra, 0*8(sp)
    sd t0, 1*8(sp)
    sd t1, 2*8(sp)
    sd t2, 3*8(sp)
    sd t3, 4*8(sp)
    sd t4, 5*8(sp)
    sd t5, 6*8(sp)
    sd t6, 7*8(sp)
    sd a0, 8*8(sp)
    sd a1, 9*8(sp)
    sd a2, 10*8(sp)
    sd a3, 11*8(sp)
    sd a4, 12*8(sp)
    sd a5, 13*8(sp)
    sd a6, 14*8(sp)
    sd a7, 15*8(sp)
    sd s0, 16*8(sp)
    sd s1, 17*8(sp)
    sd s2, 18*8(sp)
    sd s3, 19*8(sp)
    sd s4, 20*8(sp)
    sd s5, 21*8(sp)
    sd s6, 22*8(sp)
    sd s7, 23*8(sp)
    sd s8, 24*8(sp)
    sd s9, 25*8(sp)
    sd s10, 26*8(sp)
    sd s11, 27*8(sp)
    sd gp, 28*8(sp)
    sd tp, 29*8(sp)
    
    # 调用C语言异常处理函数
    call trap_handler
    
    # 恢复寄存器上下文
    ld ra, 0*8(sp)
    ld t0, 1*8(sp)
    ld t1, 2*8(sp)
    ld t2, 3*8(sp)
    ld t3, 4*8(sp)
    ld t4, 5*8(sp)
    ld t5, 6*8(sp)
    ld t6, 7*8(sp)
    ld a0, 8*8(sp)
    ld a1, 9*8(sp)
    ld a2, 10*8(sp)
    ld a3, 11*8(sp)
    ld a4, 12*8(sp)
    ld a5, 13*8(sp)
    ld a6, 14*8(sp)
    ld a7, 15*8(sp)
    ld s0, 16*8(sp)
    ld s1, 17*8(sp)
    ld s2, 18*8(sp)
    ld s3, 19*8(sp)
    ld s4, 20*8(sp)
    ld s5, 21*8(sp)
    ld s6, 22*8(sp)
    ld s7, 23*8(sp)
    ld s8, 24*8(sp)
    ld s9, 25*8(sp)
    ld s10, 26*8(sp)
    ld s11, 27*8(sp)
    ld gp, 28*8(sp)
    ld tp, 29*8(sp)
    addi sp, sp, 32*8
    
    # 返回
    sret

.section .bss
.align 16
stack_bottom:
    .space 8192  # 8KB栈空间（增加了栈大小）
stack_top: