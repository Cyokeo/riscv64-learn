#include <stdint.h>
#include <stddef.h>

// OpenSBI系统调用接口定义
#define SBI_SET_TIMER 0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
#define SBI_CLEAR_IPI 3
#define SBI_SEND_IPI 4
#define SBI_REMOTE_FENCE_I 5
#define SBI_REMOTE_SFENCE_VMA 6
#define SBI_REMOTE_SFENCE_VMA_ASID 7
#define SBI_SHUTDOWN 8

// SBI调用结构体
struct sbiret {
    long error;
    long value;
};

// 内联汇编实现SBI调用
static inline struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
                                      unsigned long arg1, unsigned long arg2,
                                      unsigned long arg3, unsigned long arg4,
                                      unsigned long arg5) {
    struct sbiret ret;
    register unsigned long a0 asm("a0") = arg0;
    register unsigned long a1 asm("a1") = arg1;
    register unsigned long a2 asm("a2") = arg2;
    register unsigned long a3 asm("a3") = arg3;
    register unsigned long a4 asm("a4") = arg4;
    register unsigned long a5 asm("a5") = arg5;
    register unsigned long a6 asm("a6") = fid;
    register unsigned long a7 asm("a7") = ext;
    
    asm volatile("ecall"
                 : "+r"(a0), "+r"(a1)
                 : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                 : "memory");
    
    ret.error = a0;
    ret.value = a1;
    return ret;
}

// 输出单个字符
void sbi_console_putchar(int ch) {
    sbi_ecall(SBI_CONSOLE_PUTCHAR, 0, ch, 0, 0, 0, 0, 0);
}

// 输出字符串
void puts(const char *s) {
    while (*s) {
        sbi_console_putchar(*s++);
    }
}

// 关机
void sbi_shutdown(void) {
    sbi_ecall(SBI_SHUTDOWN, 0, 0, 0, 0, 0, 0, 0);
    while (1) {}  // 永远不应该到达这里
}

// 内核主函数
void kernel_main(void) {
    puts("Hello World from RISC-V 64 Kernel!\n");
    puts("This is a simple kernel running on QEMU with OpenSBI\n");
    puts("Shutting down...\n");
    
    sbi_shutdown();
}