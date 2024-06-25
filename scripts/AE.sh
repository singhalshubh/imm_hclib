export LOC=$PWD

###### Actor-IMM ####
echo 'Cloning IMM-Actor Algorithms'
git clone https://github.com/singhalshubh/imm_hclib
echo 'Building IMM-Actor Algorithms'
source imm_hclib/scripts/setup.sh
cd imm_hclib/src/
export HClib_WF=$PWD
make
echo 'Ready to run IMM-Actor Algorithms'
#######################
cd $LOC

###### Ripples ####
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

if [ ! -d $HOME/ripples ]; then
    git clone https://github.com/pnnl/ripples.git $HOME/ripples
fi

cd $HOME/ripples
conan create conan/trng
conan install --install-folder build . --build 
conan install . --build missing
conan build .
############################
cd $LOC

###### Datasets ####
echo 'Copying datasets to scratch space'
cd ~/scratch/
mkdir imm_dataset && cd imm_dataset
cp -r /storage/coda1/p-vsarkar9/1/shared/imm_datasets/* ./
export DATASET_PATH=$PWD
###################

##### Run scripts for generating experimental numbers ######
cd $LOC/imm_hclib/tests
sbatch ripples-MPI_1.sh
sbatch ripples-MPI_2.sh
sbatch ripples-MPIOP_1.sh
sbatch ripples-MPIOP_2.sh
sbatch actor_01.sh
sbatch actor_02.sh
############################

##########################
python -m pip install -U matplotlib

python interpret.py cit-HepPh 2
python interpret.py soc-Epinions1 2
python interpret.py com-dblp.ungraph 2
python interpret.py com-youtube.ungraph 2
python interpret.py soc-pokec-relationships 8
python interpret.py soc-LiveJournal1 8
python interpret.py com-orkut.ungraph 8