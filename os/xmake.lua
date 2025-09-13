target("kernel")
        set_policy("check.auto_ignore_flags", false)
        set_toolchains("clang")
        set_kind("binary")
        add_includedirs("inc/")
        add_files("src/*.c")
        add_cflags("-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fno-stack-protector -ffreestanding -nostdlib")
        add_ldflags("-nostdlib -Tkernel.ld")
    target_end()

target("run")
        set_kind("phony")
        add_deps("kernel")
        set_default(true)

        on_run(function (target)
            import("core.project.config")
            local disk_template = "if=none,format=raw,id=disk,file="
            local flags = {
                "-machine", "virt", "-cpu", "rv64", "-smp", "4",
                "-bios", "default", "--no-reboot",
                "-serial", "mon:stdio", "-m","2048M", 
                "-kernel", config.builddir().."/kernel.elf",
            }
            os.execv("qemu-system-riscv64", flags)
        end)
    target_end()