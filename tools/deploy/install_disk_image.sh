#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
. "${SCRIPT_DIR}/common.sh"

require_cmd make
require_cmd losetup
require_cmd mount

IMAGE=
ARCH=
BOARD=
DEPLOY_DIR=
SKIP_USERSPACE=0
CROSS_COMPILE_ARG=${CROSS_COMPILE:-}

usage() {
    cat <<'EOF'
Usage:
  install_disk_image.sh --image <path> --arch <arch> --board <board> [--cross-compile <prefix>] [--deploy-dir <dir>] [--skip-userspace]
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --image)
            IMAGE=$2
            shift 2
            ;;
        --arch)
            ARCH=$2
            shift 2
            ;;
        --board)
            BOARD=$2
            shift 2
            ;;
        --cross-compile)
            CROSS_COMPILE_ARG=$2
            shift 2
            ;;
        --deploy-dir)
            DEPLOY_DIR=$2
            shift 2
            ;;
        --skip-userspace)
            SKIP_USERSPACE=1
            shift
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

if [ -z "${IMAGE}" ] || [ -z "${ARCH}" ] || [ -z "${BOARD}" ]; then
    usage >&2
    exit 1
fi

if [ -z "${CROSS_COMPILE_ARG}" ]; then
    echo "missing CROSS_COMPILE; pass --cross-compile <prefix> or export CROSS_COMPILE" >&2
    exit 1
fi

if [ -z "${DEPLOY_DIR}" ]; then
    DEPLOY_DIR=$(deploy_dir_for "${ARCH}" "${BOARD}")
fi

uimage="${DEPLOY_DIR}/uImage"
dtb="${DEPLOY_DIR}/${BOARD}.dtb"
boot_cmd="${DEPLOY_DIR}/boot.cmd"
boot_scr="${DEPLOY_DIR}/boot.scr"

if [ ! -f "${uimage}" ] || [ ! -f "${dtb}" ]; then
    echo "missing build artifacts in ${DEPLOY_DIR}" >&2
    echo "run: make ARCH=${ARCH} CROSS_COMPILE=<toolchain-prefix> BOARD=${BOARD} os" >&2
    exit 1
fi

if [ ! -f "${boot_cmd}" ] || [ ! -f "${boot_scr}" ]; then
    "${SCRIPT_DIR}/generate_bootscript.sh" \
        --arch "${ARCH}" \
        --board "${BOARD}" \
        --dtb "$(basename -- "${dtb}")" \
        --output-dir "${DEPLOY_DIR}"
fi

if [ ! -f "${IMAGE}" ]; then
    echo "image not found: ${IMAGE}" >&2
    echo "run: ${SCRIPT_DIR}/create_disk_image.sh --image ${IMAGE}" >&2
    exit 1
fi

tmpdir=$(mktemp -d)
boot_mnt="${tmpdir}/boot"
root_mnt="${tmpdir}/root"
loopdev=$(attach_loop "${IMAGE}")

cleanup() {
    umount_if_mounted "${boot_mnt}"
    umount_if_mounted "${root_mnt}"
    detach_loop "${loopdev}"
    rm -rf "${tmpdir}"
}
trap cleanup EXIT

mount_partition "${loopdev}" 1 "${boot_mnt}"
mount_partition "${loopdev}" 2 "${root_mnt}"

sudo mkdir -p "${boot_mnt}" "${root_mnt}/bin" "${root_mnt}/etc" "${root_mnt}/dev" "${root_mnt}/tmp"
sudo chmod 1777 "${root_mnt}/tmp"
sudo cp "${uimage}" "${boot_mnt}/uImage"
sudo cp "${dtb}" "${boot_mnt}/"
# sudo cp ~/dash/build-riscv64/src/dash "${root_mnt}/bin/"

sudo cp "${boot_cmd}" "${boot_mnt}/"
sudo cp "${boot_scr}" "${boot_mnt}/"

cat > "${tmpdir}/os-release" <<EOF
NAME=ZZZ-OS
ARCH=${ARCH}
BOARD=${BOARD}
EOF
sudo cp "${tmpdir}/os-release" "${root_mnt}/etc/os-release"

if [ "${SKIP_USERSPACE}" -eq 0 ]; then
    (
        cd "${REPO_ROOT}"
        make u ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE_ARG}" INSTALL_DIR="${root_mnt}/bin"
    )
fi

echo "installed boot and rootfs contents into ${IMAGE}"
