#!/usr/bin/awk -f
#
# This awk script contains common functions that may be used by other scripts.
# use like this:
# 	awk -f yourScript.awk -f common.awk [additional-params]
# this should work with all flavours of AWK (eg. gawk, mawk, nawk, ...)
#

BEGIN {
	# initialize things
}


################################################################################
### BEGIN: Trim functions

# left trim: removes leading white spaces (at the beginning of a string)
# see noSpaces(str)
# see rtrim(str)
function ltrim(str__common) { sub(/^[ \t\n\r]+/, "", str__common); return str__common; }

# right trim: removes trailing white spaces (at the end of a string)
# see noSpaces(str)
# see ltrim(str)
function rtrim(str__common) { sub(/[ \t\n\r]+$/, "", str__common); return str__common; }

# trim: removes leading and trailing white spaces
# (at the beginning and the end of a string)
# see ltrim(str)
# see rtrim(str)
function trim(str__common) { return rtrim(ltrim(str__common)); }

# removes all white spaces from a string
# While '\f' and '\v' would be white-spaces too, these escapes are not
# supported by all AWK implementations (for example BWK).
# http://www.gnu.org/manual/gawk/html_node/Escape-Sequences.html
function noSpaces(str__common) { gsub(/[ \t\n\r]/, "", str__common); return str__common; }

# returns a string containing only spaces with the same lenght as the input string
function lengtAsSpaces(str__common) { gsub(/./, " ", str__common); return str__common; }

### END: Trim functions
################################################################################



################################################################################
### BEGIN: Case functions

# makes the first char in a string uppercase
function capitalize(str__common) { return toupper(substr(str__common, 1, 1)) substr(str__common, 2); }

# makes the first char in a string lowercase
function lowerize(str__common) { return tolower(substr(str__common, 1, 1)) substr(str__common, 2); }

# returns 1 if the first char of a string is uppercase, 0 otherwise
function startsWithCapital(str__common) { return match(str__common, /^[ABCDEFGHIJKLMNOPQRSTUVWXYZ]/); }
#function startsWithCapital(str__common) { return match(str__common, /^[A-Z]/); } # this seems not to work in all awk flavours? :/

# returns 1 if the first char of a string is lowercase, 0 otherwise
function startsWithLower(str__common) { return match(str__common, /^[abcdefghijklmnopqrstuvwxyz]/); }
#function startsWithLower(str__common) { return match(str__common, /^[a-z]/); } # this seems not to work in all awk flavours? :/

### END: Case functions
################################################################################



################################################################################
### BEGIN: Misc functions

# sort function
# sorts an array based on values in ascending order
# values can be numbers and/or strings
# expects the first array element at index [1]
function mySort(array__common, size__common, temp__common, i__common, j__common) {

	for (i__common = 2; i__common <= size__common; ++i__common) {
		for (j__common = i__common; array[j__common-1] > array[j__common]; --j__common) {
			temp__common = array__common[j__common];
			array__common[j__common] = array__common[j__common-1];
			array__common[j__common-1] = temp__common;
		}
	}
}

function printGPLHeader(outFile__common) {
	print("/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */") > outFile__common;
}

function printGeneratedNoteHeader(outFile__common) {
	print("/* Note: This file is machine generated, do not edit directly! */") >> outFile__common;
}

function printCommentsHeader(outFile__common) {

	printGPLHeader(outFile__common);
	print("") >> outFile__common;
	printGeneratedNoteHeader(outFile__common);
}

function printCond(text_common, outFile__common, condotion_common) {

	if (condotion_common) {
		print(text_common) >> outFile__common;
	}
}

# Searches an array (with strings as keys) with a regex pattern
# returns 1 if the pattern matches at least one key, 0 otherwise
function matchesAnyKey(toSearch__common, matchArray__common) {

	for (pattern__common in matchArray__common) {
		if (match(toSearch__common, pattern__common)) {
			return 1;
		}
	}

	return 0;
}

# Escapes all regex special chars in a string,
# so you can for example search for the literal value of the string
# with a regex search function.
function regexEscape(str__common) {

	strEscaped__common = str__common;

	gsub(/\*/, "\\*", strEscaped__common);

	return strEscaped__common;
}

### END: Misc functions
################################################################################



################################################################################
### BEGIN: General Code functions

# Awaits this format:   "String name"
# Returns this format:  "name"
# or
# Awaits this format:   "const std::map<int, std::string>* idNameMap"
# Returns this format:  "idNameMap"
# or
# Awaits this format:   unsinged const char* varName[]
# Returns this format:  varName
function extractParamName(param__common) {

	paramName__common = param__common;

	paramName__common = trim(paramName__common);
	sub(/^.*[ \t]+/, "", paramName__common);
	sub(/\[.*\]$/, "", paramName__common);

	return paramName__common;
}
#function extractParamName(typeAndName) {
#
#	name = trim(typeAndName);
#	gsub(/ \*/, "* ", name);
#
#	# Remove the type
#	sub(/(const )?(unsigned )?(void|int|float|char|byte|bool|struct [_a-zA-Z0-9]+)(\*)* /, "", name);
#
#	# Remove possible array length specifiers ("[]" or "[2]")
#	gsub(/\[.*\]/, "", name);
#
#	name = trim(name);
#
#	return name;
#}

# Awaits this format:	"const std::map<int, std::string>* idNameMap"
# Returns this format:	"const std::map<int, std::string>*"
function extractParamType(param__common) {

	paramType__common = param__common;

	sub(extractParamName(paramType__common), "", paramType__common);
	paramType__common = trim(paramType__common);

	return paramType__common;
}

# Awaits this format:	"float* []"
# Returns this format:	"float**"
function cleanupCType(cType__common) {

	cTypeClean__common = cType__common;

	gsub(/[ \t]*\[\]/, "*", cTypeClean__common);
	cTypeClean__common = trim(cTypeClean__common);

	return cTypeClean__common;
}

# Awaits this format:	"int teamId, const char* name, std::map<int, std::string> idNameMap"
# Returns this format:	"teamId, name, idNameMap"
function removeParamTypes(params__common) {

	paramNames__common = "";

	size_parts__common = split(params__common, parts__common, ",");

	count_openB__common = 0;
	count_closeB__common = 0;
	paramSoFar__common = "";
	for (i__common = 1; i__common <= size_parts__common; i__common++) {
		paramSoFar__common = paramSoFar__common parts__common[i__common];
		count_openB__common += gsub(/</, "<", parts__common[i__common]);
		count_closeB__common += gsub(/>/, ">", parts__common[i__common]);
		if (count_openB__common > count_closeB__common) {
			# this means, we are inside a type still, eg here: "std::map<int,"
			continue;
		} else {
			# this means, we have a full type plus name in paramSoFar now
			paramNames__common = paramNames__common ", " extractParamName(paramSoFar__common);
			count_openB__common = 0;
			count_closeB__common = 0;
			paramSoFar__common = "";
		}
	}
	sub(/^\,[ \t]+/, "", paramNames__common);

	return paramNames__common;
}

### END: General Code functions
################################################################################



################################################################################
### BEGIN: C functions

# Awaits this format:   int myId
# Returns this format:  int
# or
# Awaits this format:   const char* varName[]
# Returns this format:  const char**
function extractCType(cTypeAndName__common) {

	cType__common = trim(cTypeAndName__common);

	gsub(/[ \t]+\*/, "* ", cType__common);

	# Remove the name
	gsub(" " extractParamName(cTypeAndName__common), "", cType__common);

	gsub(/\[[^\]]*\]/, "*", cType__common);

	return cType__common;
}

### END: C functions
################################################################################



################################################################################
### BEGIN: Java functions

# Awaits this format:	com.springrts.ai
# Returns this format:	com/springrts/ai
function convertJavaNameFormAToD(javaNameFormA__common) {

	javaNameFormD__common = javaNameFormA__common;

	gsub(/\./, "/", javaNameFormD__common);

	return javaNameFormD__common;
}

# Awaits this format:	com.springrts.ai
# Returns this format:	com_springrts_ai
function convertJavaNameFormAToC(javaNameFormA__common) {

	javaNameFormC__common = javaNameFormA__common;

	gsub(/\./, "_", javaNameFormC__common);

	return javaNameFormC__common;
}

### END: Java functions
################################################################################



################################################################################
### BEGIN: JNI functions

# Awaits this format:	jint / jfloat / jbooleanArray / jstring
# Returns this format:	I / F / B[ / Ljava/lang/String;
function convertJNIToSignatureType(jniType__common) {

	signatureType__common = jniType__common;

	isArray = sub(/Array/, "", signatureType__common);

	sub(/void/, "V", signatureType__common);

	sub(/jboolean/, "Z", signatureType__common);
	sub(/jfloat/,   "F", signatureType__common);
	sub(/jbyte/,    "B", signatureType__common);
	sub(/jchar/,    "C", signatureType__common);
	sub(/jdouble/,  "D", signatureType__common);
	sub(/jint/,     "I",  signatureType__common);
	sub(/jlong/,    "J", signatureType__common);
	sub(/jshort/,   "S", signatureType__common);

	sub(/jstring/, "Ljava/lang/String;", signatureType__common);

	if (isArray) {
		signatureType__common = "[" signatureType__common;
	}

	return signatureType__common;
}

# Awaits this format:	int / float / bool[] / char*
# Returns this format:	jint / jfloat / jbooleanArray / jstring
function convertCToJNIType(cType__common, cName) {

	jniType__common = cType__common;

	# remove some stuff we do not need and cleanup
	gsub(/const/, "", jniType__common);
	sub(/unsigned int/, "int", jniType__common);
	sub(/unsigned short/, "short", jniType__common);
	gsub(/[ \t]+\*/, "* ", jniType__common);
	jniType__common = trim(jniType__common);

	isComplex__common = 0;
	if (match(cName, /^ret_/)) {
		isComplex__common += sub(/^char\*/, "jobject",  jniType__common);
	} else {
		isComplex__common += sub(/^char\*/, "jstring",  jniType__common);
	}

	isPrimitive__common = 0;
	isPrimitive__common += sub(/bool/,          "jboolean", jniType__common);
	isPrimitive__common += sub(/byte/,          "jbyte",    jniType__common);
	isPrimitive__common += sub(/unsigned char/, "jbyte",    jniType__common);
	isPrimitive__common += sub(/^char/,         "jchar",    jniType__common);
	isPrimitive__common += sub(/short/,         "jshort",   jniType__common);
	isPrimitive__common += sub(/int/,           "jint",     jniType__common);
	isPrimitive__common += sub(/float/,         "jfloat",   jniType__common);
	#isPrimitive__common += sub(/double/, "jdouble", jniType__common);

	# convert possible array length specifiers ("[]" or "[2]")
	gsub(/\*/, "[]", jniType__common);
	arrDims__common = gsub(/\[[^\]]*\]/, "[]", jniType__common);
	arrDims__common = gsub(/\[\]/, "Array", jniType__common);

	# there is no jstringArray type
	sub(/jstringArray/, "jobjectArray", jniType__common);

	jniType__common = noSpaces(jniType__common);

	return jniType__common;
}

# Awaits this format:	jint / jfloat / jbooleanArray / jstring
# Returns this format:	int / float / boolean[] / String
function convertJNIToJavaType(jniType__common) {

	javaType__common = jniType__common;

	sub(/Array/, "[]", javaType__common);

	sub(/void/, "void", javaType__common);

	sub(/jboolean/, "boolean", javaType__common);
	sub(/jfloat/,   "float",   javaType__common);
	sub(/jbyte/,    "byte",    javaType__common);
	sub(/jchar/,    "char",    javaType__common);
	sub(/jdouble/,  "double",  javaType__common);
	sub(/jint/,     "int",     javaType__common);
	sub(/jlong/,    "long",    javaType__common);
	sub(/jshort/,   "short",   javaType__common);

	sub(/jstring/,    "String",       javaType__common);
	sub(/jobject\[]/, "String[]",     javaType__common);
	sub(/jobject/,    "StringBuffer", javaType__common);

	return javaType__common;
}

### END: JNI functions
################################################################################



################################################################################
### BEGIN: JNA functions

# Awaits this format:	int / float / String
# Returns this format:	Integer / Float / String
function convertJavaBuiltinTypeToClass(javaBuiltinType__common) {

	javaClassType__common = javaBuiltinType__common;

	sub(/int/, "Integer", javaClassType__common);
	sub(/float/, "Float", javaClassType__common);

	sub(/boolean/, "Boolean", javaClassType__common);
	sub(/byte/, "Byte", javaClassType__common);
	sub(/char/, "Character", javaClassType__common);
	sub(/double/, "Double", javaClassType__common);
	sub(/float/, "Float", javaClassType__common);
	sub(/int/, "Integer", javaClassType__common);
	sub(/long/, "Long", javaClassType__common);
	sub(/short/, "Short", javaClassType__common);

	return javaClassType__common;
}

# Awaits this format:	const char*[]
# Returns this format:	String[]
function convertCToJNAType(cType__common) {

	jnaType__common = trim(cType__common);

	sub(/const/, "", jnaType__common);
	sub(/unsigned int/, "int", jnaType__common);
	sub(/unsigned short/, "short", jnaType__common);
	gsub(/[ \t]+\*/, "* ", jnaType__common);

	isComplex__common = 0;
	isComplex__common += sub(/char\*\*/, "Pointer", jnaType__common);
	isComplex__common += sub(/^char\*( const)?/, "String", jnaType__common);
	isComplex__common += sub(/struct SSkirmishAICallback(\*)?/, "AICallback", jnaType__common);
	isComplex__common += sub(/struct [0-9a-zA-Z_]*/, "Structure", jnaType__common);

	isPrimitive__common = 0;
	isPrimitive__common += sub(/bool/, "boolean", jnaType__common);
	isPrimitive__common += sub(/(unsigned )?char/, "byte", jnaType__common);
	#isPrimitive__common += sub(/wchar_t/, "char", jnaType__common);
	isPrimitive__common += sub(/short/, "short", jnaType__common);
	isPrimitive__common += sub(/int/, "int", jnaType__common);
	isPrimitive__common += sub(/float/, "float", jnaType__common);
	isPrimitive__common += sub(/double/, "double", jnaType__common);

	isPointer__common = 0;
	if (isComplex__common <= 0 && isPrimitive__common <= 0) {
		isPointer__common += sub(/.*\*/, "Pointer", jnaType__common);
	}

	# convert possible array length specifiers ("[]" or "[2]")
	gsub(/\*/, "[]", jnaType__common);
	arrDims__common = gsub(/\[[^\]]*\]/, "[]", jnaType__common);

	jnaType__common = noSpaces(jnaType__common);

	return jnaType__common;
}


### END: JNA functions
################################################################################



################################################################################
### BEGIN: C++ functions

# Awaits this format:	const char* const
# Returns this format:	const std::string
function convertCToCppType(cType__common) {

	cppType__common = trim(cType__common);

	sub(/char\*( const)?/, "std::string", cppType__common);

	return cppType__common;
}


### END: C++ functions
################################################################################


END {
	# finalize things
}
