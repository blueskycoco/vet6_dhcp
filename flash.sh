#! /bin/bash -e
make clean
make
jflasharm -openprj$1 -opengcc/w5500.bin,0x08000000 -auto -startapp -exit

