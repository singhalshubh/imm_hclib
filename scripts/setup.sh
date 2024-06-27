#!/bin/bash
module load gcc python openmpi/4.1.4 cmake
export CC=oshcc
export CXX=oshc++
export LOC=$PWD

# bale
if [ ! -d bale ]; then
    git clone https://github.com/jdevinney/bale.git
    cd bale/src/bale_classic/
    ./bootstrap.sh
    PLATFORM=oshmem ./make_bale -s -f
    cd ../../../
fi

if [ ! -d hclib ]; then
    git clone https://github.com/srirajpaul/hclib
    cd hclib
    git fetch && git checkout bale3_actor
    ./install.sh
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
    CXX=oshc++ cmake -DTRNG_ENABLE_TESTS=Off -DTRNG_ENABLE_EXAMPLES=Off -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=$LOC/trng4/ ..
    cmake --build .
    cmake --build . --target install
    cd ../../
fi

export BALE_INSTALL=$PWD/bale/src/bale_classic/build_oshmem
export HCLIB_ROOT=$PWD/hclib/hclib-install
export TRNG_ROOT=$PWD/trng4
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BALE_INSTALL/lib:$HCLIB_ROOT/lib:$HCLIB_ROOT/../modules/bale_actor/lib:$TRNG_ROOT/lib64
export HCLIB_WORKERS=1
