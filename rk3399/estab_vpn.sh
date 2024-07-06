#!/bin/sh
set -e
###############################################################################
#		在windowsws下载安装clash
#		打开局域网连接和端口设置
#		Linux下 sudo vim /etc/bash.bashrc
#		配置网络代理，转发网络请求到主机的 Clash，由 Clash 进行代理
#		export https_proxy=windows_ip:port			
#
#		export https_proxy=http://192.168.1.5:7897	
#		export http_proxy="http://192.168.0.3:7890"
#		export https_proxy="http://192.168.0.3:7890"
#		export all_proxy="socks5://192.168.0.3:7890"
#		export ALL_PROXY="socks5://192.168.0.3:7890"
#
#                                                                             
#                                                                             
###############################################################################
echo "*************************************************"
echo "  Only effective on the current console"
echo "  export https_proxy=windows_ip:port"
echo "  like : export https_proxy=http://192.168.1.5:7897"
echo "*************************************************"
export https_proxy=http://192.168.1.5:7897
export http_proxy=http://192.168.1.5:7897

