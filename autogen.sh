#!/bin/bash

aclocal
autoconf
automake -a
./configure CFLAGS="-g -O0"
make
