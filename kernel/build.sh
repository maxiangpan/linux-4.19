#bear -- make CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm vexpress_defconfig
#bear -- make CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm -j8
make menuconfig ARCH=arm CROSS_COMPILE=../buildroot/output/host/bin/arm-buildroot-linux-gnueabi-
