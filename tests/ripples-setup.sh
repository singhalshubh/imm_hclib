#!/bin/bash

echo 'Running the first time setup script'

pip install --user "conan==1.59"

if [ ! -f ~/.conan/settings.yml ]; then
    conan config init
fi
conan profile new default --detect &> /dev/null
conan profile update settings.compiler.libcxx=libstdc++11 default
# For unknown reasons conan detects the wrong version from
# the Cray Compiler wrapper version string.
conan profile update settings.compiler.version=11.2 default
# Cray Machine have compiler wrappers providing MPI
conan profile update env.CC=cc default
conan profile update env.CXX=CC default

if grep riscv $HOME/.conan/settings.yml; then
    echo RISCV support already added. Skipping.
else
    echo RISCV support added.
    sed --in-place=.bkp 's/x86/x86, riscv/' $HOME/.conan/settings.yml
fi


for i in $(ls conan/); do
    if [ -d $HOME/.conan/data/$i ]; then
        rm -rf $HOME/.conan/data/$i
    fi

    conan create conan/$i user/stable
done
