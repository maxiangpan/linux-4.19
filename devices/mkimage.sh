#!/bin/bash
set -e
passwd='123'

CURRENT_DIR=$(pwd)
# 定义退出信号处理函数
cleanup() {
    # 搜索挂载的文件系统
    cd $CURRENT_DIR
    mount | grep "devices/p1" | awk '{print $3}' | while read -r mountpoint; do
        echo "Unmounted $mountpoint"
        sudo umount "$mountpoint"
    done
    cd $CURRENT_DIR
    mount | grep "devices/p2" | awk '{print $3}' | while read -r mountpoint; do
        echo "Unmounted $mountpoint"
        sudo umount "$mountpoint"
    done
    cd $CURRENT_DIR
    mount | grep "rootfs" | awk '{print $3}' | while read -r mountpoint; do
        echo "Unmounted $mountpoint"
        sudo umount "$mountpoint"
    done

    loop_devices=$(losetup -a | grep "sd.img" | awk -F: '{print $1}')
    for loop_device in $loop_devices; do
        sudo losetup -d $loop_device
    done

    # if [ -d "$CURRENT_DIR/p1" ]; then
    #     if mount | grep -q "$CURRENT_DIR/p1"; then
    #         sudo umount p1
    #         echo "umount p1"
    #     fi
    # fi
    # if [ -d "$CURRENT_DIR/p2" ]; then
    #     if mount | grep -q "$CURRENT_DIR/p2"; then
    #         sudo umount p2
    #         echo "umount p2"
    #     fi
    # fi
    # if [ -d "$CURRENT_DIR/rootfs" ]; then
    #     if mount | grep -q "$CURRENT_DIR/rootfs"; then
    #         sudo umount rootfs
    #         echo "umount rootfs"
    #     fi
    # fi
    # if [ -n "$loop_dev" ]; then
    #     sudo losetup -d $loop_dev
    # fi
    sudo rm -rf p1 p2 rootfs
    sudo rm uImage
}
# 捕捉退出信号
trap cleanup EXIT
te_arch=$1

echo $passwd | sudo -S ls

kernel_path=./uImage
# dtb_path=~/linux-4.19/kernel/arch/arm/boot/dts/te/te_vxpress.dtb
# 必须要dtb引导内核加载，否则内核找不到设备树，无法启动
# 如果设备树配置错误依然无法引导内核加载
if [ "$te_arch" = "arm" ]; then
    dtb_path=$CURRENT_DIR/../kernel/arch/$te_arch/boot/dts/vexpress-v2*.dtb
fi
if [ "$te_arch" = "arm64" ]; then
    dtb_path=$CURRENT_DIR/../kernel/arch/$te_arch/boot/dts/vexpress-v2*.dtb
fi
rootfs_path=${CURRENT_DIR}/../buildroot/output/images/rootfs.ext4

sec_img=sd.img

############### Create uImage
cd ../u-boot/tools

if [ ! -f sd.img ]; then
    mkimage -n "virt_linux" -A arm -a 0x60008000 -e 0x60008000 \
    -d ../../kernel/arch/$te_arch/boot/Image ../../devices/uImage
fi

cd $CURRENT_DIR
############### Create img
# 这种只能arm架构不能使用arm64架构，还得继续看 https://blog.51cto.com/u_15127650/3467228
loop_dev=$(losetup -f)
echo "use $loop_dev"

if [ ! -f sd.img ]; then
    dd if=/dev/zero of=$sec_img bs=1024 count=524288
    #创建GPT分区，下面创建了两个分区，一个用来存放kernel和设备树，另一个存放根文件系统
    #sgdisk -n 0:0:+20M -c 0:kernel $sec_img
    #sgdisk -n 0:0:0 -c 0:rootfs $sec_img
    sgdisk -n 1:2048:43007 -c 1:"kernel" $sec_img   # kernel 分区
    sgdisk -n 2:43008:0 -c 2:"rootfs" $sec_img   # rootfs 分区
fi

sudo losetup $loop_dev $sec_img
sudo partprobe $loop_dev

#格式化
#sudo mkfs.ext4 ${loop_dev}p1
#sudo mkfs.ext4 ${loop_dev}p2
sudo mkfs.ext4 -b 4096 -F -L kernel ${loop_dev}p1
sudo mkfs.ext4 -b 4096 -F -L rootfs ${loop_dev}p2

sudo mkdir p1 p2 rootfs

sudo mount -t ext4 ${loop_dev}p1 p1/
sudo mount -t ext4 ${loop_dev}p2 p2/
sudo mount -t ext4 $rootfs_path rootfs/

if [ "$te_arch" = "arm64" ]; then
    sudo cp -a $CURRENT_DIR/../kernel/arch/$te_arch/boot/Image p1/
fi
if [ "$te_arch" = "arm" ]; then
    sudo cp -a $CURRENT_DIR/../kernel/arch/$te_arch/boot/zImage p1/
fi
sudo cp -a $dtb_path p1/
if [ -s rootfs ]; then
    sudo cp -raf rootfs/* ./p2
else
    echo "rootfs does not exist or is empty."
fi

source post-build.sh passwd p2

#sudo cp -a $CURRENT_DIR/../rootfs/lib/* ./p2/lib

# sudo umount p1 p2 rootfs
# sudo losetup -d $loop_dev
# sudo rm -rf p1 p2 rootfs
# sudo rm uImage
echo "create success!"
