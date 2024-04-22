make

mkdir -p log outputs
if [ "$#" -ne 3 ]; then
    args="-i inputs/${1}.txt"
else
    conf=$2
    mode=$3

    args="-i inputs/${1}.txt -c $conf -m $mode"
fi
echo "Command: ./legal ${args}"
./legal ${args} | tee log/${1}.log

echo 
echo "Drawing output:"
echo outputs/${1}_init.png
echo outputs/${1}_solution.png
echo outputs/${1}_solution_fp.png
python3 utils/draw_tile_layout.py outputs/${1}_init.txt outputs/${1}_init.png 
python3 utils/draw_tile_layout.py outputs/${1}_legal.txt outputs/${1}_solution.png 
python3 utils/draw_full_floorplan.py outputs/${1}_legal_fp.txt outputs/${1}_solution_fp.png 

# eog outputs/${1}_solution.png &
# eog outputs/${1}_solution_fp.png &
# eog outputs/${1}_init.png &