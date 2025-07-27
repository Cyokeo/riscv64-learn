## 参考
链接：https://juejin.cn/post/6891922292075397127

## virt 设备的内存布局
```c
static const struct MemmapEntry {
    hwaddr base;
    hwaddr size;
} virt_memmap[] = {
    [VIRT_DEBUG] =       {        0x0,         0x100 },
    [VIRT_MROM] =        {     0x1000,        0xf000 },
    [VIRT_TEST] =        {   0x100000,        0x1000 },
    [VIRT_RTC] =         {   0x101000,        0x1000 },
    [VIRT_CLINT] =       {  0x2000000,       0x10000 },
    [VIRT_PCIE_PIO] =    {  0x3000000,       0x10000 },
    [VIRT_PLIC] =        {  0xc000000,     0x4000000 },
    [VIRT_UART0] =       { 0x10000000,         0x100 },
    [VIRT_VIRTIO] =      { 0x10001000,        0x1000 },
    [VIRT_FLASH] =       { 0x20000000,     0x2000000 },
    [VIRT_PCIE_ECAM] =   { 0x30000000,    0x10000000 },
    [VIRT_PCIE_MMIO] =   { 0x40000000,    0x40000000 },
    [VIRT_DRAM] =        { 0x80000000,           0x0 },
};
```
- 要注意，这个`VIRT_FLASH`是需要在qemu命令行中使用**-drive**参数指定的，而且其大小必须为32MB，否则会报错
```bash
qemu-system-riscv64: cfi.pflash01 device '/machine/virt.flash0' requires 33554432 bytes, pflash0 block backend provides 67108864 bytes
```
### qemu启动后，会在MROM区域填充以下内容【二进制代码】
```c
    uint32_t reset_vec[10] = {
        0x00000297,                  /* 1:  auipc  t0, %pcrel_hi(fw_dyn) */
        0x02828613,                  /*     addi   a2, t0, %pcrel_lo(1b) */
        0xf1402573,                  /*     csrr   a0, mhartid  */
#if defined(TARGET_RISCV32)
        0x0202a583,                  /*     lw     a1, 32(t0) */
        0x0182a283,                  /*     lw     t0, 24(t0) */
#elif defined(TARGET_RISCV64)
        0x0202b583,                  /*     ld     a1, 32(t0) */
        0x0182b283,                  /*     ld     t0, 24(t0) */
#endif
        0x00028067,                  /*     jr     t0 */
        start_addr,                  /* start: .dword */
        start_addr_hi32,
        fdt_load_addr,               /* fdt_laddr: .dword */
        0x00000000,
                                     /* fw_dyn: */
    };
```

## qemu riscv64 virt设备启动流程
1. 上电后，PC被设置为0x1000，也即MROM开始的地址;
2. 执行部分指令后，会跳转到start_addr指示的位置处；
    - 因此，如果自己给丁pflash，且不希望使用openSBI的情况下，
    - _start的地址必须与pflash0的起始地址相同，即为0x20000000

### start_addr的取值
1. 如果设置了pflash0，则会设置为pflash0；
2. 如果没有设置pflash0，则会设置为VIRT_DRAM？

### 真实硬件
一般都会有pflash，因此之后就以这个来学习吧