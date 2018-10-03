#!/bin/bash
#/usr/local/lib:/usr/lib/x86_64-linux-gnu
#export LD_LIBRARY_PATH=/usr/local/lib
export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/usr/local/lib
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
#./configure --with-boost-libdir=/usr/lib/x86_64-linux-gnu
PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./waf configure --boost-libs=/usr/lib/x86_64-linux-gnu --debug

./waf
