#!/bin/bash
sed -i '41s#    "#    // "#1' flop_counter.cpp
sed -i '42s#// ##1' flop_counter.cpp
sed -i '43s#// ##1' flop_counter.cpp
make 
if [ ${?} -eq 0 ]; then
    pin -t ./obj-intel64/flop_counter.so -- ./obj-intel64/matrix_multiplications.exe 
else
    echo "make failed"
fi
