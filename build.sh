#!/bin/bash

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-build}
BUILD_TYPE=${BUILD_TYPE:-debug}
INSTALL_DIR=${INSTALL_DIR:-${BUILD_TYPE}-install}
#INSTALL_DIR=/home/jiangkun/workspace/project/imlite/slite
CXX=${CXX:-g++}

ln -sf $BUILD_DIR/$BUILD_TYPE/compile_commands.json
mkdir -p $BUILD_DIR/$BUILD_TYPE \
    && cd $BUILD_DIR/$BUILD_TYPE \
    && cmake \
            -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
            -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
            $SOURCE_DIR \
    && make $*
