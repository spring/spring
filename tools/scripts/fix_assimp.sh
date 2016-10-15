#!/bin/sh

set -e
if [ ! -d code ]; then
	echo This script must be run in rts/lib/assimp!
	exit 1
fi

replace_func () {
	sed -i "s/std::\([^:a-zA-Z0-9_]\|^\)\(fabs\|sin\|cos\|cos\|sinh\|cosh\|tan\|tanh\|asin\|acos\|atan\|atan2\|ceil\|floor\|fmod\|hypot\|pow\|log\|log10\|exp\|frexp\|ldexp\|isnan\|isinf\|isfinite\|sqrt\|isqrt\)\(f\|l\)\?\b/math::\2/g" $file
	sed -i "s/\([^:a-zA-Z0-9_]\|^\)\(fabs\|sin\|cos\|cos\|sinh\|cosh\|tan\|tanh\|asin\|acos\|atan\|atan2\|ceil\|floor\|fmod\|hypot\|pow\|log\|log10\|exp\|frexp\|ldexp\|isnan\|isinf\|isfinite\|sqrt\|isqrt\)\(f\|l\)\? *(/\1math::\2(/g" $file
}

for file in $(find -name '*.cpp' -or -name '*.h' -or -name '*.inl');
do
	sed -i 's/<cmath>/"lib\/streflop\/streflop_cond.h"/g' $file
	sed -i 's/<math.h>/"lib\/streflop\/streflop_cond.h"/g' $file
	replace_func
	echo Processed $file
done

sed -i 's/double/float/g' code/PolyTools.h
