# for i in {1..256}                                                        ✘ INT  dv
# do
# index=$(printf "%03d" $i)
# echo $index / 256
# python3 ./utils/draw_iccad.py ./inputs/rawInputs/case02-input.txt ./videoFrames/$i.txt ./videoFramesOut/img$index.png 
# done

for group in {0..31}
do
echo $group / 31

i=$((8*$group + 0))
index=$(printf "%03d" $i)
python3 ./utils/draw_iccad.py ./inputs/rawInputs/case02-input.txt ./videoFrames/$i.txt ./videoFramesOut/img$index.png &
i=$((8*$group + 1))
index=$(printf "%03d" $i)
python3 ./utils/draw_iccad.py ./inputs/rawInputs/case02-input.txt ./videoFrames/$i.txt ./videoFramesOut/img$index.png &
i=$((8*$group + 2))
index=$(printf "%03d" $i)
python3 ./utils/draw_iccad.py ./inputs/rawInputs/case02-input.txt ./videoFrames/$i.txt ./videoFramesOut/img$index.png &
i=$((8*$group + 3))
index=$(printf "%03d" $i)
python3 ./utils/draw_iccad.py ./inputs/rawInputs/case02-input.txt ./videoFrames/$i.txt ./videoFramesOut/img$index.png &
i=$((8*$group + 4))
index=$(printf "%03d" $i)
python3 ./utils/draw_iccad.py ./inputs/rawInputs/case02-input.txt ./videoFrames/$i.txt ./videoFramesOut/img$index.png &
i=$((8*$group + 5))
index=$(printf "%03d" $i)
python3 ./utils/draw_iccad.py ./inputs/rawInputs/case02-input.txt ./videoFrames/$i.txt ./videoFramesOut/img$index.png &
i=$((8*$group + 6))
index=$(printf "%03d" $i)
python3 ./utils/draw_iccad.py ./inputs/rawInputs/case02-input.txt ./videoFrames/$i.txt ./videoFramesOut/img$index.png &
i=$((8*$group + 7))
index=$(printf "%03d" $i)
python3 ./utils/draw_iccad.py ./inputs/rawInputs/case02-input.txt ./videoFrames/$i.txt ./videoFramesOut/img$index.png &

wait 
done