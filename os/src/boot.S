.section .text.start
.global _start

_start:
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

    # 跳转到C代码主函数
    call kernel_main
    
    # 如果主函数返回，进入无限循环
loop:
    wfi
    j loop

.section .bss
.align 16
stack_bottom:
    .space 4096  # 4KB栈空间
stack_top: