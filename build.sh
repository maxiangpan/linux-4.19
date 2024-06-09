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

function config(){
    KERNEL_DEFCONFIG=te_defconfig
    BUILDROOT_DEFCONFIG=te_defconfig
    TE_ARCH=arm64
    TE_CROSS_COMPILE=${CURRENT_DIR}/buildroot/output/host/bin/aarch64-buildroot-linux-gnu-
}

function build_kernel(){ 
    make CROSS_COMPILE=$TE_CROSS_COMPILE -j$TE_JOBS
}

function build_kernel(){ 
    check_config KERNEL_DEFCONFIG || return 0

    echo "============Start building kernel============"
    echo "TARGET_ARCH   =$TE_ARCH"   
    echo "CROSS_COMPILE =$TE_CROSS_COMPILE"
    echo "TARGET_KERNEL_CONFIG =$KERNEL_DEFCONFIG"
	echo "TARGET_KERNEL_DTS    =$RK_KERNEL_DTS"
    echo "========================================="

    cd kernel
    
    make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE -j$TE_JOBS
}

function build_buildroot(){ 
    echo "==========Start building buildroot=========="
    echo "TARGET_BUILDROOT_CONFIG=$BUILDROOT_DEFCONFIG"
    echo "========================================="

    cd buildroot
    
    make $BUILDROOT_DEFCONFIG
    /usr/bin/time -f "you take %E to build" make -j$TE_JOBS
}

function start_qemu(){
    echo "star qemu ..."

    #https://blog.csdn.net/duapple/article/details/128509624
    #-append "console=ttyAMA0 kmemleak=on loglevel=8" \
    

    exec qemu-system-aarch64 -M \
    virt -cpu cortex-a53 -machine type=virt\
    -nographic \
    -smp 2 -m 512 \
    -kernel ${CURRENT_DIR}/kernel/arch/arm64/boot/Image \
    -append "noinitrd root=/dev/vda rw console=ttyAMA0,115200 loglevel=8" \
    -netdev user,id=eth0 \
    -device virtio-net-device,netdev=eth0 \
    -drive file=${CURRENT_DIR}/buildroot/output/images/rootfs.ext4,if=none,format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0  ${EXTRA_ARGS} "$@" 
}

# 将所有参数存储到数组中
OPTIONS=("$@")
config

for option in "${OPTIONS[@]}"; do
    echo "processing option: $option"
    # 在这里可以对每个参数执行你想要的命令或操作
    case $option in
        kernel) build_kernel ;;
        buildroot) build_buildroot ;;
        start)  start_qemu;;
        *)      echo "Unknown option: $option" ;;
    esac
done

