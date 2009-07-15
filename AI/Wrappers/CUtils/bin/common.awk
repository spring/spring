#!/bin/awk
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
function ltrim(str__common) { sub(/^[ \t]+/, "", str__common); return str__common; }

# right trim: removes trailing white spaces (at the end of a string)
function rtrim(str__common) { sub(/[ \t]+$/, "", str__common); return str__common; }

# trim: removes leading and trailing white spaces
# (at the beginning and the end of a string)
function trim(str__common) { return rtrim(ltrim(str__common)); }

# removes all white spaces from a string
function noSpaces(str__common) { gsub(/[ \t]/, "", str__common); return str__common; }

### END: Trim functions
################################################################################



################################################################################
### BEGIN: Case functions

# makes the first char in a string uppercase
function capitalize(str__common) { return toupper(substr(str__common, 1, 1)) substr(str__common, 2); }

# makes the first char in a string lowercase
function lowerize(str__common) { return tolower(substr(str__common, 1, 1)) substr(str__common, 2); }

# returns 1 if the first char of a string is uppercase, 0 otherwise
function startsWithCapital(str__common) { return match(str__common, /^[ABCDEFGHIJKLMNOPQRDTUVWXYZ]/); }
#function startsWithCapital(str__common) { return match(str__common, /^[A-Z]/); } # this seems not to work in all awk flavours? :/

# returns 1 if the first char of a string is lowercase, 0 otherwise
function startsWithLower(str__common) { return match(str__common, /^[abcdefghijklmnopqrdtuvwxyz]/); }
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

function printGeneratedWarningHeader(outFile__common) {

	print("// WARNING: This file is machine generated,") > outFile__common;
	print("// please do not edit directly!") >> outFile__common;
}

function printGPLHeader(outFile__common) {

	print("/*") >> outFile__common;
	print("	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>") >> outFile__common;
	print("") >> outFile__common;
	print("	This program is free software; you can redistribute it and/or modify") >> outFile__common;
	print("	it under the terms of the GNU General Public License as published by") >> outFile__common;
	print("	the Free Software Foundation; either version 2 of the License, or") >> outFile__common;
	print("	(at your option) any later version.") >> outFile__common;
	print("") >> outFile__common;
	print("	This program is distributed in the hope that it will be useful,") >> outFile__common;
	print("	but WITHOUT ANY WARRANTY; without even the implied warranty of") >> outFile__common;
	print("	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the") >> outFile__common;
	print("	GNU General Public License for more details.") >> outFile__common;
	print("") >> outFile__common;
	print("	You should have received a copy of the GNU General Public License") >> outFile__common;
	print("	along with this program.  If not, see <http://www.gnu.org/licenses/>.") >> outFile__common;
	print("*/") >> outFile__common;
}

function printCommentsHeader(outFile__common) {

	printGeneratedWarningHeader(outFile__common);
	print("") >> outFile__common;
	printGPLHeader(outFile__common);
}

# Searches an array (with strings as keys) with a regex pattern
# returns 1 if the pattern matches at least one key, 0 otherwise
function matchesAnyKey(toSearch, matchArray) {

	for (pattern in matchArray) {
		if (match(toSearch, pattern)) {
			return 1;
		}
	}

	return 0;
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

### END: Java functions
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
	sub(/unsigned/, "", jnaType__common);
	gsub(/[ \t]+\*/, "* ", jnaType__common);

	isComplex__common = 0;
	isComplex__common += sub(/char\*\*/, "Pointer", jnaType__common);
	isComplex__common += sub(/char\*( const)?/, "String", jnaType__common);
	isComplex__common += sub(/struct SAIFloat3\*/, "AIFloat3[]", jnaType__common);
	isComplex__common += sub(/struct SAIFloat3/, "AIFloat3", jnaType__common);
	isComplex__common += sub(/struct SSkirmishAICallback(\*)?/, "AICallback", jnaType__common);
	isComplex__common += sub(/struct [0-9a-zA-Z_]*/, "Structure", jnaType__common);

	isPrimitive__common = 0;
	isPrimitive__common += sub(/bool/, "boolean", jnaType__common);
	isPrimitive__common += sub(/char/, "byte", jnaType__common);
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
