export LOC=$PWD
###### Actor-IMM ####
echo -e 'Cloning IMM-Actor Algorithms'
git clone https://github.com/singhalshubh/imm_hclib
git checkout sc25
echo -e 'Building IMM-Actor Algorithms'
source imm_hclib/scripts/setup.sh
cd imm_hclib/src/
export HClib_WF=$PWD
cd lt_1D/
make
cd ../lt_2D/
make
echo -e 'Ready to run IMM-Actor Algorithms'
#######################
cd $LOC