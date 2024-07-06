#!/bin/sh
set -e

# https://blog.csdn.net/qq_45529538/article/details/132360921

if [ "$(id -u)" -eq 0 ]; then
	sudo dd if=/dev/zero of=/var/swapfile bs=1M count=12288 #12G
	sudo mkswap /var/swapfile
	sudo swapon /var/swapfile
	sudo cp /etc/fstab /etc/fstab_bak
	sudo echo "/var/swapfile swap swap defaults 0 0" >> /etc/fstab
	free -h
else
	echo "permission denied "
fi
