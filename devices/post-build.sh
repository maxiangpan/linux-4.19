#!/bin/bash
set -e

CURRENT_DIR=$(pwd)
if [ -n "$1" ]; then
    passwd=$1  #传递第一个为passwd
else
    PARAM='123'
fi
if [ -n "$1" ]; then
    TARGET_DIR=$2   #传递第二个信号为需要修改的文件
else
    TARGET_DIR='./p2'
fi

cleanup() {
    echo "done"
}
trap cleanup EXIT

text_profile='
    export PS1='\''\u@\h:\w\$ '\''
    export PS1
'
text_init='
    umount /sys/kernel/debug/
    mount -t debugfs debugfs /sys/kernel/debug/
'

function set_profile(){
    sudo touch $TARGET_DIR/etc/profile.d/common.sh
    sudo chmod 777 $TARGET_DIR/etc/profile.d/common.sh
    sudo echo "$text_profile" >> $TARGET_DIR/etc/profile.d/common.sh
}

function set_mount_debuggs(){
    sudo touch $TARGET_DIR/etc/init.d/common.sh
    sudo chmod 777 $TARGET_DIR/etc/init.d/common.sh
    sudo echo "$text_init" >> $TARGET_DIR/etc/init.d/common.sh

}

echo $passwd | sudo -S ls
set_profile
set_mount_debuggs