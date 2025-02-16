#!/bin/bash

CURRENT_DIR=$(pwd)

echo "=========build_before_script========="

pushd output/build/busybox-1.36.1
git diff > $CURRENT_DIR/package/busybox/busybox-init.patch
popd