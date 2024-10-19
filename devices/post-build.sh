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

function set_profile(){
    sudo touch $TARGET_DIR/etc/profile.d/common.sh
    sudo chmod 777 $TARGET_DIR/etc/profile.d/common.sh
    sudo echo "export PS1='\u@\h:\w\$ '" >> $TARGET_DIR/etc/profile.d/common.sh
    sudo echo "export PS1" >> $TARGET_DIR/etc/profile.d/common.sh
}

echo $passwd | sudo -S ls
set_profile