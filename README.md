# AGILE WF4 (Influence Maximization Problem)

## Prerequisite
To build IMM Actor runtime, please follow the instruction for Perlmutter.
```
wget https://github.gatech.edu/FORZA/workflow4/blob/master/scripts/setup-perlmutter.sh
source setup-perlmutter.sh
```

## How to build and run
### Step 1: Downloading the dataset
Make sure to have your dataset conatining [1..N] vertex labels. Use ripples dump-graph available at, https://github.com/pnnl/ripples/blob/master/tools/dump-graph.cc

### Step 2: Clone this repo
```
cd imm_hclib/src/perlmutter
```
### Step 3: Build the WF
```
make
```
### Step 4: Run the WF
Use ```./imm_hclib_1D -h``` and ```./imm_hclib_2D``` -h  for more help 

```
salloc --nodes 1 --qos interactive --time 0:30:00 --constraint cpu
srun -n 128 -c 1 ./imm_hclib_1D -f /path-to-dataset/ -u -w -d LT -k <> -e <> -o influencers1D.txt
srun -n 128 -c 1 ./imm_hclib_2D -f /path-to-dataset/ -u -w -d LT -k <> -e <> -o influencers2D.txt
```

Add -u for undirected graphs and -w for weighted graphs.
