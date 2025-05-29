# Influence Maximization (IM) Problem
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.13308653.svg)](https://doi.org/10.5281/zenodo.13308653) 

> Shubhendra Pal Singhal, Souvadra Hati, Jeffrey Young, Vivek Sarkar, Akihiro Hayashi, and Richard Vuduc. 2024. Asynchronous Distributed-Memory Parallel Algorithms for Influence Maximization. In Proceedings of the International Conference for High Performance Computing, Networking, Storage, and Analysis (SC '24). IEEE Press, Article 102, 1–19. https://doi.org/10.1109/SC41406.2024.00108. Please cite us if you use our algorithms. https://dl.acm.org/doi/10.1109/SC41406.2024.00108.

> Official DOI for citing our code: https://doi.org/10.5281/zenodo.13308653

This repository consists of Influence Maximization Kernels for Asynchronous Distributed Run-Time.
Two Workloads - imm_hclib_1D (Actor IMM) and imm_hclib_2D (Actor IMM 2D).

## References
[[Kempe'03]](https://dl.acm.org/doi/10.1145/956750.956769) Kempe, D., Kleinberg, J., & Tardos, É. (2003, August). Maximizing the
           spread of influence through a social network. In Proceedings of the
           ninth ACM SIGKDD international conference on Knowledge discovery and
           data mining (pp. 137-146). ACM.

[[Marco'19]](https://ieeexplore.ieee.org/document/8890991) M. Minutoli, M. Halappanavar, A. Kalyanaraman, A. Sathanur, R. Mcclure and J. McDermott (2019). Fast and Scalable Implementations of Influence Maximization Algorithms. 2019 IEEE International Conference on Cluster Computing (CLUSTER). pp. 1-12. doi: 10.1109/CLUSTER.2019.8890991.

[[ripples]](https://doi.org/10.5281/zenodo.3942705) Marco Minutoli. (2021). pnnl/ripples: (v2.2). Zenodo. https://doi.org/10.5281/zenodo.3942705.

[[SNAP]](http://snap.stanford.edu/data) Jure Leskovec and Andrej Krevl. (2014, June). SNAP Datasets: Stanford Large Network Dataset Collection. http://snap.stanford.edu/data

[[HClib]](https://hclib-actor.com) Sri Raj Paul, Akihiro Hayashi, Kun Chen, Youssef Elmougy, Vivek Sarkar,
A Fine-grained Asynchronous Bulk Synchronous parallelism model for PGAS applications,
Journal of Computational Science,
Volume 69,
2023,
102014,
ISSN 1877-7503,
https://doi.org/10.1016/j.jocs.2023.102014.

# Directory Structure
```tree
├── input_files
│   ├── cit-HepPh-LT.txt
│   ├── com-dblp.ungraph-LT.txt
│   ├── com-youtube.ungraph-LT.txt
│   ├── karate.txt
│   └── soc-Epinions1-LT.txt
├── README.md
├── scripts
│   ├── AE.sh
│   └── setup.sh
└── src
    ├── lt
    │   ├── configuration.h
    │   ├── graph.h
    │   ├── mapper.h
    │   ├── user.h
    │   └── utility.h
    ├── lt_1D
    │   ├── generateRR.h
    │   ├── Makefile
    │   ├── production.cpp
    │   ├── profile.h
    │   └── selectseeds.h
    └── lt_2D
        ├── generateRRR+.h
        ├── Makefile
        ├── production_2D.cpp
        ├── profile.h
        ├── selectseeds_2D.h
        └── selectseedsIMM.h

6 directories, 24 files
```

## Build imm_hclib package

### Dataset 
```bash
cd imm_hclib/input_files/
```
`karate.txt` is the sample input graph which is used to test the correctness of installation. Rest are the SNAP real-world pre-processed graphs.

### Instructions of installation

```bash
wget https://raw.githubusercontent.com/singhalshubh/imm_hclib/refs/heads/sc25/scripts/AE.sh
source AE.sh
```
This script installs the `HClib-Actor` and `trng4` library. You will see two executables named, `production` and `production_2D`.
> Make sure your terminal session has followed the pre-requisite, in case you run into any library [NOT FOUND] errors.

To test whether the installation is done right, please run these two commands from `imm_hclib`. Note that you will see the sample output with `inf.txt` containing 4 influencers for each run.

`srun -N 1 -n 2 ./src/lt_1D/production -f ./input_files/karate.txt -w -o inf.txt -t time.txt -e 0.13 -c -k 4`

```
Application: IMM (Only for LT), Number of influencers: 4, epsilon = 0.130000, output file: inf.txt, Is un-directed: 0, Is-weighted: 1, scale: -1, degree: -1, file: ./input_files/karate.txt

Cyclic Mapping
Total Number of Nodes in G: 34
Reading the graph file
Adjusting weights
Total Number of Edges in G: 78
Graph Info: AVG-degree: 2
Graph Info: Max-degree: 17
STEP 1: Sampling
Delta/PE: 1141
[ESTIMATE]Time taken to generate RR sets in sampling:   0.001 seconds
[ESTIMATE]Time taken to select seeds in sampling:    0.001 seconds
Fraction covered: 0.695004

ThetaFinal/PE: 1087
final,STEP 2: Generate RR final
Time taken to select generate RRR sets:    0.001 seconds
final,STEP 3: Select Seeds
Time taken to select seeds:    0.002 seconds
Total Time:    0.006 seconds
Total Time(generateRR):    0.002 seconds
Total Time(selectseeds):    0.003 seconds
```

`srun -N 1 -n 2 ./src/lt_2D/production_2D -f ./input_files/karate.txt -w -o inf.txt -t time.txt -e 0.13 -c -k 4`

```
Application: IMM (Only for LT), Number of influencers: 4, epsilon = 0.130000, output file: inf.txt, Is un-directed: 0, Is-weighted: 1, scale: -1, degree: -1, file: ./input_files/karate.txt

Cyclic Mapping
Total Number of Nodes in G: 34
Reading the graph file
Adjusting weights
Total Number of Edges in G: 78
Graph Info: AVG-degree: 2
Graph Info: Max-degree: 17
STEP 1: Sampling
Delta/PE: 1141
[ESTIMATE]Time taken to generate RR sets in sampling:    0.001 seconds
[Time until now] in matrixGen: 0.000222
[Time until now] in k loops: 0.000022
[ESTIMATE]Time taken to select seeds in sampling:    0.000 seconds
Fraction covered: 0.700263

ThetaFinal/PE: 1070
final,STEP 2: Generate RR final
Final, Time taken to generate RR sets:    0.001 seconds
final, STEP 3: Select Seeds
[Time until now] in matrixGen: 0.000418
[Time until now] in k loops: 0.000036
Final, Time taken to select seeds:    0.000 seconds
Fraction covered: 0.706468
#RRsets total/pe: 2211
Total Time:    0.023 seconds
Total Time(generateRR):    0.003 seconds
Total Time(selectseeds):    0.000 seconds
```

## Run the program
Run the executables, `production` and `production_2D`.
```bash
srun -N <> -n <> ./src/lt_1D/production -f /<path-to-dataset>/<filename> -c -k <> -e <> -o <> -t <>
srun -N <> -n <> ./src/lt_2D/production_2D -f /<path-to-dataset>/<filename> -c -k <> -e <> -o <> -t <>
```

For `srun` flags, `N` refers to total number of nodes, and `n` refers to total number of cores in the system.
Mandatory flags for running imm_hclib_1D and imm_hclib_2D:
- `f` for input dataset file name and path (use full system path to avoid any errors)
- `t` for output file which stores total time taken by program
- `c` for cyclic distribution
- `k` for the number of influencers
- `e` for value of epsilon
- `o` for output file name, which stores IDs of vertices that were selected as *influencers*
- `w` for weighted graph and,
- `u` for undirected graph

### Helper commands for reproducing figures of paper
Showcasing the example of how to run `com-youtube.ungraph-LT.txt` for k = 100, e = 0.13 and varying cores on x-axis. Suppose you have `24` cores per node, then for running on 2 nodes,

```
cd imm_hclib/
srun -N 2 -n 48 ./src/lt_1D/production -f ./input_files/com-youtube.ungraph-LT.txt -u -w -o inf.txt -t time.txt -e 0.13 -c -k 100
srun -N 2 -n 48 ./src/lt_2D/production_2D -f ./input_files/com-youtube.ungraph-LT.txt -u -w -o inf.txt -t time.txt -e 0.13 -c -k 100
```