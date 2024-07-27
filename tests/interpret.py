#!/usr/bin/env python

import json
import numpy
import matplotlib.pyplot as plt
import sys
import os

def main():
    pes = []
    x = []
    pe = int(sys.argv[2])
    while pe <= 32:
        pes += [pe]
        x += [pe*24]
        pe = pe*2

    while pe <= 32:
        pes += [pe]
        pe = pe*2

    v_actor_1d = []
    for pe in pes:
        with open("./bin/inf-1d-" + sys.argv[1] + "-" + str(pe) + ".txt", "r") as file:
            v_actor_1d.append(float(file.read()))

    v_actor_2d = []
    for pe in pes:
        with open("./bin/inf-2d-" + sys.argv[1] + "-" + str(pe) + ".txt", "r") as file:
            v_actor_2d.append(float(file.read()))

    v_mpi = []
    for pe in pes:
        with open("./bin/inf-MPI-" + sys.argv[1] + "-" + str(pe) + ".json", "r") as file:
            jsonData = json.load(file)
            v_mpi.append(float(jsonData[0]["Total"]))

    v_mpiop = []
    for pe in pes:
        with open("./bin/inf-MPIOP-" + sys.argv[1] + "-" + str(pe) + ".json", "r") as file:
            jsonData = json.load(file)
            v_mpiop.append(float(jsonData[0]["Total"]))

    plt.plot(x, v_actor_1d, marker = 's', color='tab:orange', label='Actor IMM')
    plt.plot(x, v_actor_2d, marker = 'o', color='tab:blue', label='Actor IMM 2D')
    plt.plot(x, v_mpi, marker = '^', color='tab:green', label='Ripples MPI')
    plt.plot(x, v_mpiop, marker = '*', color='tab:red', label='Ripples MPI+OpenMP')
    plt.xscale('log', base=2)
    plt.yscale('log', base=2)
    plt.xlabel("Number of PEs", fontsize="x-large")
    plt.ylabel("Execution time (in seconds)", fontsize="x-large")
    plt.legend(fontsize="x-large")
    plt.savefig(sys.argv[1] + ".png", dpi = 600)

if __name__=="__main__": 
    main()