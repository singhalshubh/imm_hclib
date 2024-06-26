#!/bin/bash

module load gcc python/3.9 openmpi/4.1.4 cmake
module load anaconda3

export PATH=$PATH:$HOME/.local/bin

# >>> conda initialize >>>
# !! Contents within this block are managed by 'conda init' !!
__conda_setup="$('/global/common/software/nersc/pm-2022q3/sw/python/3.9-anaconda-2021.11/bin/conda' 'shell.bash' 'hook' 2> /dev/null)"
if [ $? -eq 0 ]; then
    eval "$__conda_setup"
    
else
    if [ -f "/global/common/software/nersc/pm-2022q3/sw/python/3.9-anaconda-2021.11/etc/profile.d/conda.sh" ]; then
        . "/global/common/software/nersc/pm-2022q3/sw/python/3.9-anaconda-2021.11/etc/profile.d/conda.sh"
    else
        export PATH="/global/common/software/nersc/pm-2022q3/sw/python/3.9-anaconda-2021.11/bin:$PATH"
    fi
fi
unset __conda_setup
# <<< conda initialize <<<