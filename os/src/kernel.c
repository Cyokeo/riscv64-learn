#include <stdint.h>
#include <stddef.h>

// RISC-V CSR寄存器定义
#define CSR_SSTATUS     0x100
#define CSR_SIE         0x104
#define CSR_STVEC       0x105
#define CSR_SSCRATCH    0x140
#define CSR_SEPC        0x141
#define CSR_SCAUSE      0x142
#define CSR_STVAL       0x143
#define CSR_SIP         0x144
#define CSR_SATP        0x180

// SSTATUS寄存器位定义
#define SSTATUS_SIE     (1UL << 1)
#define SSTATUS_SPIE    (1UL << 5)
#define SSTATUS_SPP     (1UL << 8)
#define SSTATUS_SUM     (1UL << 18)

// 中断相关定义
#define IRQ_S_SOFT      1
#define IRQ_S_TIMER     5
#define IRQ_S_EXT       9

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

// SBI v0.2+ 扩展ID
#define SBI_EXT_BASE            0x10
#define SBI_EXT_TIME            0x54494D45
#define SBI_EXT_IPI             0x735049
#define SBI_EXT_RFENCE          0x52464E43
#define SBI_EXT_HSM             0x48534D
#define SBI_EXT_SRST            0x53525354

// 设备树相关定义
#define FDT_MAGIC       0xd00dfeed
#define FDT_BEGIN_NODE  0x1
#define FDT_END_NODE    0x2
#define FDT_PROP        0x3
#define FDT_NOP         0x4
#define FDT_END         0x9

// 页表相关定义（Sv39）
#define SATP_MODE_SV39  (8UL << 60)
#define PAGE_SHIFT      12
#define PAGE_SIZE       (1UL << PAGE_SHIFT)
#define PTE_V           (1UL << 0)   // 有效位
#define PTE_R           (1UL << 1)   // 读权限
#define PTE_W           (1UL << 2)   // 写权限
#define PTE_X           (1UL << 3)   // 执行权限
#define PTE_U           (1UL << 4)   // 用户模式访问
#define PTE_G           (1UL << 5)   // 全局映射
#define PTE_A           (1UL << 6)   // 访问位
#define PTE_D           (1UL << 7)   // 脏位

// 内存布局定义
#define KERNEL_BASE     0x80200000UL
#define KERNEL_VBASE    0xffffffffc0200000UL  // 虚拟地址基址
#define UART_BASE       0x10000000UL
#define UART_VBASE      0xffffffffc0000000UL  // UART虚拟地址

// 全局变量
static uint64_t boot_hartid;
static uint64_t boot_fdt_addr;

// SBI调用结构体
struct sbiret {
    long error;
    long value;
};

// 设备树头部结构
struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};

// CSR操作宏
#define csr_read(csr) ({ \
    unsigned long __v; \
    asm volatile("csrr %0, " #csr : "=r"(__v) : : "memory"); \
    __v; \
})

#define csr_write(csr, val) ({ \
    asm volatile("csrw " #csr ", %0" : : "r"(val) : "memory"); \
})

#define csr_set(csr, val) ({ \
    unsigned long __v; \
    asm volatile("csrrs %0, " #csr ", %1" : "=r"(__v) : "r"(val) : "memory"); \
    __v; \
})

#define csr_clear(csr, val) ({ \
    unsigned long __v; \
    asm volatile("csrrc %0, " #csr ", %1" : "=r"(__v) : "r"(val) : "memory"); \
    __v; \
})

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

// SBI服务接口
void sbi_console_putchar(int ch) {
    sbi_ecall(SBI_CONSOLE_PUTCHAR, 0, ch, 0, 0, 0, 0, 0);
}

struct sbiret sbi_get_spec_version(void) {
    return sbi_ecall(SBI_EXT_BASE, 0, 0, 0, 0, 0, 0, 0);
}

struct sbiret sbi_get_impl_id(void) {
    return sbi_ecall(SBI_EXT_BASE, 1, 0, 0, 0, 0, 0, 0);
}

struct sbiret sbi_probe_extension(long extension_id) {
    return sbi_ecall(SBI_EXT_BASE, 3, extension_id, 0, 0, 0, 0, 0);
}

void sbi_set_timer(uint64_t stime_value) {
    sbi_ecall(SBI_EXT_TIME, 0, stime_value, 0, 0, 0, 0, 0);
}

void sbi_shutdown(void) {
    sbi_ecall(SBI_SHUTDOWN, 0, 0, 0, 0, 0, 0, 0);
    while (1) {}
}

// 字符串和输出函数
static inline int strlen(const char *s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

void puts(const char *s) {
    while (*s) {
        sbi_console_putchar(*s++);
    }
}

void print_hex(uint64_t value) {
    const char hex_chars[] = "0123456789abcdef";
    char buffer[19] = "0x";
    int i;
    
    for (i = 15; i >= 0; i--) {
        buffer[2 + (15 - i)] = hex_chars[(value >> (i * 4)) & 0xf];
    }
    buffer[18] = '\0';
    puts(buffer);
}

void print_dec(uint64_t value) {
    char buffer[21];
    int i = 0;
    
    if (value == 0) {
        sbi_console_putchar('0');
        return;
    }
    
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    while (--i >= 0) {
        sbi_console_putchar(buffer[i]);
    }
}

// 字节序转换
static inline uint32_t be32_to_cpu(uint32_t val) {
    return (val << 24) | ((val & 0xffffff00) << 16) | ((val & 0xffff0000) << 8) | (val & 0xff000000);
}

// 1. 验证传入参数
int validate_boot_params(uint64_t hartid, uint64_t fdt_addr) {
    puts("=== 验证启动参数 ===\n");
    
    // 验证HART ID（通常应该是0对于单核系统）
    puts("HART ID: ");
    print_dec(hartid);
    puts("\n");
    
    if (hartid >= 16) {  // 合理的HART数量限制
        puts("错误: HART ID过大\n");
        return -1;
    }
    
    // 验证FDT地址
    puts("FDT地址: ");
    print_hex(fdt_addr);
    puts("\n");
    
    if (fdt_addr == 0) {
        puts("错误: FDT地址为空\n");
        return -1;
    }
    
    // 检查地址对齐
    if (fdt_addr & 0x3) {
        puts("错误: FDT地址未对齐\n");
        return -1;
    }
    
    // 验证FDT魔数
    struct fdt_header *fdt = (struct fdt_header *)fdt_addr;
    uint32_t magic = be32_to_cpu(fdt->magic);
    
    puts("FDT魔数: ");
    print_hex(magic);
    puts("\n");
    
    if (magic != FDT_MAGIC) {
        puts("错误: FDT魔数不匹配\n");
        return -1;
    }
    
    puts("✓ 启动参数验证通过\n\n");
    return 0;
}

// 2. 解析设备树
int parse_device_tree(uint64_t fdt_addr) {
    puts("=== 解析设备树 ===\n");
    
    struct fdt_header *fdt = (struct fdt_header *)fdt_addr;
    uint32_t totalsize = be32_to_cpu(fdt->totalsize);
    uint32_t version = be32_to_cpu(fdt->version);
    
    puts("FDT总大小: ");
    print_dec(totalsize);
    puts(" 字节\n");
    
    puts("FDT版本: ");
    print_dec(version);
    puts("\n");
    
    // 检查版本号合理性
    if (version < 16 || version > 20) {
        puts("警告: FDT版本可能不受支持\n");
    }
    
    // 检查大小合理性
    if (totalsize < sizeof(struct fdt_header) || totalsize > 0x100000) {
        puts("错误: FDT大小不合理\n");
        return -1;
    }
    
    // 获取各个段的偏移
    uint32_t off_dt_struct = be32_to_cpu(fdt->off_dt_struct);
    uint32_t off_dt_strings = be32_to_cpu(fdt->off_dt_strings);
    
    puts("设备树结构偏移: ");
    print_hex(off_dt_struct);
    puts("\n");
    
    puts("字符串表偏移: ");
    print_hex(off_dt_strings);
    puts("\n");
    
    // 简单验证偏移的合理性
    if (off_dt_struct >= totalsize || off_dt_strings >= totalsize) {
        puts("错误: FDT偏移超出范围\n");
        return -1;
    }
    
    puts("✓ 设备树解析完成\n\n");
    return 0;
}

// Higer 4G in rv39
static uint64_t page_table[512] __attribute__((aligned(4096)));
static uint64_t page_table_h2[4][512] __attribute__((aligned(4096)));
static uint64_t page_table_h3[4][512][512] __attribute__((aligned(4096)));

// Lower 4G in rv39
static uint64_t page_table_l1[4][512] __attribute__((aligned(4096)));
static uint64_t page_table_l0[4][512][512] __attribute__((aligned(4096)));

uint64_t va_2_pa_test(uint64_t va)
{

}

// 3. 初始化MMU（简化的Sv39实现）
// can refer to kvmmake in xv64
void init_mmu(void) {
    puts("=== 初始化MMU ===\n");
    
    // lower 4G directly mapping
#if 0
    uint64_t va = 0;
    for (short i = 0; i < 4; ++i)
    {
        uint64_t l2_paddr = (uint64_t)page_table_l1[i];
        page_table[i] = (l2_paddr >> 12) << 10 | PTE_V;
        for (int j = 0; j < 512; ++j)
        {
            uint64_t l1_paddr = (uint64_t)&page_table_l0[i][j];
            page_table_l1[i][j] = (l1_paddr >> 12) << 10 | PTE_V;
            for (int k = 0; k < 512; ++k)
            {
                // uint64_t l0_paddr = (i << 30) | (j << 21) | (k << 12); wrong!!! integer overflow!
                uint64_t l0_paddr = ((uint64_t)i << 30) | ((uint64_t)j << 21) | ((uint64_t)k << 12); // ✅
                l0_paddr = ((l0_paddr >> 12) << 10) | PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
                // uint64_t l0_paddr = (uint64_t)((va >> 12) << 10) | PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
                page_table_l0[i][j][k] = l0_paddr;
                va += 4096;
            }
        }
    }

#else

    for (uint64_t va = 0; va < ((uint64_t)4 << 30); va += 4096)
    {
        uint64_t *pte2 = &page_table[(va >> 30) & 0x1ff];
        uint64_t l1_table = (uint64_t)page_table_l1[(va >> 30) & 0x1ff];
        *pte2 = ((l1_table >> 12) << 10) | PTE_V;
        uint64_t *pte1 = &((uint64_t*)l1_table)[(va >> 21) & 0x1ff];
        uint64_t l0_table = (uint64_t)page_table_l0[(va >> 30) & 0x1ff][(va >> 21) & 0x1ff];
        *pte1 = ((l0_table >> 12) << 10) | PTE_V;
        uint64_t *pte0 = &((uint64_t*)l0_table)[(va >> 12) & 0x1ff];
        *pte0 = (((va >> 12)) << 10) | PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    }
#endif
    
    // 映射UART等设备（可选）
    // page_table_l1[0] = (0x10000000UL >> 12) << 10 | PTE_V | PTE_R | PTE_W;
    
    // 设置SATP寄存器启用分页
    uint64_t satp = SATP_MODE_SV39 | ((uint64_t)page_table >> 12);
    
    puts("页表L2地址: ");
    print_hex((uint64_t)page_table);
    puts("\n");
    
    puts("SATP值: ");
    print_hex(satp);
    puts("\n");
    
    // 3. 原子地启用分页
    asm volatile("sfence.vma zero, zero");
    asm volatile("csrw satp, %0" : : "r" (satp));
    asm volatile("sfence.vma zero, zero");
    
    puts("✅ MMU初始化完成\n\n");
    puts("hello, cyokeo has inited the mmu!!!\n");
}

// 异常处理函数
void trap_handler(void) {
    uint64_t scause = csr_read(scause);
    uint64_t sepc = csr_read(sepc);
    uint64_t stval = csr_read(stval);
    
    puts("!!! 异常发生 !!!\n");
    puts("异常原因 (scause): ");
    print_hex(scause);
    puts("\n");
    puts("异常PC (sepc): ");
    print_hex(sepc);
    puts("\n");
    puts("异常值 (stval): ");
    print_hex(stval);
    puts("\n");
    
    // 简单处理：直接关机
    puts("系统关机...\n");
    sbi_shutdown();
}

// 4. 设置异常处理
void setup_trap_handling(void) {
    puts("=== 设置异常处理 ===\n");
    
    // 设置异常向量基址
    extern void trap_vector(void);
    csr_write(stvec, (uint64_t)trap_vector);
    
    puts("异常向量地址: ");
    print_hex((uint64_t)trap_vector);
    puts("\n");
    
    // 启用中断
    csr_set(sstatus, SSTATUS_SIE);
    csr_set(sie, (1UL << IRQ_S_TIMER) | (1UL << IRQ_S_EXT) | (1UL << IRQ_S_SOFT));
    
    puts("SSTATUS: ");
    print_hex(csr_read(sstatus));
    puts("\n");
    
    puts("SIE: ");
    print_hex(csr_read(sie));
    puts("\n");
    
    puts("✓ 异常处理设置完成\n\n");
}

// 5. 测试SBI服务
void test_sbi_services(void) {
    puts("=== 测试SBI服务 ===\n");
    
    // 获取SBI规范版本
    struct sbiret ret = sbi_get_spec_version();
    puts("SBI规范版本: ");
    if (ret.error == 0) {
        print_hex(ret.value);
        puts(" (major: ");
        print_dec((ret.value >> 24) & 0xff);
        puts(", minor: ");
        print_dec(ret.value & 0xffffff);
        puts(")");
    } else {
        puts("获取失败");
    }
    puts("\n");
    
    // 获取SBI实现ID
    ret = sbi_get_impl_id();
    puts("SBI实现ID: ");
    if (ret.error == 0) {
        print_hex(ret.value);
        if (ret.value == 1) {
            puts(" (OpenSBI)");
        }
    } else {
        puts("获取失败");
    }
    puts("\n");
    
    // 测试扩展探测
    const char *extensions[] = {"BASE", "TIME", "IPI", "RFENCE", "HSM", "SRST"};
    long ext_ids[] = {SBI_EXT_BASE, SBI_EXT_TIME, SBI_EXT_IPI, 
                      SBI_EXT_RFENCE, SBI_EXT_HSM, SBI_EXT_SRST};
    
    for (int i = 0; i < 6; i++) {
        ret = sbi_probe_extension(ext_ids[i]);
        puts("扩展 ");
        puts(extensions[i]);
        puts(": ");
        if (ret.error == 0 && ret.value == 1) {
            puts("支持");
        } else {
            puts("不支持");
        }
        puts("\n");
    }
    
    puts("✓ SBI服务测试完成\n\n");
}

// 内核主函数
void kernel_main(uint64_t hartid, uint64_t fdt_addr) {
    // 保存启动参数
    boot_hartid = hartid;
    boot_fdt_addr = fdt_addr;
    
    puts("\n");
    puts("========================================\n");
    puts("    RISC-V 64位内核启动 - 增强版\n");
    puts("========================================\n");
    puts("版本: 1.1.0\n");
    puts("架构: RISC-V 64位 (Supervisor Mode)\n");
    puts("构建: " __DATE__ " " __TIME__ "\n\n");
    

    // 1. 验证传入参数
    if (validate_boot_params(hartid, fdt_addr) != 0) {
        puts("❌ 启动参数验证失败，系统关机\n");
        // sbi_shutdown();
    }
#if 0
    // 2. 解析设备树
    if (parse_device_tree(fdt_addr) != 0) {
        puts("设备树解析失败，系统关机\n");
        sbi_shutdown();
    }
#endif
    // 3. 初始化MMU
    init_mmu();
    
    // 4. 设置异常处理
    setup_trap_handling();
    
    // 5. 测试SBI服务
    test_sbi_services();
    
    puts("========================================\n");
    puts("       内核初始化完成！\n");
    puts("========================================\n");
    puts("\n");
    puts("Hello World from Enhanced RISC-V Kernel!\n");
    puts("所有子系统已初始化完成\n");
    puts("内核运行正常，准备关机...\n\n");
    
    // 等待一下（通过简单循环）
    for (volatile int i = 0; i < 1000000; i++);
    
    puts("系统正常关机\n");
    sbi_shutdown();
}