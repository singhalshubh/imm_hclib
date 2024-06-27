##########################
python3 -m pip install -U matplotlib
python3 -m pip install -U numpy

a_datafiles=(soc-Epinions1 cit-HepPh com-dblp.ungraph com-youtube.ungraph)
pes=(2 4 8 16)

export DATASET_PATH=~/scratch/imm_dataset

python3 interpret.py cit-HepPh 2
python3 interpret.py soc-Epinions1 2
python3 interpret.py com-dblp.ungraph 2
python3 interpret.py com-youtube.ungraph 2
rm -rf bin/ job.out