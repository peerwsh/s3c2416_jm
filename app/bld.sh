#!/bin/sh
rm test
gcc -O2 test.cpp -o test -L ./ -ljm
strip test
