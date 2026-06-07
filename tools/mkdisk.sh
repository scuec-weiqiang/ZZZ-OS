#!/usr/bin/env bash
set -euo pipefail

# ========= 配置 =========
IMG=${1:-rootfs.ext2}
IMG_SIZE_MB=5
PART_START=1MiB
PART_END=4MiB
FS=ext2
INODE_SIZE=256
PAUSE_AFTER_MKFS=${PAUSE_AFTER_MKFS:-0}

echo "[*] Creating disk image: $IMG"

# ========= 创建镜像 =========
rm -f "$IMG"
dd if=/dev/zero of="$IMG" bs=1M count=$IMG_SIZE_MB status=progress

# ========= 分区 =========
parted -s "$IMG" \
    mklabel gpt \
    mkpart primary $FS $PART_START $PART_END \
    print

sudo partprobe -s "$IMG"

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

if [ "$PAUSE_AFTER_MKFS" = "1" ]; then
    echo "[*] 脚本暂停，现在去查看 /dev/loop*p1"
    read -p "按回车继续..."
fi

echo "[✓] Disk image ready: $IMG"
