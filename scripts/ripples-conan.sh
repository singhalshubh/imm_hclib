#!/bin/bash

echo 'Running the first time setup script'

module load gcc/12.1.0-qgxpzk
module load python
module load cmake

export PATH=$PATH:$HOME/.local/bin
eval "$__conda_setup"