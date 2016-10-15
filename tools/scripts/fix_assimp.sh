#!/bin/sh

set -e
if [ ! -d code ]; then
	echo This script must be run in rts/lib/assimp!
	exit 1
fi

for file in $(find -name '*.cpp' -or -name '*.h' -or -name '*.inl');
do
	sed -i 's/<cmath>/"lib/streflop/streflop_cond.h"/g' $file
	sed -i 's/<math.h>/"lib/streflop/streflop_cond.h"/g' $file
	sed -i 's/std::sin/math::sin/g' $file
	sed -i 's/std::sqrt/math::sqrt/g' $file
	sed -i 's/std::acos/math::acos/g' $file
	sed -i 's/std::cos/math::cos/g' $file
	sed -i 's/std::fabs/math::fabs/g' $file
	sed -i 's/std::pow/math::pow/g' $file
done

sed -i 's/double/float/g' code/PolyTools.h
sed -i 's/double/float/g' contrib/clipper/clipper.*
