#!/bin/sh

TESTRUNS=4

CMD1="./spring_pic --benchmark 20 --benchmarkstart 0"
CMD2="./spring_nonpic --benchmark 20 --benchmarkstart 0"
#CMD1=$CMD2

SCRIPT="script_benchmark.txt"
#SCRIPT="script_benchmark_zwzsg.txt"
#SCRIPT="demos/20121114_033937_TheHunters-v3_91.0.1-368-gbca8185 develop.sdf"
#DEMOFILE="$SCRIPT"

for (( i=1; i <= TESTRUNS; i++ )); do
	if (( (i % 2) > 0 )); then
		CMD=$CMD1
	else
		CMD=$CMD2
	fi

	if (( i == 1)); then
		$CMD "$SCRIPT" &
		CPID=$!
		sleep 10s
		DEMOFILE=`lsof -p $CPID -Fn | grep demos | cut -c2-`
	else
		$CMD "$DEMOFILE" &
		CPID=$!
	fi
	wait $CPID

	./plot
	mkdir "bench_results${i}"
	mv benchmark.data "bench_results${i}/"
	mv bench*.png "bench_results${i}/"
done

./plot_mass.sh $TESTRUNS
