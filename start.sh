# sudo mount -t ext3 a9rootfs.ext3 tmpfs/ -o loop
# sudo cp -r rootfs/*  tmpfs/
# sudo umount tmpfs

#./build.sh
# qemu-system-arm -M vexpress-a9 -m 512M \
# 	-kernel linux-4.19/arch/arm/boot/zImage \
# 	-dtb  linux-4.19/arch/arm/boot/dts/vexpress-v2p-ca9.dtb \
# 	-nographic \
# 	-append "root=/dev/mmcblk0  console=ttyAMA0" \
# 	-sd a9rootfs.ext3


qemu-system-aarch64 -M vexpress-a9 \
	 -cpu cortex-a53 \
	 -nographic \
	 -smp 1 \
	 #-kernel Image \
	 -kernel linux-4.19/arch/arm/boot/zImage \
	 -dtb  linux-4.19/arch/arm/boot/dts/vexpress-v2p-ca9.dtb \
	 -append "rootwait root=/dev/vda console=ttysole=ttyAMA0" \
	 -drive file=rootfs.ext4
