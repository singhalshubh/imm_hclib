## How to interpret results using plot.sh

`plot.sh` reproduces the Figure 5 from 48 to 768 cores on CPU nodes of Phoenix PACE cluster at Georgia Institute of Technology.

### Central idea of paper: 
- `Distribute` the graph onto CPU nodes (unlike Ripples), and perform IMM. (Consequence of not distributing the graph is verified by Claim III)
- Improve the bottleneck of SelectSeeds algorithm from Ripples. (Verified by Claim II)

### Central contributions of paper:
- Actor IMM algorithms for LT model performs better than Ripples LT. (Verified by Claim I)
- Out of Memory errors are solved in IMM Actor algorithms. We show that ripples restricts processing of only upto `com-orkut` under assumption of availability of 8GB/core (cpu-small). (Verified by Claim IV)

### plot.sh
This section helps analyse the contributions of the paper, in correlation with the conclusions drawn from the "Obtained" figures from running Appendix Evaluation.

#### Observations to be drawn
- Actor IMM 1D and 2D algorithms perform better than Ripples MPI and MPI+OpenMP versions. (Claim I)
- Actor IMM 2D in general performs better than Actor IMM 1D. (Claim II)
- Observe the strong scaling in graphs, as graphs are represented on `log` scale. (Claim III)
- For the last submitted job, observe the `OUT-OF-MEMORY` error. Alternatively, peek into job.out by doing
`grep OOM job.out` to verify the observation. (Claim IV)


