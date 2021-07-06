#!/bin/sh

set -e

TOPDIR=$PWD

if [ ! -d $TOPDIR/speexdsp ]; then
    git clone https://github.com/xiph/speexdsp.git
fi

cd $TOPDIR/speexdsp
git checkout SpeexDSP-1.2rc3
./autogen.sh
./configure --prefix=$TOPDIR/libspeexdsp --enable-sse --disable-examples
make -j8
make install

cd $TOPDIR
