#!/bin/sh
set -e
###############################################################################
#        安装过后使用windows远程桌面直接连接 ip                               #
#                                                                             #
#                                                                             #
###############################################################################
if [ "$(id -u)" -eq 0 ]; then
	sudo apt update
	sudo apt-get install xrdp
	echo "lxsession -e LXDE -s Lubuntu" > /root/.xsession
	systemctl restart xrdpi
else
	echo "permission denied "
fi
