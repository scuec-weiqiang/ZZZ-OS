#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
. "${SCRIPT_DIR}/common.sh"

require_cmd truncate
require_cmd parted
require_cmd losetup
require_cmd mkfs.ext2

IMAGE=
IMAGE_SIZE_MIB=16
BOOT_SIZE_MIB=1

usage() {
    cat <<'EOF'
Usage:
  create_disk_image.sh --image <path> [--size-mib <n>] [--boot-size-mib <n>]

Creates a GPT image with:
  partition 1: boot (ext2)
  partition 2: rootfs (ext2)
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --image)
            IMAGE=$2
            shift 2
            ;;
        --size-mib)
            IMAGE_SIZE_MIB=$2
            shift 2
            ;;
        --boot-size-mib)
            BOOT_SIZE_MIB=$2
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

if [ -z "${IMAGE}" ]; then
    usage >&2
    exit 1
fi

if [ "${BOOT_SIZE_MIB}" -ge "${IMAGE_SIZE_MIB}" ]; then
    echo "boot partition must be smaller than the whole image" >&2
    exit 1
fi

mkdir -p "$(dirname -- "${IMAGE}")"
rm -f "${IMAGE}"
truncate -s "${IMAGE_SIZE_MIB}M" "${IMAGE}"

boot_end_mib=$((1 + BOOT_SIZE_MIB))

parted -s "${IMAGE}" \
    mklabel gpt \
    mkpart boot ext2 1MiB "${boot_end_mib}MiB" \
    mkpart rootfs ext2 "${boot_end_mib}MiB" 100% \
    set 1 boot on

loopdev=$(attach_loop "${IMAGE}")
cleanup() {
    detach_loop "${loopdev}"
}
trap cleanup EXIT

sudo mkfs.ext2 -F -L BOOT "${loopdev}p1"
sudo mkfs.ext2 -F -L ROOTFS "${loopdev}p2"

echo "disk image ready: ${IMAGE}"
