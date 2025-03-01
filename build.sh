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

function build_env(){
	export KERNEL_DTS='te_vxpress.dts'
    export ARCH=$TE_ARCH
	export CROSS_COMPILE=$TE_CROSS_COMPILE #qemu arm64 编译环境
    # export KERNEL_DEFCONFIG=te64_defconfig
    # export BUILDROOT_DEFCONFIG=te64_defconfig
    # export UBOOT_DEFCONFIG=te64_defconfig
    # echo "=============================================="
    # echo "TE_ARCH   =$TE_ARCH"
    # echo "TE_CROSS_COMPILE =$TE_CROSS_COMPILE"
    # echo "KERNEL_DEFCONFIG =$KERNEL_DEFCONFIG"
    # echo "BUILDROOT_DEFCONFIG =$BUILDROOT_DEFCONFIG"
    # echo "UBOOT_DEFCONFIG =$UBOOT_DEFCONFIG"
    # echo "=============================================="

    echo "env export success"
}

function config(){

    KERNEL_DTS='te_vxpress.dts'
    #获取vexpress默认config
    #make CROSS_COMPILE=$cross_compile ARCH=arm vexpress_defconfig

    TE_ARCH=arm64
    if [ "$TE_ARCH" = "arm64" ]; then
        TE_CROSS_COMPILE=aarch64-linux-gnu- #qemu arm64 编译环境
        KERNEL_DEFCONFIG=te64_defconfig
        BUILDROOT_DEFCONFIG=te64_defconfig
        UBOOT_DEFCONFIG=te64_defconfig
    else
        #TE_CROSS_COMPILE=arm-linux-gnueabi-
        #TE_CROSS_COMPILE=${CURRENT_DIR}/buildroot/output/host/bin/arm-buildroot-linux-gnueabi-
        TE_CROSS_COMPILE=$CURRENT_DIR/tools/arm/bin/arm-linux-gnueabi-
        KERNEL_DEFCONFIG=te_defconfig
        BUILDROOT_DEFCONFIG=te_defconfig
        UBOOT_DEFCONFIG=te_defconfig
        #arm编译环境
        #如果内核使用EABI接口那么buildroot也应当使用EABI
    fi

    build_env
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
    echo "=============================================="

    cd kernel

    if [ "$TE_ARCH" = "arm64" ]; then
        cp .config ./arch/arm64/configs/te64_defconfig
    else
        cp .config ./arch/arm/configs/te_defconfig
    fi
    
    make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE $KERNEL_DEFCONFIG
    make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE dtbs
    #当前的arm64体系架构已经不支持zImage和uImage的编译目标
    #可使用mkimage工具给不经压缩的Image镜像加上uboot头部信息
    #生成uImage启动镜像，由u-boot来启动。
    if [ "$TE_ARCH" = "arm64" ]; then
        make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE LOADADDR=0x60008000 -j$TE_JOBS
    else
        make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE LOADADDR=0x60008000 zImage -j$TE_JOBS
    fi

    finish_build
}

function build_uboot(){ 
    echo "==========Start building uboot==========="
    echo "TARGET_UBOOT_CONFIG=$UBOOT_DEFCONFIG"
    echo "========================================="

    cd $CURRENT_DIR/u-boot
    rm -f *_loader_*.bin

    if [ -f "configs/${UBOOT_DEFCONFIG}" ]; then
        #make ${UBOOT_DEFCONFIG}_defconfig
        make $UBOOT_DEFCONFIG
    fi

    echo "ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE"

    if [ "$TE_ARCH" = "arm" ]; then
        if [ -n "$TE_CROSS_COMPILE" ];then
            #qemu可以使用arm-linux-gnueabihf- 不知道何时使用arm64
            make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE all
            #make CROSS_COMPILE=arm-linux-gnueabihf- all
        fi
    fi

    finish_build
}

function build_buildroot(){ 
    echo "==========Start building buildroot=========="
    echo "TARGET_BUILDROOT_CONFIG=$BUILDROOT_DEFCONFIG"
    echo "CROSS_COMPILE =$TE_CROSS_COMPILE"
    echo "============================================"
    #配置busybox
    #sudo make busybox-menuconfig
    #make busybox-update-config

    # busybox的改动在output/build/busybox-1.36.1
    # git diff > buildroot/package/busybox/busybox-init.patch

    cd buildroot

    source save_config.sh
    
    make busybox-rebuild ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE

    make $BUILDROOT_DEFCONFIG
    /usr/bin/time -f "you take %E to build" make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE -j$TE_JOBS

    if [ "$TE_ARCH" = "arm" ]; then
        build_img
    fi
    
    finish_build
}

function build_img(){
    echo "==========Start building img============="
    echo "========================================="

    cd $CURRENT_DIR/devices
    ./mkimage.sh $TE_ARCH
}

function build_qemu(){
    SHELL_FOLDER=$(cd "$(dirname "$0")";pwd)
    cd qemu
    if [ ! -d "$SHELL_FOLDER/qemu/output" ]; then
        if [ "$TE_ARCH" = "arm64" ]; then
            ./configure --prefix=$SHELL_FOLDER/qemu/output  --target-list=aarch64-softmmu --enable-gtk  --enable-virtfs --disable-gio
        else
            ./configure --prefix=$SHELL_FOLDER/qemu/output  --target-list=arm-softmmu --enable-gtk  --enable-virtfs --disable-gio
        fi
    fi  
    make -j16
    make install
    cd ..

}

function clean_all(){
    echo "clean all ..."
    cd $CURRENT_DIR/devices
    rm -rf cd cd $CURRENT_DIR/devices/sd.img
    
    cd $CURRENT_DIR/kernel
    make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE clean

    cd $CURRENT_DIR/u-boot
    make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE clean

    cd $CURRENT_DIR/buildroot
    make ARCH=$TE_ARCH CROSS_COMPILE=$TE_CROSS_COMPILE clean
}

function build_all(){
    echo "========================================="
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
	echo "========================================="

    build_uboot    
    build_kernel
    build_buildroot
    build_qemu

    finish_build
}

function start_qemu(){
    echo "star qemu ..."

    # 没有用户名和主机名 export PS1='[\u@\h \W]\$'
    #https://blog.csdn.net/duapple/article/details/128509624
    #共享文件 https://blog.csdn.net/sinat_38201303/article/details/108062939
    #uboot引导内核启动 https://zhuanlan.zhihu.com/p/676252968
    #https://blog.csdn.net/weixin_40837318/article/details/134180125
    #-append "console=ttyAMA0 kmemleak=on loglevel=8" \
    #-dtb  ${CURRENT_DIR}/kernel/arch/arm/boot/dts/vexpress-v2p-ca9.dtb \
    #-kernel ${CURRENT_DIR}/u-boot/u-boot \
    if [ -f "${CURRENT_DIR}/devices/virt/eeprom" ]; then
        echo "rom exist"
    else
        echo "rom not exist will create rom 1k ... "
        pushd ${CURRENT_DIR}/devices/virt
        dd if=/dev/zero of=./eeprom bs=1k count=1
        popd
    fi
    if [ "$TE_ARCH" = "arm64" ]; then
    #--fsdev local,id=kmod_dev,path=$PWD/kmodules,security_model=none`
    #创建一个本地文件系统设备，其中`id`指定设备ID，`path`指定设备挂载的本地路径，`security_model`指定安全模型。
    
    #-device virtio-9p-device,fsdev=kmod_dev,mount_tag=kmod_mount`
    #将本地文件系统设备挂载到虚拟机中，其中`fsdev`指定设备ID，`mount_tag`指定设备挂载的标签。
    #./qemu/output/bin/qemu-system-aarch64 -device help | grep i2c
    #https://quard-star-tutorial.readthedocs.io/zh-cn/latest/ch16.html
        exec $CURRENT_DIR/qemu/output/bin/qemu-system-aarch64 -M virt \
        -smp 2 -m 4G -nographic \
        -cpu cortex-a53 \
        -machine type=virt \
        -dtb ${CURRENT_DIR}/kernel/arch/arm64/boot/dts/te/qemu-virt.dtb \
        -kernel ${CURRENT_DIR}/kernel/arch/arm64/boot/Image \
        -append "noinitrd root=/dev/vda rw console=ttyAMA0,115200 loglevel=8" \
        -device virtio-gpu-device,id=video0,xres=1280,yres=720 \
        -device at24c-eeprom,rom-size=1024,id=eeprom0,address=0x50,address-size=1 \
        -drive file=${CURRENT_DIR}/devices/virt/eeprom,format=raw,id=eeprom0,if=none \
        -drive file=${CURRENT_DIR}/buildroot/output/images/rootfs.ext4,if=none,format=raw,id=hd0 -device virtio-blk-device,drive=hd0  ${EXTRA_ARGS} "$@" \
        -D /tmp/qemu-debug-log \
        -monitor telnet:127.0.0.1:4444,server,nowait

        #-monitor telnet:127.0.0.1:4444,server,nowait 查看qemu log
        #-device at24c-eeprom,rom-size=32768,id=eeprom0,address=0x50 \
        #https://blog.csdn.net/yanghuajia/article/details/143603917
        #https://blog.csdn.net/weixin_30300523/article/details/98040130?spm=1001.2101.3001.6650.3&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogOpenSearchComplete%7ERate-3-98040130-blog-143603917.235%5Ev43%5Epc_blog_bottom_relevance_base6&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogOpenSearchComplete%7ERate-3-98040130-blog-143603917.235%5Ev43%5Epc_blog_bottom_relevance_base6&utm_relevant_index=4
        #dd if=/dev/zero of=./rom bs=1M count=1024
        #-bios ${CURRENT_DIR}/u-boot/u-boot.bin \
        #qemu virt没有SD卡设备
        #-netdev user,id=eth0 -device virtio-net-device,netdev=eth0 -drive file=rootfs.ext4,if=none,format=raw,id=hd0 -device virtio-blk-device,drive=hd0  ${EXTRA_ARGS} "$@"
        #-device virtio-gpu-device,id=video0,xres=1280,yres=720 \
        #-device at24c-eeprom,bus=i2c0,address=0x50,rom-size=1024 \
    fi

    # arm编译环境
    if [ "$TE_ARCH" = "arm" ]; then
        #使用telnet 127.0.0.1 4444 进入qemu monitor
        exec $CURRENT_DIR/qemu/output/bin/qemu-system-arm -M vexpress-a9 \
        -smp 2 -m 1024 \
        -nographic \
        -kernel ${CURRENT_DIR}/u-boot/u-boot \
        -sd ${CURRENT_DIR}/devices/sd.img  \
        -drive file=${CURRENT_DIR}/buildroot/output/images/rootfs.ext4,if=none,format=raw,id=hd0 \
        -device virtio-blk-device,drive=hd0  ${EXTRA_ARGS} "$@" \
        -append "console=ttyAMA0,115200 root=/dev/mmcblk0p2 rw rootwait" \
        -device at24c-eeprom,id=i2c-bus,address=0x50,rom-size=1024 \
        -netdev user,id=eth0 \
        -device virtio-net-device,netdev=eth0 \
        # -display sdl
        #-monitor telnet:127.0.0.1:4444,server,nowait

        #下面才是测试的
        #-s -S
        #-device at24c-eeprom,rom-size=1024,address=0x50 \
        #-netdev user,id=eth0 \
        #-bios ${CURRENT_DIR}/u-boot/u-boot.bin \
        #-device i2c-bus \
        #-device i2c-host,bus=sysbus.0,addr=0x50 \
        #-device i2c-eeprom,bus=i2c-bus.0,size=256 
        #-kernel ${CURRENT_DIR}/kernel/arch/arm64/boot/Image \
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
        cleanall) clean_all ;;
        kernel) build_kernel ;;
        buildroot) build_buildroot ;;
        uboot) build_uboot ;;
        mkimg) build_img ;;
        qemu)  build_qemu ;;
        start)  start_qemu ;;
        *)      echo "Unknown option: $option" ;;
    esac
done