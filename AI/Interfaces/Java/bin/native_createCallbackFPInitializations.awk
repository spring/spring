#!/bin/awk
#
# This awk script creates the function pointer initializations for the SSkirmishAICallback struct in:
# rts/ExternalAI/Interface/SSkirmishAICallback.h
#

BEGIN {
	# initialize things

	# define the field splitter(-regex)
	#FS="[ \t]+"
}


# Some utility functions

function ltrim(s) { sub(/^[ \t]+/, "", s); return s; }
function rtrim(s) { sub(/[ \t]+$/, "", s); return s; }
function trim(s)  { return rtrim(ltrim(s)); }


function printInit(functionName) {
	print("	callback->" functionName " = &_" functionName ";")
}


/^.*CALLING_CONV.*$/ {

	line = trim($0);
	doWrapp = !match(line, /^[ \t]*\/\/.*/);
	if (doWrapp) {
		numParts = split(line, parts, /\(CALLING_CONV \*/);
		if (numParts == 2) {
			numParts2 = split(parts[2], parts2, /\)\(/);
			if (numParts2 == 2) {
				fn = parts2[1];
				printInit(fn);
			}
		}
	}
}



END {
	# finalize things
}
