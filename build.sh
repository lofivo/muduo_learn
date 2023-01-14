#!/bin/bash

set -e

SOURCE_DIR=`pwd`

SRC_BASE=${SOURCE_DIR}/base/src
SRC_NET=${SOURCE_DIR}/net/src

# 如果没有 build 目录 创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

# 如果没有 lib 目录 创建该目录
if [ ! -d `pwd`/lib ]; then
    mkdir `pwd`/lib
fi

# 删除存在 build 目录生成文件并执行 cmake 命令
rm -fr ${SOURCE_DIR}/build/*
cd  ${SOURCE_DIR}/build &&
    cmake .. &&
    make all
