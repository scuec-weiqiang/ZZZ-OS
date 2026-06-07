#!/usr/bin/env bash
set -euo pipefail

DEPLOY_TOOLS_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "${DEPLOY_TOOLS_DIR}/../.." && pwd)

require_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "missing required command: $1" >&2
        exit 1
    fi
}

attach_loop() {
    sudo losetup -Pf --show "$1"
}

detach_loop() {
    local loopdev=$1
    sudo losetup -d "$loopdev" >/dev/null 2>&1 || true
}

mount_partition() {
    local loopdev=$1
    local partno=$2
    local mountpoint=$3
    sudo mkdir -p "$mountpoint"
    sudo mount "${loopdev}p${partno}" "$mountpoint"
}

umount_if_mounted() {
    local mountpoint=$1
    sudo umount "$mountpoint" >/dev/null 2>&1 || true
}

boot_media_for() {
    local arch=$1
    local board=$2

    case "${arch}:${board}" in
        riscv64:qemu_virt)
            printf '%s\n' 'virtio 0:1'
            ;;
        arm:imx6ull)
            printf '%s\n' 'mmc 0:1'
            ;;
        *)
            echo "unsupported arch/board: ${arch}/${board}" >&2
            exit 1
            ;;
    esac
}

kernel_addr_for() {
    local arch=$1

    case "$arch" in
        riscv64|arm)
            printf '%s\n' '0x80200000'
            ;;
        *)
            echo "unsupported arch: ${arch}" >&2
            exit 1
            ;;
    esac
}

fdt_addr_for() {
    local arch=$1

    case "$arch" in
        riscv64)
            printf '%s\n' '0x84000000'
            ;;
        arm)
            printf '%s\n' '0x83000000'
            ;;
        *)
            echo "unsupported arch: ${arch}" >&2
            exit 1
            ;;
    esac
}

mkimage_arch_for() {
    local arch=$1

    case "$arch" in
        riscv64)
            printf '%s\n' 'riscv'
            ;;
        arm)
            printf '%s\n' 'arm'
            ;;
        *)
            echo "unsupported arch: ${arch}" >&2
            exit 1
            ;;
    esac
}

bootloader_dir_for() {
    local arch=$1
    local board=$2

    case "${arch}:${board}" in
        riscv64:qemu_virt)
            printf '%s\n' "${REPO_ROOT}/tools/bootloaders/qemu-riscv64"
            ;;
        arm:imx6ull)
            printf '%s\n' "${REPO_ROOT}/tools/bootloaders/imx6ull"
            ;;
        *)
            echo "unsupported arch/board: ${arch}/${board}" >&2
            exit 1
            ;;
    esac
}

deploy_dir_for() {
    local arch=$1
    local board=$2

    printf '%s\n' "${REPO_ROOT}/build/deploy/${arch}/${board}"
}
