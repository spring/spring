#!/bin/bash

set -e

TESTRUNS=4

CMD[0]="./spring_pic --benchmark 20 --benchmarkstart 0"
CMD[1]="./spring_nonpic --benchmark 20 --benchmarkstart 0"
CMD[2]="./spring_mt --benchmark 20 --benchmarkstart 0"

#CMD1=$CMD2

SCRIPT="script_benchmark.txt"
#SCRIPT="script_benchmark_zwzsg.txt"
#SCRIPT="demos/20121114_033937_TheHunters-v3_91.0.1-368-gbca8185 develop.sdf"
DEMOFILE=bench_results_2012-11-20_10-33-36/benchmark.sdf

PREFIX=$PWD/bench_results_$(date +"%Y-%m-%d_%H-%M-%S")


mkdir "$PREFIX"

if ! [ -s "$DEMOFILE" ]; then
	echo Creating demo
	${CMD[0]} "$SCRIPT" >/dev/null 2>&1 &
	wait
	DEMOFILE=`cat infolog.txt | grep "Writing demo: " | cut -c27-`
	echo demo file: $DEMOFILE
	cp -v "$DEMOFILE" "$PREFIX/benchmark.sdf"
	mv benchmark.data "$PREFIX/data-0-cmd1.data"
fi


CMDCOUNT=${#CMD[*]}
for (( i=1; i <= TESTRUNS; i++ )); do
	echo Round $i/$TESTRUNS
	for (( k=0; k < $CMDCOUNT; k++ )); do
		echo Running CMD $(($k+1))/$CMDCOUNT
		${CMD[$k]} "$DEMOFILE" >/dev/null 2>&1
		mv benchmark.data "$PREFIX/data-${i}-cmd${k}.data"
	done
done

#./plot
#./plot_mass.sh $TESTRUNS
