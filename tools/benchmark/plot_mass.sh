#!/bin/sh

TESTRUNS=${1:-3}

CMDS='
set terminal pngcairo enhanced
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

for (( i=1; i <= TESTRUNS; i++ )); do
	CMDS="datafile${i}=\"bench_results${i}/benchmark.data\"
		${CMDS}"
	CMDS="${CMDS}, \\
		avg=init(0), datafile${i} using (\$1/30/60):(blend(1000/\$2,0.9999)) with line title \"${i}\""
done

CMDS="${CMDS}, datafile${TESTRUNS} using (\$1/30/60):(16) with line notitle lc rgb \"green\""
CMDS="${CMDS}, datafile${TESTRUNS} using (\$1/30/60):(33) with line notitle lc rgb \"black\""
CMDS="${CMDS}, datafile${TESTRUNS} using (\$1/30/60):(66) with line notitle lc rgb \"red\""

#CMDS="set title \"B.A.S.P. (${TESTRUNS} testruns)\"
CMDS="set title \"3v3 CAIs (${TESTRUNS} testruns)\"
${CMDS}"


(
cat << EOF
${CMDS}
EOF
) > /tmp/cplot

gnuplot /tmp/cplot
