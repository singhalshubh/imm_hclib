export LOC=$PWD
###### Actor-IMM ####
echo -e 'Cloning IMM-Actor Algorithms'
git clone https://github.com/singhalshubh/imm_hclib
cd imm_hclib/
git checkout sc25-scc
echo -e 'Building IMM-Actor Algorithms'
source scripts/setup.sh
cd src/
export HClib_WF=$PWD
cd lt_1D/
make
cd ../lt_2D/
make
echo -e 'Ready to run IMM-Actor Algorithms'
#######################
cd $LOC