#!/bin/sh

set -e
if [ ! -d code ]; then
	echo This script must be run in rts/lib/assimp!
	exit 1
fi

replace_func () {
	sed -i "s/std::$1\b/math::$1/g" $file
	sed -i "s/\([^:a-zA-Z0-9_]\|^\)$1 *(/\1math::$1(/g" $file
}

for file in $(find -name '*.cpp' -or -name '*.h' -or -name '*.inl');
do
	sed -i 's/<cmath>/"lib\/streflop\/streflop_cond.h"/g' $file
	sed -i 's/<math.h>/"lib\/streflop\/streflop_cond.h"/g' $file
	replace_func fabs;
	replace_func sin;
	replace_func cos;
	replace_func cosf;

	replace_func sinh;
	replace_func cosh;
	replace_func tan;
	replace_func tanh;
	replace_func asin;
	replace_func acos;
	replace_func atan;
	replace_func atan2;

	replace_func ceil;
	replace_func floor;
	replace_func fmod;
	replace_func hypot;
	replace_func pow;
	replace_func log;
	replace_func log10;
	replace_func exp;
	replace_func frexp;
	replace_func ldexp;

	replace_func isnan;
	replace_func isinf;
	replace_func isfinite;

	replace_func sqrt
	replace_func sqrtf
	replace_func isqrt


	echo Processed $file
done

#sed -i 's/double/float/g' code/PolyTools.h
#sed -i 's/double/float/g' contrib/clipper/clipper.*
