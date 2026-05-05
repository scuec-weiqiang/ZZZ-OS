# ZZZ-OS

A lightweight, Unix-like hobby operating system written in C, supporting **ARMv7-A** (Cortex-A7) and **RISC-V64** architectures. Designed with a Linux-inspired layered architecture: device tree boot, bus-device-driver model, VFS with dentry/inode cache, buddy + slab memory allocator, and round-robin preemptive scheduler.

## Features

| Subsystem | Status | Description |
|---|---|---|
| **Multi-arch boot** | Done | ARM32 (iMX6ULL) & RISC-V64 (QEMU virt) |
| **Device Tree** | Done | FDT parser, OF platform bus, device/driver matching |
| **IRQ framework** | Done | irq_chip + irq_domain + deferred work, Linux-style 3-layer model |
| **Timer / Timekeeping** | Done | clocksource + clockevent + timerqueue |
| **GPIO framework** | Done | gpio_chip + gpiolib + basic_mmio_gpio |
| **Memory Management** | Done | memblock, buddy, slab, VMA, page fault, lazy/eager mapping |
| **Scheduler** | Done | Pluggable sched_class, RR + idle, per-CPU runqueue, time-slice preemption |
| **Process Management** | Done | fork / execve / exit / waitpid, kernel threads |
| **VFS** | Done | dcache, icache, mount namespace, page cache |
| **Filesystems** | Done | ramfs, ext2 (read/write) |
| **ELF Loader** | Done | User-space ELF binary execution |
| **User-space** | Done | newlib-based C library support, shell |
| **Device Model** | Done | bus / device / driver framework, platform bus |
| **Syscalls** | Done | 13 syscalls via dispatch table |

## Supported Platforms

| Architecture | Board | Cross Compiler | Load Address | IRQ Controller |
|---|---|---|---|---|
| ARMv7-A (32-bit) | iMX6ULL | `arm-none-eabi-` | `0xc0200000` | ARM GIC |
| RISC-V64 (64-bit) | QEMU virt | `riscv64-unknown-elf-` | `0x80200000` | RISC-V PLIC |

## Quick Start

### Build for ARM (iMX6ULL)

```bash
make os ARCH=arm BOARD=imx6ull
```

### Build & Run on QEMU (RISC-V)

```bash
make os ARCH=riscv64 BOARD=virt
make run ARCH=riscv64 BOARD=virt
```

### Build User-space Programs

```bash
make u
```

### Clean

```bash
make distclean
```

## Project Structure

```
.
├── arch/                    # Architecture-specific code
│   ├── arm/                 #   ARMv7-A (Cortex-A7, iMX6ULL)
│   │   ├── boot/dts/        #     Device tree source
│   │   ├── config/          #     config.mk, link.ld
│   │   ├── include/asm/     #     Architecture headers
│   │   ├── mm/              #     Page table implementation
│   │   └── *.S              #     Boot, context switch, vectors
│   └── riscv64/             #   RISC-V64 (QEMU virt)
│       ├── boot/dts/
│       ├── config/
│       ├── include/asm/
│       ├── mm/
│       ├── timer/
│       └── *.S
├── drivers/                 # Drivers & driver framework
│   ├── core/                #   Device/driver/bus/platform framework
│   ├── of/                  #   Device tree helpers (fdt, of_*)
│   ├── irq_chip/            #   IRQ controller drivers (gic, plic)
│   ├── clocksource/         #   Timer drivers (arm_arch_timer)
│   ├── gpio_chip/           #   GPIO drivers (gpiolib, gpio-mxc, gpio-generic)
│   ├── mem_disk/            #   RAM disk driver
│   ├── virt_disk/           #   Virtio block driver
│   ├── imx6ull_uart.c       # UART driver
│   ├── imx6ull_rtc.c        # RTC driver
│   ├── imx6ull_led.c        # LED driver
│   └── qemu_riscv_uart.c    # QEMU RISC-V UART driver
├── fs/                      # VFS & filesystems
│   ├── ext2/                #   ext2 filesystem
│   └── ramfs/               #   ramfs filesystem
├── include/                 # Headers
│   ├── os/                  #   Core kernel headers
│   ├── mm/                  #   MM headers
│   ├── fs/                  #   VFS headers
│   └── dt-bindings/         #   Device tree bindings
├── kernel/                  # Core kernel
│   ├── irq/                 #   IRQ subsystem
│   ├── sched/               #   Scheduler (core, rr, fork, exit, wait)
│   ├── time/                #   Timekeeping & timer subsystem
│   ├── kernel.c             #   Boot entry (start_kernel)
│   └── syscall.c            #   Syscall dispatch
├── lib/                     # Library utilities (string, bitmap, hashtable, lru)
├── mm/                      # Memory management
│   ├── memblock.c           #   Early boot memory
│   ├── physmem.c            #   Physical page map
│   ├── buddy.c              #   Buddy allocator
│   ├── slab.c               #   Slab allocator
│   ├── pgtbl.c              #   Generic page table logic
│   ├── vma.c                #   Virtual memory areas
│   ├── page_fault.c         #   Page fault handler
│   └── uaccess.c            #   User-space access helpers
├── user_runtime/            # User-space runtime (newlib stubs, crt0, syscalls)
├── user_proc/               # User-space programs
│   ├── init/                #   First user process
│   ├── hello/               #   Hello world test
│   └── simple-c-shell/      #   Minimal C shell
├── tools/                   # Build tools
│   ├── kbuild/              #   Build system scanner
│   ├── dtc/                 #   Device tree compiler
│   ├── mkimage              #   U-Boot image wrapper
│   └── mkdisk.sh            #   Disk image creator
└── doc/                     # Documentation
```

## Architecture Overview

### Boot Flow

```
Bootloader (U-Boot)
  → loads kernel.elf + device tree.dtb
  → jumps to arch-specific entry (boot.S)
  → sets up MMU, stack, page tables
  → calls start_kernel()
    → early_malloc_init()
    → fdt_init(dtb)              // parse device tree
    → memblock_init()            // physical memory management
    → initial_mm_init()          // kernel MM
    → kmalloc_init()             // slab allocator
    → irq_init()                 // interrupt subsystem
    → time_init()                // timer / timekeeping
    → sched_init()               // scheduler
    → kernel_thread(kernel_init) // kernel init thread
    → kernel_thread(kthreadd)    // kernel thread daemon
    → idle loop
```

`kernel_init` thread:
```
of_platform_populate()           // create platform devices from DT
arch_initcalls_run()
core_initcalls_run()
fs_initcalls_run()
device_initcalls_run()
mount_root("/dev/ram_disk", "ext2")
late_initcalls_run()
setup_stdio("/dev/uart0")
do_execve("/bin/init", ...)      // launch user-space init
```

### Key Design Decisions

- **Linux-style layered architecture**: IRQ (irq_chip → irq_domain → irq_data), driver model (bus → device → driver), VFS (dentry → inode → superblock)
- **Device tree boot**: All hardware discovered via FDT, no hardcoded platform devices
- **Lazy vs Eager mapping**: User-space uses VMA + page fault (lazy); kernel MMIO and known mappings are eager
- **Pluggable scheduler**: `sched_class` interface supports multiple scheduling policies; currently RR + idle
- **OF_DECLARE pattern**: Drivers register via `IRQCHIP_DECLARE()`, `TIMERCHIP_DECLARE()` macros, auto-discovered at boot via DT compatible strings

## Documentation

- [Memory Management](doc/mm.md) — Page tables, buddy, slab, VMA, lazy vs eager mapping
- [Filesystem](doc/fs.md) — VFS, mount context, namespace
- [IRQ Subsystem](doc/irq.md) — irq_chip, irq_domain, deferred work
- [Scheduler](doc/sched.md) — sched_class, RR, fork/exec, wait
- [Driver Framework](doc/driver.md) — bus/device/driver model, platform bus
- [Device Tree](doc/device-tree.md) — FDT parsing, OF helpers
- [User-space](doc/userspace.md) — newlib integration, syscalls, user programs
- [Build System](doc/build.md) — Makefile, kbuild, disk images

## License

See the source headers for copyright information.
