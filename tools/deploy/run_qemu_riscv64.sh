#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
. "${SCRIPT_DIR}/common.sh"

require_cmd qemu-system-riscv64

BOARD=qemu_virt
IMAGE="${REPO_ROOT}/build/images/${BOARD}.img"
EXTRA_ARGS=()

usage() {
    cat <<'EOF'
Usage:
  run_qemu_riscv64.sh [--image <path>] [-- <extra qemu args>]
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --image)
            IMAGE=$2
            shift 2
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        --)
            shift
            EXTRA_ARGS=("$@")
            break
            ;;
        *)
            echo "unknown argument: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

if [ ! -f "${IMAGE}" ]; then
    echo "image not found: ${IMAGE}" >&2
    echo "prepare it first:" >&2
    echo "  ${SCRIPT_DIR}/create_disk_image.sh --image ${IMAGE}" >&2
    echo "  make ARCH=riscv64 CROSS_COMPILE=<toolchain-prefix> BOARD=${BOARD} os" >&2
    echo "  ${SCRIPT_DIR}/install_disk_image.sh --image ${IMAGE} --arch riscv64 --board ${BOARD} --cross-compile <toolchain-prefix>" >&2
    exit 1
fi

if [ ! -f "${REPO_ROOT}/tools/bootloaders/qemu-riscv64/u-boot.bin" ]; then
    echo "missing bootloader: ${REPO_ROOT}/tools/bootloaders/qemu-riscv64/u-boot.bin" >&2
    exit 1
fi

echo "QEMU is starting with U-Boot."
echo "Use QEMU's nographic console hotkeys if your terminal passes them through."
echo "Typical exit sequence: Ctrl-A X"
echo "If your U-Boot build does not auto-run /boot.scr, run these commands at the prompt:"
echo "  setenv kernel_addr_r 0x80200000"
echo "  setenv fdt_addr_r 0x84000000"
echo "  ext2load virtio 0:1 \${kernel_addr_r} /uImage"
echo "  ext2load virtio 0:1 \${fdt_addr_r} /qemu_virt.dtb"
echo "  bootm \${kernel_addr_r} - \${fdt_addr_r}"

exec qemu-system-riscv64 \
    -nographic \
    -smp 2 \
    -m 256M \
    -machine virt \
    -bios default \
    -kernel "${REPO_ROOT}/tools/bootloaders/qemu-riscv64/u-boot.bin" \
    -cpu rv64,sstc=on \
    -drive "file=${IMAGE},if=none,format=raw,id=disk0" \
    -device virtio-blk-device,drive=disk0,bus=virtio-mmio-bus.0 \
    -global virtio-mmio.force-legacy=false \
    "${EXTRA_ARGS[@]}"
