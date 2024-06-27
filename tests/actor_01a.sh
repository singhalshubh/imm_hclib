#!/bin/bash
#SBATCH --job-name=actor-1
#SBATCH -A gts-vsarkar9-forza
#SBATCH -N32
#SBATCH --ntasks-per-node=24
#SBATCH -t60
#SBATCH -qinferno        
#SBATCH -ojob.out

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
        srun -n $(($pe*24)) ../src/imm_hclib_1D -f $DATASET_PATH/$df-LT.txt -d LT -k 100 -e 0.13 -w -o $PWD/bin/1d-$df-$pe.txt -t $PWD/bin/inf-1d-$df-$pe.txt
        srun -n $(($pe*24)) ../src/imm_hclib_2D -f $DATASET_PATH/$df-LT.txt -d LT -k 100 -e 0.13 -w -o $PWD/bin/2d-$df-$pe.txt -t $PWD/bin/inf-2d-$df-$pe.txt
    done
done