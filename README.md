# Influence Maximization (IM) Problem
Two Workloads - imm_hclib_1D (Actor IMM) and imm_hclib_2D (Actor IMM 2D).

## References
[[Kempe'03]](https://dl.acm.org/doi/10.1145/956750.956769) Kempe, D., Kleinberg, J., & Tardos, É. (2003, August). Maximizing the
           spread of influence through a social network. In Proceedings of the
           ninth ACM SIGKDD international conference on Knowledge discovery and
           data mining (pp. 137-146). ACM.

[[Marco'19]](https://ieeexplore.ieee.org/document/8890991) M. Minutoli, M. Halappanavar, A. Kalyanaraman, A. Sathanur, R. Mcclure and J. McDermott (2019). Fast and Scalable Implementations of Influence Maximization Algorithms. 2019 IEEE International Conference on Cluster Computing (CLUSTER). pp. 1-12. doi: 10.1109/CLUSTER.2019.8890991.

[[ripples]](https://doi.org/10.5281/zenodo.4673587) Marco Minutoli. (2021). pnnl/ripples: (v2.1). Zenodo. https://doi.org/10.5281/zenodo.4673587.

[[SNAP]](http://snap.stanford.edu/data) Jure Leskovec and Andrej Krevl. (2014, June). SNAP Datasets: Stanford Large Network Dataset Collection. http://snap.stanford.edu/data

# Directory Structure
```tree
├── src/ (implementation of IMM Actor Algorithms) 
│   │── perlmutter/
│   │   ├── Makefile
│   │   ├── configuration.h
│   │   ├── generateRR.h
│   │   ├── graph.h
│   │   ├── imm_hclib_1D.cpp
│   │   ├── imm_hclib_2D.cpp
│   │   ├── renumbering.h
│   │   ├── selectseeds.h
│   │   ├── selectseeds_2D.h
│   │   ├── utility.h
│   ├── systemA/
│   │   ├── Makefile
│   │   ├── configuration.h
│   │   ├── generateRR.h
│   │   ├── graph.h
│   │   ├── imm_hclib_1D.cpp
│   │   ├── imm_hclib_2D.cpp
│   │   ├── renumbering.h
│   │   ├── selectseeds.h
│   │   ├── selectseeds_2D.h
│   │   ├── utility.h
├── scripts/ (contains setup scripts for different HPC machines)
│   ├── setup-perlmutter.h
│   ├── setup-systemA.h
└── README.md
```

# Prerequisite
## HClib run-time system
Please follow the instructions for Perlmutter for installing and loading all the dependencies of IMM Actor (this repo).
```bash
wget https://anonymous.4open.science/r/imm_hclib-81BD/scripts/setup-perlmutter.sh
source setup-perlmutter.sh
```

## Dataset 
### General Instructions
We recommend using scratch space for your dataset. It is not a mandatory requisite.
```bash
salloc --nodes 1 --qos interactive --time 0:30:00 --constraint cpu
srun -n 1 --cpu-bind none $HOME/ripples/build/Release/tools/dump-graph -i /<path-to-dataset>/<filename> -d LT --normalize -o /<path-to-dataset>/<filename>-LT.txt
``` 

> dumb-graph.cc normalizes your dataset from random vertice labels to ordered [1..N] vertex labels. LT model is used to generate weights based on IMM [[Marco'19]] (https://ieeexplore.ieee.org/document/8890991) strategy same as [[Kempe'03]](https://dl.acm.org/doi/10.1145/956750.956769). Follow the instructions to build dump-graph.cc executable, available at [[ripples]](https://doi.org/10.5281/zenodo.4673587) in ``tools/``.

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

# Build and Run workflow
## Build
Download this repository from the link provided and rename the folder to imm_hclib
```bash
cd imm_hclib/src/perlmutter
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
  
### Showcasing an example
Once you `make` and generate two executables `imm_hclib_1D` and `imm_hclib_2D`, this section shows how to run IMM Actor for `cit-HepPh-LT.txt` dataset.
## For 100 influencers, &epsilon; = 0.2 for 2 nodes of Perlmutter  
```bash
salloc --nodes 2 --qos regular --time 0:30:00 --constraint cpu
srun -n 256 -c 1 ./imm_hclib_1D -f $SCRATCH/cit-HepPh-LT.txt -d LT -k 100 -e 0.2 -o influencers1D-citHepPh.txt
srun -n 256 -c 1 ./imm_hclib_2D -f $SCRATCH/cit-HepPh-LT.txt -d LT -k 100 -e 0.2 -o influencers2D-citHepPh.txt
```
You can see the execution prints of the program on your terminal screen. Influencer IDs are stored in two files, `influencers1D-citHepPh.txt` for IMM Actor and `influencers2D-citHepPh.txt` for IMM Actor 2D.

