#!/bin/bash
echo 'Running setup script for `ripples`'
module load gcc
module load python
module load cmake
module load openmpi

pip3 install --user "conan==1.59"

if [ ! -f ~/.conan/settings.yml ]; then
    conan config init
fi
conan profile new default --detect &> /dev/null
conan profile update settings.compiler.libcxx=libstdc++11 default
conan profile update settings.compiler.version=12 default
conan profile update env.CC=cc default
conan profile update env.CXX=CC default

export agile_WF=$PWD
if [ ! -d $HOME/ripples ]; then
    git clone https://github.com/pnnl/ripples.git $HOME/ripples
fi

cd $HOME/ripples
conan create conan/waf-generator user/stable
conan create conan/trng
conan install --install-folder build . --build 
conan install . --build missing
conan build .

echo $PWD
cd $agile_WF
