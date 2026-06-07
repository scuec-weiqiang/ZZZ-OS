echo Booting ZZZ-OS from mmc 0:1
setenv kernel_addr_r 0x80200000
setenv fdt_addr_r 0x83000000
ext2load mmc 0:1 ${kernel_addr_r} /uImage
ext2load mmc 0:1 ${fdt_addr_r} /imx6ull.dtb
bootm ${kernel_addr_r} - ${fdt_addr_r}
