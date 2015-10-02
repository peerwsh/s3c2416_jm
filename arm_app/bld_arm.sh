#!/bin/sh
rm -rf libjm.so arm_chk
export ARM_ROOT=/home/peterwsh/work/project/buildroot-2012.05/output/host/usr/
g++ -DMASTER -fPIC -shared crypt.cpp nanocomm.cpp -o libjm.so -lcryptopp -lpthread
arm-none-linux-gnueabi-g++ -I../cryptopp/include -I$ARM_ROOT/include -O2 crypt.cpp nanocomm.cpp -o arm_chk -L ../cryptopp/lib -lcryptopp -L $ARM_ROOT/lib -lpthread
