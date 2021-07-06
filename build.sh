#!/bin/sh

set -e

gcc -Wall -static wavdev.c test.c -o test -I$PWD/libspeexdsp/include -L$PWD/libspeexdsp/lib -lspeexdsp -lwinmm
