##########################
python3 -m pip install -U matplotlib

a_datafiles=(soc-Epinions1 cit-HepPh com-dblp.ungraph com-youtube.ungraph soc-pokec-relationships soc-LiveJournal1 com-orkut.ungraph)
pes=(2 4 8 16)

export DATASET_PATH=~/scratch/imm_dataset

python3 interpret.py cit-HepPh 2
python3 interpret.py soc-Epinions1 2
python3 interpret.py com-dblp.ungraph 2
python3 interpret.py com-youtube.ungraph 2
python interpret.py soc-pokec-relationships 8
python interpret.py soc-LiveJournal1 8
python interpret.py com-orkut.ungraph 8
rm -rf bin/ job.out