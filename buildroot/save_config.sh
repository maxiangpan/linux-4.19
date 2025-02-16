#!/bin/bash

#cp .config ./configs/te_defconfig
#cp .config ./configs/te64_defconfig

if [ "$ARCH" = "arm64" ]; then
    cp .config ./configs/te64_defconfig
else
    cp .config ./configs/te_defconfig
fi