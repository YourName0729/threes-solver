name='best_six' # ensure three.cpp set the same slider


./threes --total=1000 --slide_name="${name}"_tuple_slider --save=stats.txt --slide="learn=no_learn load=${name}.bin"
./judge2/threes-judge --load=stats.txt --judge="version=2"