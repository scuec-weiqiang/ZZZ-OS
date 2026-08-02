# Deploy Helpers

`tools/deploy/` contains helper scripts that sit on top of the main build.

The top-level `Makefile` is intentionally kept focused on compilation:

- `make ARCH=<arch> CROSS_COMPILE=<toolchain-prefix> BOARD=<board> os` stages `uImage`, `dtb`, `boot.cmd`, and `boot.scr`
- helper scripts prepare boot/rootfs media and optional QEMU runs

Prebuilt bootloader binaries used by deployment helpers live under:

- `tools/bootloaders/`

For example, the RISC-V QEMU helper uses:

- `tools/bootloaders/qemu-riscv64/u-boot.bin`

For ARM hardware targets, the recommended convention is to place board-specific
U-Boot images under:

- `tools/bootloaders/imx6ull/`

## Standard layout

The helpers assume a GPT disk with two partitions:

1. `boot`
   Stores `uImage`, `<board>.dtb`, `boot.cmd`, `boot.scr`
2. `rootfs`
   Stores `/bin/init` and the rest of the userspace payload

Both partitions currently use `ext2`.

## Scripts

- `create_disk_image.sh`
  Creates a blank GPT disk image with boot/rootfs partitions.

- `install_disk_image.sh`
  Installs build artifacts, userspace programs and a generated U-Boot boot script.

- `generate_bootscript.sh`
  Generates `boot.cmd` and `boot.scr` for a supported board.
  If `tools/bootloaders/<platform>/boot.cmd` exists, it is used as the source.

- `run_qemu_riscv64.sh`
  QEMU launcher for `riscv64/qemu_virt`.

## Quick start

Build first:

```bash
make ARCH=<arch> CROSS_COMPILE=<toolchain-prefix> BOARD=<board> os
```

This stages boot artifacts in:

```text
build/deploy/<arch>/<board>/
```

To override the generated boot script, add:

- `tools/bootloaders/<platform>/boot.cmd`

Create and populate an image:

```bash
tools/deploy/create_disk_image.sh --image build/images/qemu_virt.img
tools/deploy/install_disk_image.sh --image build/images/qemu_virt.img --arch riscv64 --board qemu_virt
```

The repository includes the unmodified DOOM 1.9 shareware WAD for no-charge
testing under its separate, non-open-source shareware terms. You can select
another legally obtained WAD explicitly:

```bash
tools/deploy/install_disk_image.sh \
    --image build/images/qemu_virt.img \
    --arch riscv64 \
    --board qemu_virt \
    --doom-wad /path/to/legally-obtained/doom.wad
```

Alternatively, set `ZZZ_DOOM_WAD`. Without an override, the bundled
`user_proc/doom/doomq.wad` is installed.

Then launch QEMU:

```bash
tools/deploy/run_qemu_riscv64.sh
```

## Hardware-first flow

For a real board such as `arm/imx6ull`, the recommended workflow is:

1. `make ARCH=<arch> CROSS_COMPILE=<toolchain-prefix> BOARD=<board> os`
2. create a disk image with `create_disk_image.sh`
3. install boot/rootfs with `install_disk_image.sh`
4. write the finished image to SD/eMMC with your normal flashing method
5. configure U-Boot to load `uImage` and `dtb` from partition 1

If you keep the board's U-Boot images in this repository, store them under
`tools/bootloaders/imx6ull/`, together with any board-specific `boot.cmd`,
rather than under `arch/arm/`.

The same boot/rootfs layout is therefore shared by both QEMU and hardware.
