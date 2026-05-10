#!/usr/bin/env bash
set -euo pipefail

# ========= 配置 =========
IMG=${1:-disk.img}
IMG_SIZE_MB=4
PART_START=1MiB
PART_END=2MiB
FS=ext2
INODE_SIZE=256

echo "[*] Creating disk image: $IMG"

# ========= 创建镜像 =========
rm -f "$IMG"
dd if=/dev/urandom of="$IMG" bs=1M count=$IMG_SIZE_MB status=progress

# ========= 分区 =========
parted -s "$IMG" \
    mklabel gpt \
    mkpart primary $FS $PART_START $PART_END \
    print

sudo partprobe -s "$IMG"

# ========= 绑定 loop =========
LOOPDEV=$(sudo losetup -Pf --show "$IMG")
echo "[*] Loop device: $LOOPDEV"

# cleanup() {
#     echo "[*] Cleaning up loop device"
#     sudo losetup -d "$LOOPDEV"
# }
# trap cleanup EXIT

# ========= 格式化 =========
sudo mkfs.$FS -I $INODE_SIZE "${LOOPDEV}p1"

# 加在这里！让脚本停住，你去查看分区
echo "[*] 脚本暂停，现在去查看 /dev/loop*p1"
read -p "按回车继续..."

echo "[✓] Disk image ready: $IMG"
