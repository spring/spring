#!/bin/bash

set -e

TESTRUNS=4

CMD1="./spring_pic --benchmark 20 --benchmarkstart 0"
CMD2="./spring_nonpic --benchmark 20 --benchmarkstart 0"
#CMD1=$CMD2

SCRIPT="script_benchmark.txt"
#SCRIPT="script_benchmark_zwzsg.txt"
#SCRIPT="demos/20121114_033937_TheHunters-v3_91.0.1-368-gbca8185 develop.sdf"
#DEMOFILE="$SCRIPT"

PREFIX=$PWD/bench_results_$(date +"%Y-%m-%d_%k-%M-%S")


if ! [ -s "$DEMOFILE" ]; then
	echo Creating demo
	$CMD1 "$SCRIPT" >/dev/null 2>&1 &
	CPID=$!
	sleep 10s
	DEMOFILE=`lsof -p $CPID -Fn | grep demos | cut -c2-`
	echo demo file: $DEMOFILE
	wait $CPID
fi


mkdir "$PREFIX"
cp -v "$DEMOFILE" "$PREFIX/benchmark.sdf"
mv benchmark.data "$PREFIX/data-0-cmd1.data"

for (( i=1; i <= TESTRUNS; i++ )); do
	echo Round $i/$TESTRUNS

	$CMD1 "$DEMOFILE" >/dev/null 2>&1
	mv benchmark.data "$PREFIX/data-${i}-cmd1.data"

	$CMD2 "$DEMOFILE" >/dev/null 2>&1
	mv benchmark.data "$PREFIX/data-${i}-cmd1.data"
done

#./plot
#./plot_mass.sh $TESTRUNS
