toolchain("riscv64-gcc")
    set_kind("standalone")
    set_sdkdir("/Volumes/OuterSpace-MBA/Workspace/riscv-learn/tools/riscv64-unknown-elf-toolchain")
    set_toolset("ld", "/Volumes/OuterSpace-MBA/Workspace/riscv-learn/tools/riscv64-unknown-elf-toolchain/bin/riscv64-unknown-elf-ld")
    set_toolset("cc", "/Volumes/OuterSpace-MBA/Workspace/riscv-learn/tools/riscv64-unknown-elf-toolchain/bin/riscv64-unknown-elf-gcc")
    -- set_toolset("as", "/Volumes/OuterSpace-MBA/Workspace/riscv-learn/tools/riscv64-unknown-elf-toolchain/bin/riscv64-unknown-elf-gcc")
toolchain_end()

target("kernel")
        set_default(false)
        set_policy("check.auto_ignore_flags", false)
        set_toolchains("riscv64-gcc")
        set_kind("binary")
        set_targetdir("img/")
        add_includedirs("inc/")
        add_files("src/*.c")
        add_files("src/*.S")
        add_cflags("-march=rv64imac -mabi=lp64 -mcmodel=medany -fno-builtin -fno-stack-protector -nostdlib -ffreestanding -fno-common -g -Wall -Wextra")
        add_asflags("-march=rv64imac -mabi=lp64 -mcmodel=medany -fno-builtin -fno-stack-protector -nostdlib -ffreestanding -fno-common -g -Wall -Wextra")
        add_ldflags("-T kernel.ld -nostdlib -Map img/kernel.map")
        after_build(function (target)
            print("build ok for ")
        end)
    target_end()

target("run")
        set_kind("phony")
        set_default(true)
        add_deps("kernel")
        on_run(function (target)
            local file = "$(buidir)/$(host)/$(arch)/$(mode)/kernel"
            -- print(file)
            local flags = {
                "-machine", "virt", "-cpu", "rv64", "-smp", "1",
                "-bios", "default", "--no-reboot",
                "-nographic", "-m","2048M", 
                "-kernel", "img/kernel"
            }
            -- flags variable will not  extract the actual value of built-in variables
            -- print(flags)
            os.execv("qemu-system-riscv64", flags)
        end)
    target_end()