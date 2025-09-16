# RISC-V 64位增强内核# RISC-V 64位简单内核

这是一个使用C语言编写的简单RISC-V 64位内核程序，通过OpenSBI的ecall功能在QEMU上运行并输出"Hello World"。

## 文件结构

- `kernel.c` - 内核主程序，包含SBI调用接口和主要逻辑
- `boot.S` - 启动汇编代码，设置栈和跳转到C代码
- `kernel.ld` - 链接器脚本，定义内存布局
- `Makefile` - 构建脚本

## 系统要求

### Ubuntu/Debian系统
```bash
# 安装RISC-V工具链和QEMU
sudo apt update
sudo apt install gcc-riscv64-linux-gnu qemu-system-misc

# 或者使用Makefile安装
make install-deps
```

### 其他系统
- RISC-V 64位交叉编译工具链 (`riscv64-linux-gnu-gcc`)
- QEMU RISC-V模拟器 (`qemu-system-riscv64`)

## 构建和运行

### 1. 构建内核
```bash
make
```

### 2. 运行内核
```bash
make run
```

### 3. 调试内核
```bash
# 终端1：启动QEMU调试模式
make debug

# 终端2：连接GDB
riscv64-linux-gnu-gdb kernel
(gdb) target remote localhost:1234
(gdb) continue
```

### 4. 查看反汇编
```bash
make disasm
cat kernel.disasm
```

### 5. 清理文件
```bash
make clean
```

## 程序工作原理

1. **启动过程**：
   - QEMU加载内核到0x80200000地址
   - `boot.S`设置栈指针，清零BSS段
   - 跳转到C语言的`kernel_main`函数

2. **SBI调用**：
   - 使用OpenSBI提供的系统调用接口
   - 通过`ecall`指令调用SBI服务
   - 实现控制台字符输出和系统关机

3. **输出和关机**：
   - 输出"Hello World"消息
   - 调用SBI关机服务正常退出

## 预期输出

运行`make run`后，应该看到以下输出：

```
Hello World from RISC-V 64 Kernel!
This is a simple kernel running on QEMU with OpenSBI
Shutting down...
```

然后QEMU会自动退出。

## 技术细节

- **目标架构**：RISC-V 64位 (rv64imac)
- **内存模型**：medany
- **加载地址**：0x80200000
- **SBI版本**：兼容OpenSBI
- **栈大小**：4KB

## 故障排除

### 1. 工具链问题
确保安装了正确的RISC-V工具链：
```bash
riscv64-linux-gnu-gcc --version
```

### 2. QEMU问题
确保QEMU支持RISC-V：
```bash
qemu-system-riscv64 --version
```

### 3. 编译错误
检查是否缺少头文件或库文件：
```bash
make clean
make
```

## 扩展建议

1. 添加更多SBI调用功能（定时器、中断处理等）
2. 实现简单的内存管理
3. 添加设备驱动程序
4. 实现用户态程序加载

## 许可证

本项目仅用于学习和教育目的。