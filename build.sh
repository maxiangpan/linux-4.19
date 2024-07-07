#!/bin/bash

#make ARCH=arm64 CROSS_COMPILE=/home/mxp/Desktop/linux/buildroot-2024.02.2/output/host/bin/aarch64-buildroot-linux-gnu- -j4
set -e  # 这将使脚本在任何命令返回非零状态时立即退出
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
	cd $CURRENT_DIR
}

function config(){
    KERNEL_DEFCONFIG=te_defconfig
    BUILDROOT_DEFCONFIG=te_defconfig
    UBOOT_DEFCONFIG=te_defconfig
    KERNEL_DTS=te_vxpress

    TE_ARCH=arm
    if [ "$TE_ARCH" = "arm64" ]; then
        TE_CROSS_COMPILE=aarch64-linux-gnu- #qemu arm64 编译环境
    else
        TE_CROSS_COMPILE=arm-linux-gnueabihf- #arm编译环境
    fi
    #TE_CROSS_COMPILE=${CURRENT_DIR}/buildroot/output/host/bin/aarch64-buildroot-linux-gnu-
}

# function build_kernel(){ 
#     #make CROSS_COMPILE=$TE_CROSS_COMPILE -j$TE_JOBS
#     make $KERNEL_DEFCONFIG
#     #make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE LOADADDR=0x60003000 uImage -j4
#     make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE -j4

#     finish_build
# }

function build_kernel(){ 
    check_config KERNEL_DEFCONFIG || return 0

    echo "============Start building kernel============"
    echo "TARGET_ARCH   =$TE_ARCH"   
    echo "CROSS_COMPILE =$TE_CROSS_COMPILE"
    echo "TARGET_KERNEL_CONFIG =$KERNEL_DEFCONFIG"
	echo "TARGET_KERNEL_DTS    =$KERNEL_DTS"
    echo "========================================="

    cd kernel
    
    make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE $KERNEL_DEFCONFIG
    make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE dtbs
    #当前的arm64体系架构已经不支持zImage和uImage的编译目标
    #可使用mkimage工具给不经压缩的Image镜像加上uboot头部信息
    #生成uImage启动镜像，由u-boot来启动。
    if [ "$TE_ARCH" = "arm64" ]; then
        make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE LOADADDR=0x60008000 -j$TE_JOBS
    else
        make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE LOADADDR=0x60008000 bzImage -j$TE_JOBS
    fi

    finish_build
}

function build_uboot(){ 
    echo "==========Start building uboot=========="
    echo "TARGET_UBOOT_CONFIG=$UBOOT_DEFCONFIG"
    echo "========================================="

    cd u-boot
    rm -f *_loader_*.bin

    if [ -f "configs/${UBOOT_DEFCONFIG}" ]; then
        #make ${UBOOT_DEFCONFIG}_defconfig
        make $UBOOT_DEFCONFIG
    fi

    echo "ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE"
    if [ -n "$TE_CROSS_COMPILE" ];then
        #qemu可以使用arm-linux-gnueabihf- 不知道何时使用arm64
        #make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE all
        make CROSS_COMPILE=arm-linux-gnueabihf- all
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

    finish_build
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

    build_buildroot
    build_uboot    
    build_kernel

    finish_build
}

function start_qemu(){
    echo "star qemu ..."

    # 没有用户名和主机名 export PS1='[\u@\h \W]\$'
    #https://blog.csdn.net/duapple/article/details/128509624
    #共享文件 https://blog.csdn.net/sinat_38201303/article/details/108062939
    #uboot引导内核启动 https://zhuanlan.zhihu.com/p/676252968
    #-append "console=ttyAMA0 kmemleak=on loglevel=8" \
    #-dtb  ${CURRENT_DIR}/kernel/arch/arm/boot/dts/vexpress-v2p-ca9.dtb \
    #-kernel ${CURRENT_DIR}/u-boot/u-boot \
    if [ "$TE_ARCH" = "arm64" ]; then
    #--fsdev local,id=kmod_dev,path=$PWD/kmodules,security_model=none`
    #创建一个本地文件系统设备，其中`id`指定设备ID，`path`指定设备挂载的本地路径，`security_model`指定安全模型。
    
    #-device virtio-9p-device,fsdev=kmod_dev,mount_tag=kmod_mount`
    #将本地文件系统设备挂载到虚拟机中，其中`fsdev`指定设备ID，`mount_tag`指定设备挂载的标签。
        exec qemu-system-aarch64 -M virt \
        -cpu cortex-a57 \
        -machine type=virt \
        -nographic \
        -smp 2 -m 2048 \
        -kernel ${CURRENT_DIR}/u-boot/u-boot \
        -append "noinitrd root=/dev/vda rw console=ttyAMA0,115200 loglevel=8" \
        -sd ${CURRENT_DIR}/devices/sd.img \
        -netdev user,id=eth0 \
        -device virtio-net-device,netdev=eth0 \
        -drive file=${CURRENT_DIR}/buildroot/output/images/rootfs.ext4,if=none,format=raw,id=hd0 \
        -device virtio-blk-device,drive=hd0  ${EXTRA_ARGS} "$@" \
        #-kernel ${CURRENT_DIR}/kernel/arch/arm64/boot/Image \
        #-kernel ${CURRENT_DIR}/u-boot/u-boot \
        #-bios ${CURRENT_DIR}/u-boot/u-boot.bin \
        #-device i2c-bus \
        #-device i2c-host,bus=sysbus.0,addr=0x50 \
        #-device i2c-eeprom,bus=i2c-bus.0,size=256 
    fi

    # arm编译环境
    if [ "$TE_ARCH" = "arm" ]; then
        exec qemu-system-arm -M vexpress-a9 \
        -nographic \
        -m 1024 \
        -kernel ${CURRENT_DIR}/u-boot/u-boot \
        -sd ${CURRENT_DIR}/devices/sd.img 
        #-netdev user,id=eth0 \
        -drive file=${CURRENT_DIR}/buildroot/output/images/rootfs.ext4,if=none,format=raw,id=hd0 \
        -device virtio-blk-device,drive=hd0  ${EXTRA_ARGS} "$@" \
        #-bios ${CURRENT_DIR}/u-boot/u-boot.bin \
        #-device i2c-bus \
        #-device i2c-host,bus=sysbus.0,addr=0x50 \
        #-device i2c-eeprom,bus=i2c-bus.0,size=256 
    fi
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

