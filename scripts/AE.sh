export LOC=$PWD
###### Actor-IMM ####
echo -e 'Cloning IMM-Actor Algorithms'
git clone https://github.com/singhalshubh/imm_hclib
echo -e 'Building IMM-Actor Algorithms'
source imm_hclib/scripts/setup.sh
cd imm_hclib/src/
export HClib_WF=$PWD
make
echo -e 'Ready to run IMM-Actor Algorithms'
#######################
cd $LOC

###### Ripples ####
source ripples-conan.sh
source ripples-setup.sh
############################
cd $LOC

###### Datasets ####
echo -e 'Copying datasets to scratch space'
cd ~/scratch/
mkdir imm_dataset && cd imm_dataset
cp -r /storage/coda1/p-vsarkar9/1/shared/imm_datasets/* ./
export DATASET_PATH=$PWD
###################

##### Run scripts for generating experimental numbers ######
echo -e 'Running jobs'
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