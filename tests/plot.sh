##########################
python3 -m pip install -U matplotlib

a_datafiles=(soc-Epinions1 cit-HepPh com-dblp.ungraph com-youtube.ungraph)
pes=(2 4 8 16)

export DATASET_PATH=~/scratch/imm-dataset

for df in ${a_datafiles[@]}
do
    for pe in ${pes[@]}
    do
        sed -i '1d' ./bin/inf-1d-$df-$pe.txt
        sed -i '1d' ./bin/inf-2d-$df-$pe.txt
    done
done

python3 interpret.py cit-HepPh 2
python3 interpret.py soc-Epinions1 2
python3 interpret.py com-dblp.ungraph 2
python3 interpret.py com-youtube.ungraph 2
rm -rf bin/ job.out