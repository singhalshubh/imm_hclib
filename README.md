# Influence Maximization (IM) Problem
This repository consists of Influence Maximization Kernels for Asynchronous Distributed Run-Time.

Two Workloads - imm_hclib_1D (Actor IMM) and imm_hclib_2D (Actor IMM 2D).

## References
[[Kempe'03]](https://dl.acm.org/doi/10.1145/956750.956769) Kempe, D., Kleinberg, J., & Tardos, É. (2003, August). Maximizing the
           spread of influence through a social network. In Proceedings of the
           ninth ACM SIGKDD international conference on Knowledge discovery and
           data mining (pp. 137-146). ACM.

[[Marco'19]](https://ieeexplore.ieee.org/document/8890991) M. Minutoli, M. Halappanavar, A. Kalyanaraman, A. Sathanur, R. Mcclure and J. McDermott (2019). Fast and Scalable Implementations of Influence Maximization Algorithms. 2019 IEEE International Conference on Cluster Computing (CLUSTER). pp. 1-12. doi: 10.1109/CLUSTER.2019.8890991.

[[ripples]](https://doi.org/10.5281/zenodo.4673587) Marco Minutoli. (2021). pnnl/ripples: (v2.1). Zenodo. https://doi.org/10.5281/zenodo.4673587.

[[SNAP]](http://snap.stanford.edu/data) Jure Leskovec and Andrej Krevl. (2014, June). SNAP Datasets: Stanford Large Network Dataset Collection. http://snap.stanford.edu/data

[[HClib]] (https://hclib-actor.com) Sri Raj Paul, Akihiro Hayashi, Kun Chen, Youssef Elmougy, Vivek Sarkar,
A Fine-grained Asynchronous Bulk Synchronous parallelism model for PGAS applications,
Journal of Computational Science,
Volume 69,
2023,
102014,
ISSN 1877-7503,
https://doi.org/10.1016/j.jocs.2023.102014.

# Directory Structure
```tree
├── src/ (implementation of IMM Actor Algorithms) 
│   ├── Makefile
│   ├── configuration.h
│   ├── generateRR.h
│   ├── graph.h
│   ├── imm_hclib_1D.cpp
│   ├── imm_hclib_2D.cpp
│   ├── renumbering.h
│   ├── selectseeds.h
│   ├── selectseeds_2D.h
│   ├── utility.h
├── scripts/ (contains setup scripts for different HPC machines)
│   ├── AE.sh (contains build of Ripples and Actor with sbatch scripts for Figure 5,6 of SC 24 paper).
│   ├── setup.sh (Actor IMM setup)
│   ├── ripples-setup.sh (Ripples setup)
│   ├── ripples-conan.sh (Ripples config)
├── tests/ (contains SLURM scripts for SC submission)
│   ├── actor_01.sh
│   ├── actor_02.sh
│   ├── interpret.py 
│   ├── ripples-MPI_1.sh
│   ├── ripples-MPI_2.sh
│   ├── ripples-MPIOP_1.sh
│   └── ripples-MPIOP_2.sh
│   └── ripples-MPI_OOM.sh
│   ├── plot.sh (To reproduce Figure 5,6 from PACE outputs)
└── README.md
```

# Prerequisite
## HClib run-time system
Please follow the instructions for Perlmutter for installing and loading all the dependencies of IMM Actor (this repo).
Download this repository from the link provided and rename the folder to imm_hclib
```bash
cd imm_hclib/scripts/
source setup.sh
```

## Dataset 
### General Instructions
We recommend using scratch space for your dataset, although it is not a mandatory requisite. We support dataset in edge list format in space-separated format. Dataset should contain vertice labels in the range of [1..N].
```bash
salloc --nodes 1 --qos interactive --time 0:30:00 --constraint cpu
srun -n 1 --cpu-bind none $HOME/ripples/build/Release/tools/dump-graph -i /<path-to-dataset>/<filename> -d LT --normalize -o /<path-to-dataset>/<filename>-LT.txt
``` 

> dump-graph.cc normalizes your dataset from random vertice labels to ordered [1..N] vertex labels. LT model is used to adjust/generate weights based on IMM [[Marco'19]] (https://ieeexplore.ieee.org/document/8890991) strategy same as [[Kempe'03]](https://dl.acm.org/doi/10.1145/956750.956769). Follow the instructions to build dump-graph.cc executable, available at [[ripples]](https://doi.org/10.5281/zenodo.4673587) in ``tools/``.

### Showcasing an example
#### STEP 1: Downloading the dataset - cit-HepPh graph 
```bash
cd $SCRATCH
wget http://snap.stanford.edu/data/cit-HepPh.txt.gz 
gzip -df cit-HepPh.txt.gz
```
#### Step 2: Follow the general instructions 
```bash
salloc --nodes 1 --qos interactive --time 0:30:00 --constraint cpu
srun -n 1 --cpu-bind none $HOME/ripples/build/Release/tools/dump-graph -i $SCRATCH/cit-HepPh.txt -d LT --normalize -o $SCRATCH/cit-HepPh-LT.txt
``` 

#### Sample output for synthetic4x
```
[2024-04-12 07:37:56.982] [console] [info] Loading Done!
[2024-04-12 07:37:56.983] [console] [info] Number of Nodes : 844400
[2024-04-12 07:37:56.983] [console] [info] Number of Edges : 4245213
[2024-04-12 07:37:56.983] [console] [info] Loading took 5047ms
```
# Build and Run workflow
## Build
```bash
cd imm_hclib/src/
make
```
You will see two executables named, `imm_hclib_1D` and `imm_hclib_2D`.
> Make sure your terminal session has followed the pre-requisite, in case you run into any library [NOT FOUND] errors.

## Run 
Run the executables, `imm_hclib_1D` and `imm_hclib_2D`.
```bash
salloc --nodes <> --qos regular --time 0:30:00 --constraint cpu
srun -n <> -c 1 ./imm_hclib_1D -f /<path-to-dataset>/<filename> -d LT -k <> -e <> -o <>
srun -n <> -c 1 ./imm_hclib_2D -f /<path-to-dataset>/<filename> -d LT -k <> -e <> -o <>
```
`salloc` and `srun` options for running imm_hclib_1D and imm_hclib_2D:
- `nodes` for the number of nodes you desire to run our program on.
- `n` for the total number of cores
> e.g. on Perlmutter, `n = 128 * nodes`
  
mandatory flags for running imm_hclib_1D and imm_hclib_2D:
- `f` for input dataset file name
- `d` LT
- `k` for the number of influencers
- `e` for value of epsilon
- `o` for output file name, which stores IDs of vertices that were selected as *influencers*
  
optional flags, for supporting different kinds of datasets:
- `u` for undirected graph and, 
- `w` for weighted graph
  
### Showcasing an example, for 100 influencers, &epsilon; = 0.2 for 2 nodes of Perlmutter
Once you `make` and generate two executables `imm_hclib_1D` and `imm_hclib_2D`, this section shows how to run IMM Actor for `cit-HepPh-LT.txt` dataset.  
```bash
salloc -N 2 --qos regular --time 0:30:00 --constraint cpu
srun -n 256 ./imm_hclib_1D -f $SCRATCH/cit-HepPh-LT.txt -d LT -k 100 -e 0.2 -o influencers1D-citHepPh.txt
srun -n 256 ./imm_hclib_2D -f $SCRATCH/cit-HepPh-LT.txt -d LT -k 100 -e 0.2 -o influencers2D-citHepPh.txt
```
You can see the execution prints of the program on your terminal screen. Influencer IDs are stored in two files, `influencers1D-citHepPh.txt` for IMM Actor and `influencers2D-citHepPh.txt` for IMM Actor 2D.

### Sample output
Please do not compare analytical variable values printed, as they will differ greatly even with the slightest change in configuration or system.
```
Application: IMM, Filename: /storage/scratch1/8/ssinghal74/imm-dataset/cit-HepPh-LT.txt, number of influencers: 100, epsilon = 0.130000, output file: inf-2D_1.txt, Model: LT, Is un-directed: 0, Is weighted: 1

Total Number of Nodes in G: 34546
Total Number of Edges in G: 421578
STEP 1: Sampling
Delta/PE: 229
[ESTIMATE]Time taken to generate RR sets in sampling:   0.009 seconds
[ESTIMATE]Time taken to select seeds in sampling:    0.043 seconds
Fraction covered: 0.015250
Delta/PE: 228
[ESTIMATE]Time taken to generate RR sets in sampling:   0.010 seconds
[ESTIMATE]Time taken to select seeds in sampling:    0.035 seconds
Fraction covered: 0.012913
Delta/PE: 457
[ESTIMATE]Time taken to generate RR sets in sampling:   0.010 seconds
[ESTIMATE]Time taken to select seeds in sampling:    0.037 seconds
Fraction covered: 0.011254
Delta/PE: 913
[ESTIMATE]Time taken to generate RR sets in sampling:   0.010 seconds
[ESTIMATE]Time taken to select seeds in sampling:    0.053 seconds
Fraction covered: 0.009981
Delta/PE: 1827
[ESTIMATE]Time taken to generate RR sets in sampling:   0.012 seconds
[ESTIMATE]Time taken to select seeds in sampling:    0.092 seconds
Fraction covered: 0.009509
Delta/PE: 3654
[ESTIMATE]Time taken to generate RR sets in sampling:   0.016 seconds
[ESTIMATE]Time taken to select seeds in sampling:    0.173 seconds
Fraction covered: 0.009094
Delta/PE: 7308
[ESTIMATE]Time taken to generate RR sets in sampling:   0.021 seconds
[ESTIMATE]Time taken to select seeds in sampling:    0.404 seconds
Fraction covered: 0.008949

ThetaFinal/PE: 7239
final,STEP 2: Generate RR final
Time taken to select generate RRR sets:    0.024 seconds
final,STEP 3: Select Seeds
Time taken to select seeds:    0.657 seconds
Total Time:    1.606 seconds
Total Time(generateRR):    0.112 seconds
Total Time(selectseeds):    1.494 seconds
```
