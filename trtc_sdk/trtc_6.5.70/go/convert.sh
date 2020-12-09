#!/bin/bash
#
mkdir src
swig -c++ -go -intgosize 64 -gccgo -outdir src -I recordsdk.i
