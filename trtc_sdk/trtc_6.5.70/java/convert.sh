#!/bin/bash
#make src dir and swig convert
mkdir src
swig -c++ -java -package com.tencent -outdir src -I recordsdk.i
#complile jni libtrtcengine.so make sure export JAVA_HOME env var.
export JAVA_HOME=/usr/java/default
g++ -std=c++11 -fPIC -I$JAVA_HOME/include -I$JAVA_HOME/include/linux -L../lib -lTRTCEngine recordsdk_wrap.cxx -shared -v -o libtrtcenginewarper.so
g++ -std=c++11 -fPIC -I$JAVA_HOME/include -I$JAVA_HOME/include/linux -L../lib -lTRTCEngineIPC recordsdk_wrap.cxx -shared -v -o libtrtcengineipcwarper.so
#copy libtrtcengine.so libTRTCEngine.so to /usr/lib 
#unzip trtc_demo.zip 
