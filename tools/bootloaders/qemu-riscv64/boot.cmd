echo Booting ZZZ-OS from virtio 0:1
setenv kernel_addr_r 0x80200000
setenv fdt_addr_r 0x84000000
ext2load virtio 0:1 ${kernel_addr_r} /uImage
ext2load virtio 0:1 ${fdt_addr_r} /qemu_virt.dtb
bootm ${kernel_addr_r} - ${fdt_addr_r}
