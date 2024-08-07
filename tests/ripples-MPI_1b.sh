#!/bin/bash
#SBATCH --job-name=ripples-MPI-1
#SBATCH -A gts-vsarkar9-forza
#SBATCH -N32
#SBATCH -c24
#SBATCH -t360
#SBATCH -qinferno        
#SBATCH -ojob.out

export OMP_NUM_THREADS=1

echo "Started on `/bin/hostname`"   # prints name of compute node job was started on
cd $SLURM_SUBMIT_DIR 

b_datafiles=(com-dblp.ungraph com-youtube.ungraph)
pes=(2 4 8 16 32)

export DATASET_PATH=~/scratch/imm_dataset

for df in ${b_datafiles[@]}
do
    for pe in ${pes[@]}
    do
        srun -n $(($pe*24)) -c 1 --cpu-bind none $HOME/ripples/build/Release/tools/mpi-imm -i $DATASET_PATH/$df-LT.txt -w -u -p -k 100 -d LT -e 0.13 --parallel -o $PWD/bin/inf-MPI-$df-$pe.json
    done
done