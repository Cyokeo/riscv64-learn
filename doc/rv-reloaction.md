# RISC-V主要重定位类型
## 重定位表项结构
```c
struct rela_entry {
    uintptr_t r_offset;    /* 需要重定位的地址位置 */
    uintptr_t r_info;      /* 重定位类型和符号索引 */
    intptr_t  r_addend;    /* 重定位加数 */
};
```
## 动态链接器使用的重定位类型：
```c
// 常见的RISC-V重定位类型
#define R_RISCV_NONE      0   /* 无重定位 */
#define R_RISCV_32        1   /* 32位绝对地址 */
#define R_RISCV_64        2   /* 64位绝对地址 */
#define R_RISCV_RELATIVE  3   /* 相对基址重定位 */
#define R_RISCV_COPY      4   /* 复制重定位 */
#define R_RISCV_JUMP_SLOT 5   /* 跳转槽重定位 */
#define R_RISCV_TLS_DTPMOD32 6  /* TLS模块ID */
#define R_RISCV_TLS_DTPREL32 7  /* TLS偏移 */
#define R_RISCV_TLS_TPREL32  8  /* TLS线程偏移 */
```

# 不同类型的重定位处理
## 前置代码
```asm
	/* relocate the global table content */
	li	t0, FW_TEXT_START	/* link start */
	lla	t1, _fw_start		/* load start */
	sub	t2, t1, t0		/* load offset */
	lla	t0, __rela_dyn_start
	lla	t1, __rela_dyn_end
	beq	t0, t1, _relocate_done
2:
	REG_L	t5, __SIZEOF_LONG__(t0)	/* t5 <-- relocation info:type */
	li	t3, R_RISCV_RELATIVE	/* reloc type R_RISCV_RELATIVE */
	bne	t5, t3, 3f
	REG_L	t3, 0(t0)
	REG_L	t5, (__SIZEOF_LONG__ * 2)(t0)	/* t5 <-- addend */
	add	t5, t5, t2
	add	t3, t3, t2
	REG_S	t5, 0(t3)		/* store runtime address to the GOT entry */

3:
	addi	t0, t0, (__SIZEOF_LONG__ * 3)
	blt	t0, t1, 2b
_relocate_done:
```
## R_RISCV_64 (绝对地址重定位)
### 使用场景：
- 外部符号的绝对地址引用
- 需要知道符号的确切运行时地址
### 处理代码
```asm
/* 处理 R_RISCV_64 */
li   t3, R_RISCV_64
bne  t5, t3, check_next_type

REG_L t3, 0(t0)              /* t3 = 重定位位置 */
REG_L t5, (__SIZEOF_LONG__ * 2)(t0)  /* t5 = addend */
REG_L t4, symbol_value       /* t4 = 符号的运行时地址 */
add  t5, t4, t5              /* 新值 = 符号地址 + addend */
add  t3, t3, t2              /* 重定位位置 + load_offset */
REG_S t5, 0(t3)              /* 存储最终值 */
```

## R_RISCV_JUMP_SLOT (函数跳转槽)
### 使用场景：
- 动态链接的函数调用
- PLT (Procedure Linkage Table) 相关

### 处理代码
```asm
/* 处理 R_RISCV_JUMP_SLOT */
li   t3, R_RISCV_JUMP_SLOT
bne  t5, t3, check_next_type

REG_L t3, 0(t0)              /* t3 = GOT条目地址 */
REG_L t4, symbol_address     /* t4 = 函数实际地址 */
add  t3, t3, t2              /* GOT位置 + load_offset */
REG_S t4, 0(t3)              /* GOT[entry] = 函数地址 */
```

## R_RISCV_32 (32位绝对地址)
### 处理代码
```asm
/* 处理 R_RISCV_32 */
li   t3, R_RISCV_32
bne  t5, t3, check_next_type

REG_L t3, 0(t0)              /* t3 = 重定位位置 */
REG_L t5, (__SIZEOF_LONG__ * 2)(t0)  /* t5 = addend */
REG_L t4, symbol_value       /* t4 = 符号地址 */
add  t5, t4, t5              /* 计算最终值 */
add  t3, t3, t2              /* 调整位置 */
/* 注意：只存储32位 */
sw   t5, 0(t3)               /* 存储32位值 */
```
## 完整的重定位处理循环
```asm
/* 扩展版本的重定位处理 */
_relocate_loop:
    REG_L t5, __SIZEOF_LONG__(t0)     /* t5 = r_info */
    li    t6, 0xFF
    and   t5, t5, t6                  /* t5 = reloc_type */
    
    /* 检查 R_RISCV_RELATIVE */
    li    t3, R_RISCV_RELATIVE
    beq   t5, t3, handle_relative
    
    /* 检查 R_RISCV_64 */
    li    t3, R_RISCV_64
    beq   t5, t3, handle_64
    
    /* 检查 R_RISCV_32 */
    li    t3, R_RISCV_32
    beq   t5, t3, handle_32
    
    /* 检查 R_RISCV_JUMP_SLOT */
    li    t3, R_RISCV_JUMP_SLOT
    beq   t5, t3, handle_jump_slot
    
    j next_entry

handle_relative:
    /* 之前讨论的 RELATIVE 处理 */
    REG_L t3, 0(t0)
    REG_L t5, (__SIZEOF_LONG__ * 2)(t0)
    add   t5, t5, t2
    add   t3, t3, t2
    REG_S t5, 0(t3)
    j next_entry

handle_64:
    /* 需要符号查找，通常在动态链接器中 */
    REG_L t3, 0(t0)                    /* 重定位位置 */
    REG_L t4, (__SIZEOF_LONG__ * 2)(t0) /* addend */
    /* 这里需要符号解析机制 */
    call  resolve_symbol               /* 解析符号地址 */
    add   t4, a0, t4                   /* 符号地址 + addend */
    add   t3, t3, t2                   /* 调整位置 */
    REG_S t4, 0(t3)
    j next_entry

handle_jump_slot:
    /* PLT/GOT 相关处理 */
    REG_L t3, 0(t0)                    /* GOT条目 */
    call  resolve_symbol               /* 解析函数地址 */
    add   t3, t3, t2                   /* 调整GOT位置 */
    REG_S a0, 0(t3)                    /* 更新GOT条目 */
    j next_entry

next_entry:
    addi  t0, t0, (__SIZEOF_LONG__ * 3) /* 下一个条目 */
    blt   t0, t1, _relocate_loop
```

## 不同重定位类型的产生场景

## 为什么OpenSBI只处理RELATIVE？
OpenSBI作为固件，通常：
- 自包含 - 不依赖外部符号
- 静态链接 - 不需要动态链接
- 位置无关 - 主要需要调整内部地址
因此只需要处理 R_RISCV_RELATIVE 类型，这也是为什么代码中只看到这一种类型的处理。
其他重定位类型如R_RISCV_32、R_RISCV_64、R_RISCV_JUMP_SLOT主要用于动态链接场景 Binutils/gas/ld port for RISC-V，在完整的操作系统动态链接器中才会遇到。