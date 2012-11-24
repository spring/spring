#!/bin/bash

if [ $# -le 0 ]; then
	echo "Usage: $0 [[file1] file2 ...]"
	echo "Advanced usage: "
	echo "$0 ../../bench_results_*/data-{1,2,3,4}-cmd{0,1}.data"
	exit 1
fi
set -e

TESTRUNS=$1

echo $TESTRUNS

CMDS='
set terminal pngcairo enhanced size 1024,768
set xlabel "GameTime (in minutes)"
set xtics nomirror
set ytics nomirror
set border 3
#set xrange [0:20]
#set yrange [0:10]

outdir="./"

blend(x,a) = (avg = (avg == 0)? x: avg*a+x*(1-a), avg)
init(x) = (avg = 0)

#set key opaque invert box top left reverse Left
#set yrange [0:200]
set ylabel "FrameTime (in ms)"
set output outdir . "benchmark_total.png"
set label "60FPS" front  at screen 0.5, first 19  tc rgb "green"
set label "30FPS" front  at screen 0.5, first 36  tc rgb "black"
set label "15FPS" front  at screen 0.5, first 70  tc rgb "red"
plot avg=init(0)'

N=-1
for file in $@; do
	N=$((N +1))
	if [ ! -s $file ]; then
		echo "$file doesn't exist!"
		exit 1;
	fi
	DATAFILES="$DATAFILES datafile$N=\"$file\"
"
	CMDS="$CMDS, \\
		avg=init(0), datafile$N using (\$1/30/60):(blend(1000/\$2,0.9999)) with line title \"$N $(basename $file) .data\""
done

#CMDS="$CMDS, \
#"
CMDS="$CMDS,\\
datafile$N using (\$1/30/60):(16) with line notitle lc rgb \"green\""
CMDS="$CMDS, datafile$N using (\$1/30/60):(33) with line notitle lc rgb \"black\""
CMDS="$CMDS, datafile$N using (\$1/30/60):(66) with line notitle lc rgb \"red\""

#CMDS="set title \"B.A.S.P. (${TESTRUNS} testruns)\"
CMDS="set title \"3v3 CAIs (${N} testruns)\"
${CMDS}"


(
cat << EOF
$DATAFILES
$CMDS
EOF
) > /tmp/cplot

gnuplot /tmp/cplot

eog benchmark_total.png
