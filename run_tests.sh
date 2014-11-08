#!/bin/bash

make test
python data_generator.py test_files/schema_example.json test_files/test_data.csv 1000
./test

echo ""

#Measure the performance trend of msort with respect to different file sizes.
#Fix the parameters mem_capacity and k. Plot the performance (in ms) versus different file sizes. Try to generate large files (at least 4 GB files).
##Measure the performance trend of msort with respect to different total memory allowed.
#Fix the parameter k and the file size. Plot the performance (in ms) versus different choices of memory capacity.
##Measure the performance trend of msort with respect to different choices of k.
#Fix the file size and the memory capacity. Plot the performance (in ms) versus different choices of k. Of course, k must be greater than 1.
csvsizes=(100 1000 10000 100000 1000000 10000000)
ksizes=(2 4 8 16 32)
memsizes=(1000 10000 100000)

averages=10

echo "Generating test files."
for i in ${csvsizes[@]}
do
    file="test_files/test_data_$i.csv"
    if ! [ -e $file ]
    then
        python data_generator.py test_files/schema_example.json $file $i
    fi
done

echo "Sorting with msort"
echo ""
echo "Sorting with fixed mem_capacity(1000) and k(4)"
echo -e "Records\tTime"
for i in ${csvsizes[@]}
do
    csv_file="test_files/test_data_$i.csv"
    out_file="test_files/test_data_$i.out"
    time_total=0
    for (( a=0; a<$averages; a++))
    do
        time=$(./msort test_files/schema_example.json $csv_file $out_file 1000 4 cgpa | tail -n 1)
        time=${time:6}
        ((time_total+=time))
    done
    time=$(echo "scale=2; $time_total/$averages" | bc)
    echo -e "$i\t$time"

done
rm -rf test_files/test_data_*.out

echo ""
echo "Sorting with fixed mem_capactiy 1000"
echo -e "Records\tK\tTime"
for i in ${csvsizes[@]}
do
    csv_file="test_files/test_data_$i.csv"

    for k in ${ksizes[@]}
    do
        out_file="test_files/test_data_$i_k$k.out"
        time_total=0
        for (( a=0; a<$averages; a++))
        do
            time=$(./msort test_files/schema_example.json $csv_file $out_file 1000 $k cgpa | tail -n 1)
            time=${time:6}
            ((time_total+=time))
        done
        time=$(echo "scale=2; $time_total/$averages" | bc)
        echo -e "$i\t$k\t$time"

    done
    
done
rm -rf test_files/test_data_*.out

echo ""
echo "Sorting with fixed k 4"
echo -e "Records\tMem\tTime"
for i in ${csvsizes[@]}
do
    csv_file="test_files/test_data_$i.csv"

    for m in ${memsizes[@]}
    do
        out_file="test_files/test_data_$i_m$m.out"
        time_total=0
        for (( a=0; a<$averages; a++))
        do
            time=$(./msort test_files/schema_example.json $csv_file $out_file $m 4 cgpa | tail -n 1)
            time=${time:6}
            ((time_total+=time))
        done
        time=$(echo "scale=2; $time_total/$averages" | bc)
        echo -e "$i\t$m\t$time"
    done
    
done
rm -rf test_files/test_data_*.out


echo ""
echo "Sorting with bsort"
echo -e "Records\tTime"
for i in ${csvsizes[@]}
do
    csv_file="test_files/test_data_$i.csv"
    out_file="test_files/test_data_bsort_$i.out"
    time_total=0
    for (( a=0; a<$averages; a++))
    do
        time=$(./bsort test_files/schema_example.json $csv_file $out_file cgpa | tail -n 1)
        time=${time:6}
        ((time_total+=time))
    done
    time=$(echo "scale=2; $time_total/$averages" | bc)
    echo -e "$i\t$time"
done

rm -rf test_files/test_data_bsort_*.out

echo ""
echo "Experiments complete."
