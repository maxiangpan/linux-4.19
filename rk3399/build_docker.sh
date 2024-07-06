#!/bin/sh
set -e
# 清华网站 https://mirror.tuna.tsinghua.edu.cn/help/docker-ce/
# firefly  https://wiki.t-firefly.com/zh_CN/Firefly-Linux-Guide/first_use.html#docker-zhi-chi
# 不如直接更换阿里云 https://cr.console.aliyun.com/cn-hangzhou/instances/mirrors


if [ "$(id -u)" -eq 0 ]; then
	sudo apt update
	wget -O- https://get.docker.com/ | sh
else
	echo "permission denied "
fi
