#!/bin/bash
#PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./waf configure --boost-libs=/usr/lib/x86_64-linux-gnu

#./waf clean
#./waf

PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./waf configure -optimized --boost-libs=/usr/lib/x86_64-linux-gnu

#./waf clean
./waf

