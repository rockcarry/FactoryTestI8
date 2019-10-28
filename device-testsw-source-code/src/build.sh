#!/bin/sh

set -e

export PATH=$PWD/mips-gcc472-glibc216-32bit/bin:$PATH

mips-linux-gnu-gcc -I$PWD/include -L$PWD/lib appMicSn.c -ldl -lrt -lm -lpthread -limp -lalog -lsysutils -o appMicSn
mips-linux-gnu-strip appMicSn

