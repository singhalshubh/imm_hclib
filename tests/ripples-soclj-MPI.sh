#!/bin/bash
#SBATCH --job-name=ripples-mpi-soclj
#SBATCH -q regular
#SBATCH -N 32
#SBATCH -c 128
#SBATCH -C cpu
#SBATCH -t 00:30:00

#######################################
########Ripples MPI RUN################
#######################################
echo "Started on `/bin/hostname`"
cd $SLURM_SUBMIT_DIR          
export OMP_NUM_THREADS=1
srun -n 4096 -c 1 --cpu-bind none $HOME/ripples/build/Release/tools/mpi-imm -i soc-LiveJournal1-LT.txt -w -p -k 100 -d LT -e 0.13 --parallel -o inf-RMPI-soclj.json