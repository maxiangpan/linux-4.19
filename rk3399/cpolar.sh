#!/bin/bash
set -e

sudo apt-get install curl -y
curl -L https://www.cpolar.com/static/downloads/install-release-cpolar.sh | sudo bash
cpolar version
#可能需要修改
cpolar authtoken ZmMxMDJhMzgtZjRmMi00OGU3LWFlNWItOGNlMmFiY2FjZDdj
sudo systemctl enable cpolar
sudo systemctl start cpolar
sudo systemctl status cpolar