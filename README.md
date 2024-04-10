# Influence Maximization (IM) Problem
Two Workloads - imm_hclib_1D (Actor IMM) and imm_hclib_2D (Actor IMM 2D).

## References
[[Kempe'03]](https://dl.acm.org/doi/10.1145/956750.956769) Kempe, D., Kleinberg, J., & Tardos, É. (2003, August). Maximizing the
           spread of influence through a social network. In Proceedings of the
           ninth ACM SIGKDD international conference on Knowledge discovery and
           data mining (pp. 137-146). ACM.

[[Marco'19]](https://ieeexplore.ieee.org/document/8890991) M. Minutoli, M. Halappanavar, A. Kalyanaraman, A. Sathanur, R. Mcclure and J. McDermott (2019). Fast and Scalable Implementations of Influence Maximization Algorithms. 2019 IEEE International Conference on Cluster Computing (CLUSTER). pp. 1-12. doi: 10.1109/CLUSTER.2019.8890991.

[[ripples]](https://doi.org/10.5281/zenodo.4673587) Marco Minutoli. (2021). pnnl/ripples: (v2.1). Zenodo. https://doi.org/10.5281/zenodo.4673587

[[SNAP]](http://snap.stanford.edu/data) Jure Leskovec and Andrej Krevl. (2014, June). SNAP Datasets: Stanford Large Network Dataset Collection. http://snap.stanford.edu/data

# Prerequisite
## Setup HClib run-time system
To build IMM Actor runtime, please follow the instruction for Perlmutter.
```
wget https://anonymous.4open.science/r/imm_hclib-81BD/scripts/setup-perlmutter.sh
source setup-perlmutter.sh
```

## Dataset 
### STEP 1: Downloading the dataset
Showcasing example of downloading cit-HepPh graph from [SNAP]. We recommend using scratch space for dataset.

```
wget http://snap.stanford.edu/data/cit-HepPh.txt.gz 
gzip -df cit-HepPh.txt.gz
```

### Step 2: Make graph (dataset) weighted 

You should see cit-HepPh.txt file in your scratch space. Then,

```
salloc --nodes 1 --qos interactive --time 0:30:00 --constraint cpu
srun -n 1 --cpu-bind none $HOME/ripples/build/Release/tools/dump-graph -i /path-to-dataset/cit-HepPh.txt -d LT --normalize -o /path-to-dataset/cit-HepPh-LT.txt
``` 

dumb-graph.cc normalizes your dataset from random vertice labels to ordered [1..N] vertex labels. LT model is used to generate weights based on IMM [[Marco'19]](https://ieeexplore.ieee.org/document/8890991) strategy same as [[Kempe'03]](https://dl.acm.org/doi/10.1145/956750.956769). Follow the instructions to run dump-graph.cc executable, available at [[ripples]](https://doi.org/10.5281/zenodo.4673587) in ``tools/``.


# Build and Run workflow
## Build
```
Download from <link-provided> and rename folder to imm_hclib
cd imm_hclib/src/perlmutter
make
``` 
Make sure your terminal session has followed the pre-requisite, in case you run into any library [NOT FOUND] errors.

## Run 

```
salloc --nodes 1 --qos interactive --time 0:30:00 --constraint cpu
srun -n 128 -c 1 ./imm_hclib_1D -f /path-to-dataset/cit-HepPh-LT.txt -d LT -k <> -e <> -o influencers1D.txt
srun -n 128 -c 1 ./imm_hclib_2D -f /path-to-dataset/cit-HepPh-LT.txt -d LT -k <> -e <> -o influencers2D.txt
```
extra options for running imm_hclib_1D and imm_hclib_2D
- -u for undirected graph and, 
- -w for weighted graph.

Use ```./imm_hclib_1D -h``` and ```./imm_hclib_2D -h```  for more help.
