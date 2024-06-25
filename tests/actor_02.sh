#!/bin/bash
#SBATCH --job-name=actor-1
#SBATCH -A gts-vsarkar9-coda20
#SBATCH -N32
#SBATCH --ntasks-per-node=24
#SBATCH -t90
#SBATCH -qinferno        
#SBATCH -ojob.out

echo "Started on `/bin/hostname`"   # prints name of compute node job was started on
cd $SLURM_SUBMIT_DIR 

a_datafiles=(soc-pokec-relationships soc-LiveJournal1)
b_datafiles=(com-orkut.ungraph)
pe=(8 16 32)

export DATASET_PATH=~/scratch/imm-dataset

for df in ${a_datafiles[@]}
do
    for pe in ${pe[@]}
    do
        srun -n $(($pe*24)) ../src/imm_hclib_1D -f $DATASET_PATH/$df-LT.txt -d LT -k 100 -e 0.13 -w -o 1d-$df-$pe.txt &> inf-1d-$df-$pe.txt
        srun -n $(($pe*24)) ../src/imm_hclib_2D -f $DATASET_PATH/$df-LT.txt -d LT -k 100 -e 0.13 -w -o 2d-$df-$pe.txt &> inf-2d-$df-$pe.txt
    done
done

for df in ${b_datafiles[@]}
do
    for pe in ${pe[@]}
    do
        srun -n $(($pe*24)) ../src/imm_hclib_1D -f $DATASET_PATH/$df-LT.txt -d LT -k 100 -e 0.13 -w -u -o 1d-$df-$pe.txt &> inf-1d-$df-$pe.txt
        srun -n $(($pe*24)) ../src/imm_hclib_2D -f $DATASET_PATH/$df-LT.txt -d LT -k 100 -e 0.13 -w -u -o 2d-$df-$pe.txt &> inf-2d-$df-$pe.txt
    done
done