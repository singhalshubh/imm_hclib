#!/bin/bash

echo 'Running the first time setup script'

module load gcc/12.1.0-qgxpzk
module load python
module load cmake
module load mvapich2/2.3.7-733lcv
module load pytorch

export PATH=$PATH:$HOME/.local/bin
eval "$__conda_setup"

pip3 install --user "conan==1.59"

if [ ! -f ~/.conan/settings.yml ]; then
    conan config init
fi
conan profile new default --detect &> /dev/null
conan profile update settings.compiler.libcxx=libstdc++11 default

if grep riscv $HOME/.conan/settings.yml; then
    echo RISCV support already added. Skipping.
else
    echo RISCV support added.
    sed --in-place=.bkp 's/x86/x86, riscv/' $HOME/.conan/settings.yml
fi