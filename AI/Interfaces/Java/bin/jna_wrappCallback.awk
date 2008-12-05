#!/bin/awk
#
# This awk script creates a Java class with JNA functions
# to call to C function pointers in:
# rts/ExternalAI/Interface/SAICallback.h
#

BEGIN {
	# initialize things

	# define the field splitter(-regex)
	#FS=/,|\(|\)\;/
	FS="(,)|(\\()|(\\)\\;)"

	javaSrcRoot = "../java/src";

	myPkgA = "com.clan_sy.spring.ai";
	myPkgD = convertJavaNameFormAToD(myPkgA);
	myInterface = "AICallback";
	myDefaultClass = "DefaultAICallback";
	myWin32Class = "Win32AICallback";

	# Print a warning header
	#printHeader(myPkgA, myClass);
	#printLoadNativeFunc(myWrapClass, myWrapVar)

	fi = 0;
}



function printGeneratedWarningHeader(outFile) {

	print("// WARNING: This file is machine generated,") > outFile;
	print("// please do not edit directly!") >> outFile;
}

function printGPLHeader(outFile) {

	print("/*") >> outFile;
	print("	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>") >> outFile;
	print("") >> outFile;
	print("	This program is free software; you can redistribute it and/or modify") >> outFile;
	print("	it under the terms of the GNU General Public License as published by") >> outFile;
	print("	the Free Software Foundation; either version 2 of the License, or") >> outFile;
	print("	(at your option) any later version.") >> outFile;
	print("") >> outFile;
	print("	This program is distributed in the hope that it will be useful,") >> outFile;
	print("	but WITHOUT ANY WARRANTY; without even the implied warranty of") >> outFile;
	print("	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the") >> outFile;
	print("	GNU General Public License for more details.") >> outFile;
	print("") >> outFile;
	print("	You should have received a copy of the GNU General Public License") >> outFile;
	print("	along with this program.  If not, see <http://www.gnu.org/licenses/>.") >> outFile;
	print("*/") >> outFile;
}


function printCommentsHeader(outFile) {

	printGeneratedWarningHeader(outFile);
	print("") >> outFile;
	printGPLHeader(outFile);
}

function printHeader(outFile_h, javaPkg_h, javaClassName_h) {

	printCommentsHeader(outFile_h);
	print("") >> outFile_h;
	print("package " javaPkg_h ";") >> outFile_h;
	print("") >> outFile_h;
	print("") >> outFile_h;
	print("import com.sun.jna.*;") >> outFile_h;
	if (javaClassName_h == myWin32Class) {
		print("import com.sun.jna.win32.StdCallLibrary;") >> outFile_h;
	}
	print("") >> outFile_h;
	print("/**") >> outFile_h;
	print(" * Lets Java Skirmish AIs call back to the Spring engine.") >> outFile_h;
	print(" * We are using JNA, so we do not need native code for Java -> C wrapping.") >> outFile_h;
	print(" * We still need native code for C -> Java wrapping though.") >> outFile_h;
	print(" *") >> outFile_h;
	print(" * @author	AWK wrapper script") >> outFile_h;
	print(" * @version	GENERATED") >> outFile_h;
	print(" */") >> outFile_h;
	if (javaClassName_h == myInterface) {
		print("public interface " javaClassName_h " {") >> outFile_h;
	} else {
		print("public class " javaClassName_h " extends Structure implements Structure.ByReference, " myInterface " {") >> outFile_h;
	}
	print("") >> outFile_h;
}


# Some utility functions

function ltrim(s) { sub(/^[ \t]+/, "", s); return s; }
function rtrim(s) { sub(/[ \t]+$/, "", s); return s; }
function trim(s)  { return rtrim(ltrim(s)); }

function noSpaces(s)  { gsub(/[ \t]/, "", s); return s; }

function capitalize(s)  { return toupper(substr(s, 1, 1)) substr(s, 2); }
function lowerize(s)  { return tolower(substr(s, 1, 1)) substr(s, 2); }

function startsWithCapital(s)  { return match(s, /^[A-Z]/); }
function startsWithLower(s)  { return match(s, /^[a-z]/); }


# Awaits this format:	com.clan_sy.spring.ai
# Returns this format:	com/clan_sy/spring/ai
function convertJavaNameFormAToD(javaNameFormA) {

	javaNameFormD = javaNameFormA;

	gsub(/\./, "/", javaNameFormD);

	return javaNameFormD;
}

function removeParamTypes(params) {

	innerParams = params;

	sub(/^[^ ]* /, "", innerParams);
	gsub(/, [^ ]*/, ",", innerParams);

	return innerParams;
}

# Awaits this format:	const char*[]
# Returns this format:	String[]
function convertCToJNAType(cType) {

	jnaType = trim(cType);

	sub(/const/, "", jnaType);
	sub(/unsigned/, "", jnaType);
	gsub(/ \*/, "* ", jnaType);

	isComplex = 0;
	isComplex += sub(/char\*\*/, "Pointer", jnaType);
	isComplex += sub(/char\*/, "String", jnaType);
	isComplex += sub(/struct SAIFloat3(\*)?/, "AIFloat3", jnaType);
	isComplex += sub(/struct SAICallback(\*)?/, "AICallback", jnaType);
	isComplex += sub(/struct [0-9a-zA-Z_]*/, "Structure", jnaType);

	isPrimitive = 0;
	isPrimitive += sub(/bool/, "boolean", jnaType);
	isPrimitive += sub(/char/, "byte", jnaType);
	#isPrimitive += sub(/wchar_t/, "char", jnaType);
	isPrimitive += sub(/short/, "short", jnaType);
	isPrimitive += sub(/int/, "int", jnaType);
	isPrimitive += sub(/float/, "float", jnaType);
	isPrimitive += sub(/double/, "double", jnaType);

	isPointer = 0;
	if (isComplex <= 0 && isPrimitive <= 0) {
		isPointer += sub(/.*\*/, "Pointer", jnaType);
	}

	# convert possible array length specifiers ("[]" or "[2]")
	gsub(/\*/, "[]", jnaType);
	arrDims = gsub(/\[[^\]]*\]/, "[]", jnaType);

	jnaType = noSpaces(jnaType);

	return jnaType;
}

function createJavaFileName(clsName_f) {
	return javaSrcRoot "/" myPkgD "/" clsName_f ".java";
}

# Awaits this format:	unsinged const char* paramName[]
# Returns this format:	paramName
function extractName(typeAndName) {

	name = trim(typeAndName);

	# Remove possible array length specifiers ("[]" or "[2]")
	gsub(/\[.*\]/, "", name);

	# Remove all type specific parts before the name
	gsub(/.*[ \t]/, "", name);

	return name;
}

# Awaits this format:	unsinged const char* paramName[]
# Returns this format:	unsinged const char**
function extractType(typeAndName) {

	name = extractName(typeAndName);
	type = typeAndName;
	sub(name, "", type);
	type = trim(type);

	return type;
}



function printInterface() {

	outFile_i = createJavaFileName(myInterface);

	printHeader(outFile_i, myPkgA, myInterface);

	for (i=0; i < fi; i++) {
		fullName = funcFullName[i];
		retType = funcRetType[i];
		paramList = funcParamList[i];

		print("\t" retType " " fullName "(" paramList ");") >> outFile_i;
	}

	print("}") >> outFile_i;
	print("") >> outFile_i;
}

function printClass(clsName_c) {

	outFile_c = createJavaFileName(clsName_c);

	printHeader(outFile_c, myPkgA, clsName_c);

	clbType_c = "Callback";
	if (clsName_c == myWin32Class) {
		clbType_c = "StdCallLibrary.StdCallCallback";
	}

	for (i=0; i < fi; i++) {
		fullName = funcFullName[i];
		retType = funcRetType[i];
		paramList = funcParamList[i];

		isVoid_c = (retType == "void");
		condRet_c = isVoid_c ? "" : "return ";
		paramListNoTypes = removeParamTypes(paramList);

		print("\t" "public static interface _" fullName " extends " clbType_c " {") >> outFile_c;
		print("\t" "\t" retType " invoke(" paramList ");") >> outFile_c;
		print("\t" "}") >> outFile_c;
		print("\t" "public _" fullName " _M_" fullName ";") >> outFile_c;
		print("\t" "public " retType " " fullName "(" paramList ") {") >> outFile_c;
		print("\t" "\t" condRet_c "_M_" fullName ".invoke(" paramListNoTypes ");") >> outFile_c;
		print("\t" "}") >> outFile_c;
		print("") >> outFile_c;
	}

	print("}") >> outFile_c;
	print("") >> outFile_c;
}



# save function pointer info into arrays
/^[^\/]*CALLING_CONV.*$/ {

	doWrapp = !match(($0), /.*CALLING_CONV \*File_.*/);

	if (doWrapp) {
		retType_c = trim($1);
		retType_jna = convertCToJNAType(retType_c);
		#condRet = match(retType_c, "void") ? "" : "return ";
		fullName=$2;
		sub(/CALLING_CONV \*/, "", fullName);
		sub(/\)/, "", fullName);

		# function parameters
		paramList=""
	
		if (($3) && !match(($3), /.*\/\/.*/)) {
			name = extractName(($3));
			type_c = extractType(($3));
			type_jna = convertCToJNAType(type_c);
			paramList = paramList type_jna " " name;
		}
		for (i=4; i<=NF && ($i) && !match(($i), /.*\/\/.*/); i++) {
			name = extractName(($i));
			type_c = extractType(($i));
			type_jna = convertCToJNAType(type_c);
			paramList = paramList ", " type_jna " " name;
		}

		funcFullName[fi] = fullName;
		funcRetType[fi] = retType_jna;
		funcParamList[fi] = paramList;
		fi++;
	}
}




END {
	# finalize things

	printInterface();
	printClass(myDefaultClass);
	printClass(myWin32Class);
}
