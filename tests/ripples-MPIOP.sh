#!/bin/bash
#SBATCH --job-name=ripples-IMM-mpiop
#SBATCH -q regular
#SBATCH -N 8
#SBATCH -c 128
#SBATCH -C cpu
#SBATCH -t 00:30:00

#######################################
########Ripples MPI+OpenMP RUN#########
#######################################

echo "Started on `/bin/hostname`"
cd $SLURM_SUBMIT_DIR
export OMP_NUM_THREADS=4
srun -n 512 -c 4 --cpu-bind none $HOME/ripples/build/Release/tools/mpi-imm -i com-youtube.ungraph-LT.txt -w -p -u -k 100 -d LT -e 0.13 --parallel -o inf-RMPIOP-yt.json