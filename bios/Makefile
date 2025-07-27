# RISC-V64 BIOS Makefile
# reference: https://juejin.cn/post/6891922292075397127

CROSS_COMPILE_PATH = /Volumes/OuterSpace-MBA/Workspace/starFive/tools/riscv64-unknown-elf-toolchain/bin
# 工具链前缀
CROSS_COMPILE ?= $(CROSS_COMPILE_PATH)/riscv64-unknown-elf-

# 工具链
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

# 编译选项
ASFLAGS = -march=rv64ima -mabi=lp64
LDFLAGS = -T link.ld -nostdlib

# 目标文件
TARGET = riscv_bios
SOURCES = bios.S
OBJECTS = $(SOURCES:.S=.o)

# 默认目标
all: $(TARGET).bin pflash.img

# 编译汇编文件
%.o: %.S
	$(AS) $(ASFLAGS) -o $@ $<

# 链接生成ELF文件
$(TARGET).elf: $(OBJECTS) link.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

# 生成二进制文件
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# 创建pflash镜像文件 (32MB, 填充0xFF)
pflash.img: $(TARGET).bin
	# 创建确切32MB的文件，填充0xFF (Flash擦除状态)
	python3 -c "with open('$@', 'wb') as f: f.write(b'\xFF' * 33554432)"
	# 将BIOS二进制文件写入pflash开头
	dd if=$(TARGET).bin of=$@ conv=notrunc bs=1

# 生成反汇编文件用于调试
$(TARGET).dis: $(TARGET).elf
	$(OBJDUMP) -d $< > $@

# 清理
clean:
	rm -f *.o *.elf *.bin *.dis pflash.img

# 在QEMU中测试 - 使用-bios选项
test: $(TARGET).bin
	qemu-system-riscv64 \
		-machine virt \
		-cpu rv64 \
		-m 128M \
		-bios $(TARGET).bin \
		-nographic

# 在QEMU中测试 - 使用pflash设备
# -drive : set pflash0, its base address is 0x20000000 with len = 32MB
# -bios none: tell qemu not load default openSBI
# self bios's _start must locate at 0x20000000
test-pflash: pflash.img
	qemu-system-riscv64 \
		-machine virt \
		-cpu rv64 \
		-m 128M \
		-bios none \
		-drive if=pflash,format=raw,file=pflash.img,readonly=on \
		-nographic

# 调试模式运行 - 使用pflash
debug-pflash: pflash.img
	qemu-system-riscv64 \
		-machine virt \
		-cpu rv64 \
		-m 128M \
		-bios none \
		-drive if=pflash,format=raw,file=pflash.img,readonly=on \
		-nographic \
		-s -S

# 调试模式运行 - 使用-bios
debug: $(TARGET).bin
	qemu-system-riscv64 \
		-machine virt \
		-cpu rv64 \
		-m 128M \
		-bios $(TARGET).bin \
		-nographic \
		-s -S

# 带图形界面的测试（可选）
test-gui: $(TARGET).bin
	qemu-system-riscv64 \
		-machine virt \
		-cpu rv64 \
		-m 128M \
		-bios $(TARGET).bin \
		-serial stdio

# 显示pflash信息
info-pflash: pflash.img
	@echo "PFLASH镜像信息:"
	@ls -lh pflash.img
	@echo "文件大小(字节): $(wc -c < pflash.img)"
	@echo "期望大小: 33554432 字节 (32MB)"
	@echo "前64字节的十六进制内容:"
	@hexdump -C pflash.img | head -4

# 验证pflash大小
verify-pflash: pflash.img
	@SIZE=$(wc -c < pflash.img); \
	if [ $SIZE -eq 33554432 ]; then \
		echo "✓ PFLASH镜像大小正确: $SIZE 字节"; \
	else \
		echo "✗ PFLASH镜像大小错误: $SIZE 字节 (期望: 33554432)"; \
		exit 1; \
	fi

# 显示帮助信息
help:
	@echo "可用的make目标:"
	@echo "  all          - 编译BIOS并创建pflash镜像"
	@echo "  test         - 使用-bios选项运行QEMU"
	@echo "  test-pflash  - 使用pflash设备运行QEMU (推荐)"
	@echo "  test-gui     - 带图形界面运行QEMU"
	@echo "  debug        - 使用-bios选项调试模式"
	@echo "  debug-pflash - 使用pflash设备调试模式"
	@echo "  info-pflash  - 显示pflash镜像信息"
	@echo "  verify-pflash- 验证pflash镜像大小"
	@echo "  clean        - 清理生成的文件"
	@echo "  help         - 显示此帮助信息"

.PHONY: all clean test debug test-gui test-pflash debug-pflash info-pflash verify-pflash help