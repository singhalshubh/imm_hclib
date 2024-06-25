##########################
cd $LOC/imm_hclib/tests
python -m pip install -U matplotlib

a_datafiles=(cit-HepPh soc-Epinions1 com-dblp.ungraph com-youtube.ungraph soc-pokec-relationships soc-LiveJournal1 com-orkut.ungraph)
pe=(2 4 8 16)

export DATASET_PATH=~/scratch/imm-dataset

for df in ${datafiles[@]}
do
    for pe in ${pe[@]}
    do
        sed -i '1d' inf-1d-$df-$pe.txt
        sed -i '1d' inf-2d-$df-$pe.txt
    done
done

python interpret.py cit-HepPh 2
python interpret.py soc-Epinions1 2
python interpret.py com-dblp.ungraph 2
python interpret.py com-youtube.ungraph 2
python interpret.py soc-pokec-relationships 8
python interpret.py soc-LiveJournal1 8
python interpret.py com-orkut.ungraph 8