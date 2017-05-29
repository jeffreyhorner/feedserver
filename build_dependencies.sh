#!/bin/bash

echo "Compilers for feedserver dependencies"
echo "CC: `$CC --version`"
echo "CXX: `$CC --version`"

if [ ! -d dependencies ]; then
  mkdir dependencies
fi
cd dependencies

# Get and Build haywire
if [ ! -d "haywire" ]; then
  echo "git clone https://github.com/jeffreyhorner/haywire"
  git clone https://github.com/jeffreyhorner/haywire
  cd haywire
  cp ../../compile_haywire_dependencies.sh compile_dependencies.sh
  ./compile_dependencies.sh
  ./compile_make.sh
  cd ..
fi

# Get json-parser
if [ ! -d "json-parser" ]; then
  echo "git clone https://github.com/udp/json-parser"
  git clone https://github.com/udp/json-parser
  cd json-parser
  git checkout b1b09b5585e9f7c0627c8569ab51e2cd7faf2ec0
  cd ..
fi

# No extern C in headers so we keep in the tarball
#
# Get vc_vector
#if [ ! -d "vc_vector" ]; then
#  echo "git clone https://github.com/skogorev/vc_vector"
#  git clone https://github.com/skogorev/vc_vector
#  cd vc_vector
#  git checkout 66c5a988de8c2e6477747a3ec67cdf7428eb3fb9
#  cd ..
#fi

# Get rocksdb
if [ ! -d "rocksdb" ]; then
  echo "wget https://github.com/facebook/rocksdb/archive/rocksdb-5.4.5.zip"
  wget https://github.com/facebook/rocksdb/archive/rocksdb-5.4.5.zip
  unzip rocksdb-5.4.5.zip
  mv rocksdb-rocksdb-5.4.5 rocksdb
  rm rocksdb-5.4.5.zip
  cd rocksdb
  make static_lib
  cd ..
fi

cd ..

# R libraries
Rscript -e "install.packages('devtools')"
Rscript -e "library(devtools); install_github('hadley/httr')"
Rscript -e "library(devtools); install_github('jeroen/jsonlite')"
echo "Built"
