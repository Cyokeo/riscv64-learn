# RISC-V64 BIOS Makefile
# reference: https://juejin.cn/post/6891922292075397127

CROSS_COMPILE_PATH = /Volumes/OuterSpace-MBA/Workspace/riscv-learn/tools/riscv64-unknown-elf-toolchain/bin
# 工具链前缀
CROSS_COMPILE ?= $(CROSS_COMPILE_PATH)/riscv64-unknown-elf-

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

# 编译参数
CFLAGS = -march=rv64imac -mabi=lp64 -mcmodel=medany -fno-builtin -fno-stack-protector -nostdlib -nostartfiles -ffreestanding -fno-common -g -Wall -Wextra
LDFLAGS = -T kernel.ld -nostdlib -nostartfiles -Map kernel.map

BUILDDIR = build

# 源文件
SRCS = src/kernel.c
ASMS = src/boot.S
OBJS = $(SRCS:.c=.o) $(ASMS:.S=.o)

# 目标文件
KERNEL = $(BUILDDIR)/kernel.elf
KERNEL_BIN = $(BUILDDIR)/kernel.bin

# 默认目标
all: $(KERNEL_BIN)

# 编译C源文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 编译汇编文件
%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

# 链接生成内核ELF文件
$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(KERNEL)

# 生成二进制文件
$(KERNEL_BIN): $(KERNEL)
	$(OBJCOPY) -O binary $(KERNEL) $(KERNEL_BIN)

# 反汇编（用于调试）
disasm: $(KERNEL)
	$(OBJDUMP) -D $(KERNEL) > $(KERNEL).disasm

# 运行QEMU模拟
run: $(KERNEL)
	qemu-system-riscv64 \
		-machine virt \
		-cpu rv64 \
		-smp 1 \
		-m 128M \
		-nographic \
		-bios default \
		-kernel build/macosx/arm64/release/kernel

# 使用GDB调试
debug: $(KERNEL)
	qemu-system-riscv64 \
		-machine virt \
		-cpu rv64 \
		-smp 1 \
		-m 128M \
		-nographic \
		-bios default \
		-kernel $(KERNEL) \
		-s -S

# 清理生成的文件
clean:
	rm -f $(OBJS) $(KERNEL) $(KERNEL_BIN) $(KERNEL).disasm

# 安装依赖（Ubuntu/Debian）
install-deps:
	sudo apt update
	sudo apt install -y gcc-riscv64-linux-gnu qemu-system-misc

# 显示帮助信息
help:
	@echo "可用的Make目标："
	@echo "  all          - 构建内核（默认）"
	@echo "  run          - 在QEMU中运行内核"
	@echo "  debug        - 在QEMU中以调试模式运行内核"
	@echo "  disasm       - 生成反汇编文件"
	@echo "  clean        - 清理生成的文件"
	@echo "  install-deps - 安装必要的依赖（Ubuntu/Debian）"
	@echo "  help         - 显示此帮助信息"

.PHONY: all run debug disasm clean install-deps help