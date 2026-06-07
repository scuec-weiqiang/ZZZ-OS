#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
. "${SCRIPT_DIR}/common.sh"

ARCH=
BOARD=
OUTPUT_DIR=
DTB_NAME=

usage() {
    cat <<'EOF'
Usage:
  generate_bootscript.sh --arch <arch> --board <board> --dtb <name> --output-dir <dir>
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --arch)
            ARCH=$2
            shift 2
            ;;
        --board)
            BOARD=$2
            shift 2
            ;;
        --dtb)
            DTB_NAME=$2
            shift 2
            ;;
        --output-dir)
            OUTPUT_DIR=$2
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "unknown argument: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

if [ -z "${ARCH}" ] || [ -z "${BOARD}" ] || [ -z "${OUTPUT_DIR}" ] || [ -z "${DTB_NAME}" ]; then
    usage >&2
    exit 1
fi

mkdir -p "${OUTPUT_DIR}"

boot_media=$(boot_media_for "${ARCH}" "${BOARD}")
kernel_addr=$(kernel_addr_for "${ARCH}")
fdt_addr=$(fdt_addr_for "${ARCH}")
mkimage_arch=$(mkimage_arch_for "${ARCH}")
boot_cmd="${OUTPUT_DIR}/boot.cmd"
boot_scr="${OUTPUT_DIR}/boot.scr"
board_boot_cmd="$(bootloader_dir_for "${ARCH}" "${BOARD}")/boot.cmd"

if [ -f "${board_boot_cmd}" ]; then
    cp "${board_boot_cmd}" "${boot_cmd}"
else
    cat > "${boot_cmd}" <<EOF
echo Booting ZZZ-OS from ${boot_media}
setenv kernel_addr_r ${kernel_addr}
setenv fdt_addr_r ${fdt_addr}
ext2load ${boot_media} \${kernel_addr_r} /uImage
ext2load ${boot_media} \${fdt_addr_r} /${DTB_NAME}
bootm \${kernel_addr_r} - \${fdt_addr_r}
EOF
fi

"${REPO_ROOT}/tools/mkimage" -A "${mkimage_arch}" -O linux -T script -C none \
    -n "ZZZ-OS boot" -d "${boot_cmd}" "${boot_scr}"

echo "generated ${boot_cmd}"
echo "generated ${boot_scr}"
