all:
	g++ -std=c++17 -O3 -g -Wall -fmessage-length=0 -o threes threes.cpp
stats:
	./threes --total=1000 --save=stats.txt --slide_name=best_six_tuple_slider --slide="learn=no_learn load=best_six.bin"
clean:
	rm threes
	rm stats.txt
