#!/bin/sh
# Author: Tobi Vollebregt
#
# Simple helper script to quickly make creg metadata register tables.
# Usage: run the script, paste a class in it, press ctrl+D, copy the metadata
# from stdout, paste the next class, etc.
# Use ctrl+C to quit.
#
# It should work on most types, including pointers, multidimensional arrays,
# template types (e.g. STL), extra qualifiers (mutable, volatile, unsigned, etc.)
# Function pointers, method pointers and references don't work.
#
# Commented out members are included commented out in the member table.
# (Only C++ style "//" comments supported).
#
# Explanation of the script:
# 1) [egrep] Lines matching the pattern for a member declaration are filtered out,
# 2) [egrep] Exclude typedefs.
# 3) [sed] The part before member name is replaced with "CR_MEMBER(", the part
#    after member name is replaced with "),".
# 4) [sed] The same for commented out members (except that the part after the
#    member name is already done by the other sed).
#

while true; do
cat | \
	egrep '(([A-Za-z_][A-Za-z_0-9:<>]*)[ \t*&]+)+[A-Za-z_][A-Za-z_0-9]*(\[[A-Za-z_0-9]*\])*;' | \
	egrep -v 'typedef' | \
	sed -r 's#^[ \t]*(([A-Za-z_0-9:<>]+)[ \t*&]+)+#CR_MEMBER(#g; s#(\[[A-Za-z_0-9]*\])*;.*#),#g' | \
	sed -r 's#^[ \t]*//[ \t]*(([A-Za-z_0-9:<>]+)[ \t*&]+)+#//CR_MEMBER(#g'
done
