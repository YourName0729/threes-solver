start=$(date +%s)

./threes --total=1 --save=stats.txt --slide_name=td_lambda --slide="load=td_lamb.bin save=td_lamb.bin"

end=$(date +%s)
echo "Elapsed Time: $(($end-$start)) seconds"