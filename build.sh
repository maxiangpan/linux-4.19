#!/bin/bash

# sudo mount -t ext3 a9rootfs.ext3 tmpfs/ -o loop
# sudo cp -r rootfs/*  tmpfs/
# sudo umount tmpfs

#make ARCH=arm64 CROSS_COMPILE=/home/mxp/Desktop/linux/buildroot-2024.02.2/output/host/bin/aarch64-buildroot-linux-gnu- -j4
CURRENT_DIR=$(pwd)

NPROC=`nproc`
export TE_JOBS=$NPROC
TE_ARCH=arm64
TE_CROSS_COMPILE=${CURRENT_DIR}/buildroot-2024.02.2/output/host/bin/aarch64-buildroot-linux-gnu-

function build_kernel(){
    echo "============Start building kernel============"
    echo "TARGET_ARCH   =$TE_ARCH"   
    echo "CROSS_COMPILE =$TE_CROSS_COMPILE"
    echo "TE_JOBS       =$TE_JOBS"
    echo "========================================="

    cd linux-4.19
    make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE -j$TE_JOBS
}

function start(){
    echo "star qemu ..."

    cd linux-4.19
    make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE -j$TE_JOBS
}

# 将所有参数存储到数组中
OPTIONS=("$@")

for option in "${OPTIONS[@]}"; do
    echo "processing option: $option"
    # 在这里可以对每个参数执行你想要的命令或操作
    case $option in
        kernel) build_kernel ;;
        start)  echo $CURRENT_DIR;;
    esac
done

