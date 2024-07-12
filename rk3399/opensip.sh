#!/bin/sh
set -e

sudo apt-get install flex bison libncurses5-dev mysql-server mysql-client libmysqlclient-dev -y
sudo apt-get -y install build-essential nghttp2 libnghttp2-dev libssl-dev
#指定目录
cd path/to
git clone https://github.com/OpenSIPS/opensips.git -b2.2 opensips-2.2
