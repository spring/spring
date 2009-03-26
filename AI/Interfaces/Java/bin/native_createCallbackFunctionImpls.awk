#!/bin/awk
#
# This awk script creates function impls that can be used in the C callbacks
# with some little adjustments; eg:
# rts/ExternalAI/SSkirmishAICallbackImpl.cpp
# rts/ExternalAI/SAIInterfaceCallbackImpl.cpp
#
# Accepts input like this:
# [code]
# 	bool allowTeamColors;
# 
# 	// construction related fields
# 	/// Should constructions without builders decay?
# 	bool constructionDecay;
# [/code]
#
# use like this:
# 	awk -f yourScript.awk -f common.awk -f commonDoc.awk [additional-params]
# this should work with all flavours of AWK (eg. gawk, mawk, nawk, ...)
#

BEGIN {
	# initialize things

	# define the field splitter(-regex)
	#FS="[ \t]+"

	# 0 -> print impl
	# 1 -> print impl header
	# 2 -> print interface header
	printingHeader = 1;
}

function getCType(gct_type) {

	if (match(gct_type, /string/)) {
		return "const char* const";
	} else {
		return gct_type;
	}
}
function getToCTypeConv(gtctc_type) {

	if (match(gtctc_type, /string/)) {
		return ".c_str()";
	} else {
		return "";
	}
}
function getFuncPrefix(gfp_type) {

	#if (match(gfp_type, /bool/)) {
	#	return "is";
	#} else {
		return "get";
	#}
}

function printFunc(pf_fieldName, pf_type, pf_header) {

	pf_cType = getCType(pf_type);
	pf_toCTypeConv = getToCTypeConv(pf_type);
	pf_funcPrefix = getFuncPrefix(pf_type);

	if (pf_header == 2 && hasStoredDoc()) {
		printStoredDoc("");
	}

	if (pf_header != 0) {
		pf_firstLineEnd = ";";
		pf_sizeToFill = length("const char* const") - length(pf_cType);
		if (pf_sizeToFill < 0) {
			pf_sizeToFill = 0;
		}
		pf_filler = "";
		for (pf_i = 0; pf_i < pf_sizeToFill; pf_i++) {
			pf_filler = pf_filler " ";
		}
	} else {
		pf_firstLineEnd = " {";
		pf_filler = "";
	}

	if (pf_header == 2) {
		print(pf_cType pf_filler " (CALLING_CONV *PreFixName_" pf_funcPrefix capitalize(pf_fieldName) ")(int teamId)" pf_firstLineEnd);
	} else {
		print("EXPORT(" pf_cType pf_filler ") PreFixName_" pf_funcPrefix capitalize(pf_fieldName) "(int teamId)" pf_firstLineEnd);
	}
	if (!pf_header) {
		print("\t" "return modInfo->" pf_fieldName pf_toCTypeConv ";");
		print("}");
	}
}

# needed by commonDoc.awk
function canDeleteDocumentation() {
	return 1;
}


{
	line = trim($0);

	if (line == "") {
		print(line);
	} else if (!isInsideDoc()) {
		sub(/\;$/, "", line);

		name = line;
		sub(/^.*[ \t]/, "", name);

		cppType = line;
		sub(name, "", cppType);
		cppType = rtrim(cppType);

		printFunc(name, cppType, printingHeader);
	}
}



END {
	# finalize things
}
