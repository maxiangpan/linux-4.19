#!/bin/bash

#make ARCH=arm64 CROSS_COMPILE=/home/mxp/Desktop/linux/buildroot-2024.02.2/output/host/bin/aarch64-buildroot-linux-gnu- -j4
CURRENT_DIR=$(pwd)
NPROC=`nproc`
export TE_JOBS=$NPROC

function check_config(){
	unset missing
	for var in $@; do
		eval [ \$$var ] && continue

		missing="$missing $var"
	done

	[ -z "$missing" ] && return 0

	echo "Skipping ${FUNCNAME[1]} for missing configs: $missing."
	return 1
}

function finish_build(){
	echo "Running ${FUNCNAME[1]} succeeded."
	cd $TOP_DIR
}

function config(){
    KERNEL_DEFCONFIG=te_defconfig
    BUILDROOT_DEFCONFIG=te_defconfig
    UBOOT_DEFCONFIG=te_defconfig
    KERNEL_DTS=te_vxpress

    TE_ARCH=arm64
    TE_CROSS_COMPILE=${CURRENT_DIR}/buildroot/output/host/bin/aarch64-buildroot-linux-gnu-
}

function build_kernel(){ 
    #make CROSS_COMPILE=$TE_CROSS_COMPILE -j$TE_JOBS
    make $KERNEL_DEFCONFIG
    #make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE LOADADDR=0x60003000 uImage -j4
    make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE -j4

    finish_build
}

function build_kernel(){ 
    check_config KERNEL_DEFCONFIG || return 0

    echo "============Start building kernel============"
    echo "TARGET_ARCH   =$TE_ARCH"   
    echo "CROSS_COMPILE =$TE_CROSS_COMPILE"
    echo "TARGET_KERNEL_CONFIG =$KERNEL_DEFCONFIG"
	echo "TARGET_KERNEL_DTS    =$KERNEL_DTS"
    echo "========================================="

    cd kernel
    
    make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE -j$TE_JOBS

    finish_build
}

function build_uboot(){ 
    echo "==========Start building uboot=========="
    echo "TARGET_UBOOT_CONFIG=$UBOOT_DEFCONFIG"
    echo "========================================="

    cd u-boot
    rm -f *_loader_*.bin

    if [ -f "configs/${RK_UBOOT_DEFCONFIG}_defconfig" ]; then
        #make ${UBOOT_DEFCONFIG}_defconfig
        make UBOOT_DEFCONFIG
    fi

    if [ -n "$TE_CROSS_COMPILE" ];then
        make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE all
    fi

    finish_build
}

function build_buildroot(){ 
    echo "==========Start building buildroot=========="
    echo "TARGET_BUILDROOT_CONFIG=$BUILDROOT_DEFCONFIG"
    echo "========================================="

    cd buildroot
    
    make $BUILDROOT_DEFCONFIG
    /usr/bin/time -f "you take %E to build" make -j$TE_JOBS
}

function build_all(){
    echo "============================================"
	echo "TARGET_ARCH=$TE_ARCH"
    echo "CROSS_COMPILE=$TE_CROSS_COMPILE"
	#echo "TARGET_PLATFORM=$RK_TARGET_PRODUCT"
	echo "TARGET_UBOOT_CONFIG=$UBOOT_DEFCONFIG"
	#echo "TARGET_SPL_CONFIG=$RK_SPL_DEFCONFIG"
	echo "TARGET_KERNEL_CONFIG=$KERNEL_DEFCONFIG"
	#echo "TARGET_KERNEL_DTS=$RK_KERNEL_DTS"
	#echo "TARGET_TOOLCHAIN_CONFIG=$RK_CFG_TOOLCHAIN"
	echo "TARGET_BUILDROOT_CONFIG=$BUILDROOT_DEFCONFIG"
	#echo "TARGET_RECOVERY_CONFIG=$RK_CFG_RECOVERY"
	#echo "TARGET_PCBA_CONFIG=$RK_CFG_PCBA"
	#echo "TARGET_RAMBOOT_CONFIG=$RK_CFG_RAMBOOT"
	echo "============================================"

    build_uboot
    build_kernel
    build_buildroot
    finish_build
}

function start_qemu(){
    echo "star qemu ..."

    #https://blog.csdn.net/duapple/article/details/128509624
    #-append "console=ttyAMA0 kmemleak=on loglevel=8" \
    #-dtb  ${CURRENT_DIR}/kernel/arch/arm/boot/dts/vexpress-v2p-ca9.dtb \
    #-kernel ${CURRENT_DIR}/u-boot/u-boot \
    exec qemu-system-aarch64 -M virt \
    -cpu cortex-a53 \
    -machine type=virt\
    -nographic \
    -smp 2 -m 512 \
    -kernel ${CURRENT_DIR}/kernel/arch/arm64/boot/Image \
    -append "noinitrd root=/dev/vda rw console=ttyAMA0,115200 loglevel=8" \
    -netdev user,id=eth0 \
    -device virtio-net-device,netdev=eth0 \
    -drive file=${CURRENT_DIR}/buildroot/output/images/rootfs.ext4,if=none,format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0  ${EXTRA_ARGS} "$@" \
    #-device i2c-host,bus=sysbus.0,addr=0x50 \
    #-device i2c-eeprom,bus=i2c-bus.0,size=256 
}

# 将所有参数存储到数组中 默认为all
OPTIONS="${@:-all}"
config

for option in "${OPTIONS[@]}"; do
    echo "processing option: $option"
    # 在这里可以对每个参数执行你想要的命令或操作
    case $option in
        all) build_all ;;
        kernel) build_kernel ;;
        buildroot) build_buildroot ;;
        uboot) build_uboot ;;
        start)  start_qemu ;;
        *)      echo "Unknown option: $option" ;;
    esac
done

