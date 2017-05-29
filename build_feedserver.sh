#!/bin/bash

echo "Compilers for feedserver"
echo "CC: `$CC --version`"
echo "CXX: `$CXX --version`"

rm -rf build
mkdir -p build
cd build
cmake ..
make
make test
cd ..
