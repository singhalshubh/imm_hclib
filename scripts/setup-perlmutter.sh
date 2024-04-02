#!/bin/bash

# Setup HClib and its modules
echo "-----------------------------------------------------"
echo "Setting up IMM pre-req"
echo "-----------------------------------------------------"

# Load modules
module load cray-openshmemx
module load cray-pmi
module load cmake

# Export environment variables
export PLATFORM=ex
export CC=cc
export CXX=CC
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/cray/pe/perftools/23.02.0/lib64
export LOC=$PWD
# Setup Bale
if [ ! -d bale ]; then
    git clone https://github.com/jdevinney/bale.git bale
    cd bale
    wget https://raw.githubusercontent.com/ahayashi/hclib-actor/master/cluster-scripts/perlmutter.patch
    patch -p1 < perlmutter.patch
    cd ..
    cd bale/src/bale_classic
    export BALE_INSTALL=$PWD/build_${PLATFORM}
    ./bootstrap.sh
    python3 ./make_bale -s
    cd ../../../
fi

# Setup HCLib
if [ ! -d hclib ]; then
    git clone https://github.com/srirajpaul/hclib
    cd hclib
    git fetch && git checkout bale3_actor
    ./install.sh --enable-production
    source hclib-install/bin/hclib_setup_env.sh
    cd modules/bale_actor && make
    cd benchmarks
    unzip ../inc/boost.zip -d ../inc/
    cd ../../../../
fi

if [ ! -d trng4 ]; then
    git clone https://github.com/rabauke/trng4
    cd trng4/
    mkdir build && cd build
    CXX=CC cmake -DTRNG_ENABLE_TESTS=Off -DTRNG_ENABLE_EXAMPLES=Off -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=$LOC/trng4/ ..
    cmake --build .
    cmake --build . --target install
    cd ../../
fi

export BALE_INSTALL=$PWD/bale/src/bale_classic/build_${PLATFORM}
export HCLIB_ROOT=$PWD/hclib/hclib-install
export TRNG_ROOT=$PWD/trng4
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BALE_INSTALL/lib:$HCLIB_ROOT/lib:$HCLIB_ROOT/../modules/bale_actor/lib:$TRNG_ROOT/lib64
export HCLIB_WORKERS=1

# All setups are complete
echo "-----------------------------------------------------"
echo "               IMM setup complete!                 "
echo "-----------------------------------------------------"