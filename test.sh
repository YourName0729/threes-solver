# make 

N=331776
best_score=0
best_rule=0
cnt=0

echo Best Score: $score
echo Best Rule: $i
echo Count: $cnt

while true
do
    cnt=$((cnt + 1))

    i=$(($RANDOM * $RANDOM % $N))
    ./threes --total=1000 --save=stats.txt --slide=rule=$i > /dev/null
    # score=$(./judge/threes-judge  --load stats.txt | sed -n '3 p' | grep -o -P '(?<== ).*(?=, m)')
    score=$(./judge/threes-judge  --load stats.txt | tail -1 | cut -d " " -f 2)
    # echo score $score
    # echo $(echo "$score > $best_score" | bc)
    echo $score >> anal.txt
    tput cuu1
    tput el
    if (($(echo "$score > $best_score" | bc) == 1))
    then
        best_score=$score
        best_rule=$i

        tput cuu1
        tput el
        tput cuu1
        tput el
        echo Best Score: $score
        echo Best Rule: $i
    fi
    echo Count: $cnt
done


# make stats
# make clean


# echo $score
# echo $(echo "$score > 1" | bc)