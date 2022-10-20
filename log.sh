

# name='four'
# ./threes --total=1000000 --limit=1000 --block=1000 --slide_name="${name}"_tuple_slider --slide="alpha=0.01 save=${name}.bin" > ./log/"${name}".log

# name='six'
# ./threes --total=500000 --limit=1000 --block=1000 --slide_name="${name}"_tuple_slider --slide="alpha=0.13 save=${name}.bin" > ./log/"${name}".log
# ./threes --total=300000 --limit=1000 --block=1000 --slide_name="${name}"_tuple_slider --slide="alpha=0.08 load=${name}.bin save=${name}.bin" >> ./log/"${name}".log
# ./threes --total=200000 --limit=1000 --block=1000 --slide_name="${name}"_tuple_slider --slide="alpha=0.03 load=${name}.bin save=${name}.bin" >> ./log/"${name}".log

name='best_six'
./threes --total=500000 --limit=1000 --block=1000 --slide_name="${name}"_tuple_slider --slide="alpha=0.13 save=${name}.bin" > ./log/"${name}".log
./threes --total=300000 --limit=1000 --block=1000 --slide_name="${name}"_tuple_slider --slide="alpha=0.01 load=${name}.bin save=${name}.bin" >> ./log/"${name}".log
./threes --total=200000 --limit=1000 --block=1000 --slide_name="${name}"_tuple_slider --slide="alpha=0.001 load=${name}.bin save=${name}.bin" >> ./log/"${name}".log

# make
# make train_init file="${name}.bin" > /dev/null

# for i in {1..1000};
# do
#     make train file="${name}.bin" > ./log/"${name}-tuple"/"${i}.txt"
# done

./judge2/threes-judge --load=stat.txt