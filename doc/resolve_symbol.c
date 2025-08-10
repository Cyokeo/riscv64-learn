/*
 * 简化的符号解析器实现
 * 用于RISC-V重定位处理
 */

#include <stdint.h>
#include <string.h>

/* ELF符号表条目结构 */
typedef struct {
    uint32_t st_name;      /* 符号名在字符串表中的偏移 */
    uint8_t  st_info;      /* 符号类型和绑定信息 */
    uint8_t  st_other;     /* 符号可见性 */
    uint16_t st_shndx;     /* 符号所在段索引 */
    uint64_t st_value;     /* 符号值(地址) */
    uint64_t st_size;      /* 符号大小 */
} Elf64_Sym;

/* 重定位条目结构 */
typedef struct {
    uint64_t r_offset;     /* 重定位位置 */
    uint64_t r_info;       /* 类型和符号索引 */
    int64_t  r_addend;     /* 加数 */
} Elf64_Rela;

/* 符号解析器上下文 */
typedef struct {
    Elf64_Sym *symtab;     /* 符号表起始地址 */
    char      *strtab;     /* 字符串表起始地址 */
    uint32_t   symcount;   /* 符号数量 */
    uint64_t   load_bias;  /* 加载偏移 */
} resolver_context_t;

/* 全局解析器上下文 */
static resolver_context_t g_resolver;

/* 从r_info中提取符号索引 */
#define ELF64_R_SYM(info)  ((info) >> 32)
#define ELF64_R_TYPE(info) ((info) & 0xffffffff)

/* 符号类型宏 */
#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3

/* 符号绑定宏 */
#define STB_LOCAL   0
#define STB_GLOBAL  1
#define STB_WEAK    2

#define ELF64_ST_BIND(info) ((info) >> 4)
#define ELF64_ST_TYPE(info) ((info) & 0xf)

/**
 * 初始化符号解析器
 * @param symtab: 符号表地址
 * @param strtab: 字符串表地址  
 * @param symcount: 符号数量
 * @param load_bias: 加载偏移
 */
void resolver_init(Elf64_Sym *symtab, char *strtab, 
                   uint32_t symcount, uint64_t load_bias)
{
    g_resolver.symtab = symtab;
    g_resolver.strtab = strtab;
    g_resolver.symcount = symcount;
    g_resolver.load_bias = load_bias;
}

/**
 * 按名称查找符号
 * @param name: 符号名称
 * @return: 符号的运行时地址，找不到返回0
 */
uint64_t resolve_symbol_by_name(const char *name)
{
    if (!name || !g_resolver.symtab || !g_resolver.strtab) {
        return 0;
    }

    for (uint32_t i = 0; i < g_resolver.symcount; i++) {
        Elf64_Sym *sym = &g_resolver.symtab[i];
        
        /* 跳过未定义符号 */
        if (sym->st_shndx == 0) {
            continue;
        }
        
        /* 跳过局部符号 */
        if (ELF64_ST_BIND(sym->st_info) == STB_LOCAL) {
            continue;
        }
        
        /* 获取符号名称 */
        const char *sym_name = &g_resolver.strtab[sym->st_name];
        
        /* 比较符号名称 */
        if (strcmp(sym_name, name) == 0) {
            /* 返回调整后的符号地址 */
            return sym->st_value + g_resolver.load_bias;
        }
    }
    
    return 0;  /* 符号未找到 */
}

/**
 * 按符号索引解析符号
 * @param sym_index: 符号表索引
 * @return: 符号的运行时地址
 */
uint64_t resolve_symbol_by_index(uint32_t sym_index)
{
    if (sym_index >= g_resolver.symcount || !g_resolver.symtab) {
        return 0;
    }
    
    Elf64_Sym *sym = &g_resolver.symtab[sym_index];
    
    /* 检查符号是否有效 */
    if (sym->st_shndx == 0) {
        return 0;  /* 未定义符号 */
    }
    
    /* 返回调整后的符号地址 */
    return sym->st_value + g_resolver.load_bias;
}

/**
 * 主要的符号解析函数
 * 用于重定位处理中的符号解析
 * @param rela: 重定位条目
 * @return: 解析后的符号地址
 */
uint64_t resolve_symbol(const Elf64_Rela *rela)
{
    uint32_t sym_index = ELF64_R_SYM(rela->r_info);
    uint32_t reloc_type = ELF64_R_TYPE(rela->r_info);
    
    /* 对于RELATIVE类型，不需要符号解析 */
    if (reloc_type == 3) {  /* R_RISCV_RELATIVE */
        return 0;
    }
    
    return resolve_symbol_by_index(sym_index);
}

/**
 * 完整的重定位处理函数(C语言版本)
 * 展示如何配合符号解析器使用
 */
void process_relocations(Elf64_Rela *rela_start, Elf64_Rela *rela_end, 
                        uint64_t load_bias)
{
    for (Elf64_Rela *rela = rela_start; rela < rela_end; rela++) {
        uint32_t reloc_type = ELF64_R_TYPE(rela->r_info);
        uint64_t *target = (uint64_t *)(rela->r_offset + load_bias);
        
        switch (reloc_type) {
            case 3: /* R_RISCV_RELATIVE */
                *target = rela->r_addend + load_bias;
                break;
                
            case 2: /* R_RISCV_64 */
            {
                uint64_t sym_addr = resolve_symbol(rela);
                if (sym_addr) {
                    *target = sym_addr + rela->r_addend;
                }
                break;
            }
            
            case 1: /* R_RISCV_32 */
            {
                uint64_t sym_addr = resolve_symbol(rela);
                if (sym_addr) {
                    uint32_t *target32 = (uint32_t *)target;
                    *target32 = (uint32_t)(sym_addr + rela->r_addend);
                }
                break;
            }
            
            case 5: /* R_RISCV_JUMP_SLOT */
            {
                uint64_t sym_addr = resolve_symbol(rela);
                if (sym_addr) {
                    *target = sym_addr;
                }
                break;
            }
            
            default:
                /* 未知或不支持的重定位类型 */
                break;
        }
    }
}

/* 调试辅助函数：打印符号表信息 */
void dump_symbol_table(void)
{
    if (!g_resolver.symtab || !g_resolver.strtab) {
        return;
    }
    
    for (uint32_t i = 0; i < g_resolver.symcount; i++) {
        Elf64_Sym *sym = &g_resolver.symtab[i];
        const char *name = &g_resolver.strtab[sym->st_name];
        
        printf("Symbol[%u]: %s = 0x%lx (type=%u, bind=%u)\n",
               i, name, sym->st_value + g_resolver.load_bias,
               ELF64_ST_TYPE(sym->st_info),
               ELF64_ST_BIND(sym->st_info));
    }
}

/*
 * 使用示例：
 * 
 * // 初始化阶段
 * resolver_init(symtab, strtab, symcount, load_bias);
 * 
 * // 查找特定符号
 * uint64_t addr = resolve_symbol_by_name("my_function");
 * 
 * // 重定位处理
 * process_relocations(rela_start, rela_end, load_bias);
 */