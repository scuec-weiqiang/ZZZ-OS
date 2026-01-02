#!/usr/bin/env bash
set -euo pipefail

# ========= 配置 =========
IMG=${1:-disk.img}
IMG_SIZE_MB=16
PART_START=1MiB
PART_END=15MiB
FS=ext2
INODE_SIZE=128

echo "[*] Creating disk image: $IMG"

# ========= 创建镜像 =========
rm -f "$IMG"
dd if=/dev/urandom of="$IMG" bs=1M count=$IMG_SIZE_MB status=progress

# ========= 分区 =========
parted -s "$IMG" \
    mklabel gpt \
    mkpart primary $FS $PART_START $PART_END \
    print

# ========= 绑定 loop =========
LOOPDEV=$(sudo losetup -Pf --show "$IMG")
echo "[*] Loop device: $LOOPDEV"

cleanup() {
    echo "[*] Cleaning up loop device"
    sudo losetup -d "$LOOPDEV"
}
trap cleanup EXIT

# ========= 格式化 =========
sudo mkfs.$FS -I $INODE_SIZE "${LOOPDEV}p1"

echo "[✓] Disk image ready: $IMG"
