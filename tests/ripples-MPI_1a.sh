#!/bin/bash
#SBATCH --job-name=ripples-MPI-1
#SBATCH -A gts-vsarkar9-forza
#SBATCH -N32
#SBATCH -c24
#SBATCH -t60
#SBATCH -qinferno        
#SBATCH -ojob.out

export OMP_NUM_THREADS=1

echo "Started on `/bin/hostname`"   # prints name of compute node job was started on
cd $SLURM_SUBMIT_DIR 

a_datafiles=(cit-HepPh soc-Epinions1)
pes=(2 4 8 16 32)

export DATASET_PATH=~/scratch/imm_dataset

if [ ! -d bin ]; then
    mkdir bin
fi

for df in ${a_datafiles[@]}
do
    for pe in ${pes[@]}
    do
        srun -n $(($pe*24)) -c 1 --cpu-bind none $HOME/ripples/build/Release/tools/mpi-imm -i $DATASET_PATH/$df-LT.txt -w -p -k 100 -d LT -e 0.13 --parallel -o $PWD/bin/inf-MPI-$df-$pe.json
    done
done