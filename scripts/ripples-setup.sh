
echo 'Running setup script for `ripples`'
module load gcc python openmpi/4.1.4 cmake
pip3 install --user "conan==1.59"

if [ ! -f ~/.conan/settings.yml ]; then
    conan config init
fi
conan profile new default --detect &> /dev/null
export agile_WF=$PWD
if [ ! -d $HOME/ripples ]; then
    git clone https://github.com/pnnl/ripples.git $HOME/ripples
fi
cd $HOME/ripples

conan create conan/trng
conan install --install-folder build . --build 
conan install . --build missing
conan build .

echo $PWD
cd $agile_WF