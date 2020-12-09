#!/bin/bash
SHELL_FOLDER=$(dirname $(readlink -f "$0"))

if [ "$#" -ne "1" ]; then
    echo "Usage: ./cmake_build.sh <clean/all/sample>"
    exit 0
elif [ "$#" -eq "1"  -a $1 == "all" ]; then # build clean
    echo "Build all (SDK lib and sample)"
    git submodule init
    git submodule update
    git submodule foreach 'git checkout -b v3.1.5'

    rm -rf output
    rm -rf build
    mkdir -p build
    cd build
    cmake ..
    make -j8
    exit 0
elif [ "$#" -eq "1"  -a $1 == "sample" ]; then # build clean
    echo "Build sample (Only sample)"
    if [ -d "output/bin" ]; then
        rm -rf output/bin
    else
        echo "Output folder not found! Please build SDK first"
        exit -1
    fi
    rm -rf build
    echo "Build sample only"
    mkdir -p build
    cd build
    cmake -DSAMPLE_ONLY=ON ..
    make -j8
    exit 0
elif [ "$#" -eq "1"  -a $1 == "clean" ]; then # build clean
    echo "Clean sdk"
    rm -rf output
    rm -rf build
    exit 0
else
    echo "Usage: ./cmake_build.sh <clean/all/sample>"
    exit -1
fi
