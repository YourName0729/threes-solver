name='best_six' # ensure three.cpp set the same slider


./threes --total=10 --slide_name=td_lambda --save=stats.txt --slide="learn=no_learn load=td_lamb.bin"
./judge2/threes-judge --load=stats.txt --judge="version=2"