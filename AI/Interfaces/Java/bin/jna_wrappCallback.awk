#!/bin/awk
#
# This awk script creates a Java class with JNA functions
# to call to C function pointers in:
# rts/ExternalAI/Interface/SSkirmishAICallback.h
#
# This script uses functions from the following files:
# * common.awk
# * commonDoc.awk
# Variables that can be set on th ecommand-line (with -v):
# * GENERATED_SOURCE_DIR: will contain the generated sources
#
# usage:
# 	awk -f thisScript.awk -f common.awk -f commonDoc.awk
# 	awk -f thisScript.awk -f common.awk -f commonDoc.awk -v 'GENERATED_SOURCE_DIR=/tmp/build/AI/Interfaces/Java/generated-java-src'
#

BEGIN {
	# initialize things

	# define the field splitter(-regex)
	#FS = /,|\(|\)\;/
	FS = "(,)|(\\()|(\\)\\;)";

	# These vars can be assigned externally, see file header.
	# Set the default values if they were not supplied on the command line.
	if (!GENERATED_SOURCE_DIR) {
		GENERATED_SOURCE_DIR = "../java/generated";
	}

	javaSrcRoot = "../java/src";
	javaGeneratedSrcRoot = GENERATED_SOURCE_DIR;

	myPkgA = "com.springrts.ai";
	myPkgD = convertJavaNameFormAToD(myPkgA);
	myInterface = "AICallback";
	myDefaultClass = "DefaultAICallback";
	myWin32Class = "Win32AICallback";

	fi = 0;
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

function createJavaFileName(clsName_f) {
	return javaGeneratedSrcRoot "/" myPkgD "/" clsName_f ".java";
}

function printInterface() {

	outFile_i = createJavaFileName(myInterface);

	printHeader(outFile_i, myPkgA, myInterface);

	for (i=0; i < fi; i++) {
		fullName = funcFullName[i];
		retType = funcRetType[i];
		paramList = funcParamList[i];

		printFunctionComment_Common(outFile_i, funcDocComment, i, "\t");
		print("\t" retType " " fullName "(" paramList ");") >> outFile_i;
	}

	print("}") >> outFile_i;
	print("") >> outFile_i;
	close(outFile_i);
}

function printClass(clsName_c) {

	outFile_c = createJavaFileName(clsName_c);

	printHeader(outFile_c, myPkgA, clsName_c);

	clbType_c = "Callback";
	if (clsName_c == myWin32Class) {
		clbType_c = "StdCallLibrary.StdCallCallback";
	}

	# print the static instantiator method
	print("\t" "public static " clsName_c " getInstance(Pointer memory) {") >> outFile_c;
	print("") >> outFile_c;
	print("\t" "	" clsName_c " _inst = new " clsName_c "();") >> outFile_c;
	print("\t" "	_inst.useMemory(memory);") >> outFile_c;
	print("\t" "	_inst.read();") >> outFile_c;
	print("\t" "	return _inst;") >> outFile_c;
	print("\t" "}") >> outFile_c;
	print("") >> outFile_c;

	for (i=0; i < fi; i++) {
		fullName = funcFullName[i];
		retType = funcRetType[i];
		paramList = funcParamList[i];

		isVoid_c = (retType == "void");
		condRet_c = isVoid_c ? "" : "return ";
		paramListNoTypes = removeParamTypes(paramList);

		print("\t" "public interface _" fullName " extends " clbType_c " {") >> outFile_c;
		print("\t" "\t" retType " invoke(" paramList ");") >> outFile_c;
		print("\t" "}") >> outFile_c;
		print("\t" "public _" fullName " _M_" fullName ";") >> outFile_c;
		printFunctionComment_Common(outFile_c, funcDocComment, i, "\t");
		print("\t" "public " retType " " fullName "(" paramList ") {") >> outFile_c;
		print("\t" "\t" condRet_c "_M_" fullName ".invoke(" paramListNoTypes ");") >> outFile_c;
		print("\t" "}") >> outFile_c;
		print("") >> outFile_c;
	}

	print("}") >> outFile_c;
	print("") >> outFile_c;
	close(outFile_c);
}


function wrappFunction(funcDef) {

	# All function pointers of the struct have to be mirrored,
	# otherwise, JNA will call the wrong function pointers.
	doWrapp = 1;

	if (doWrapp) {
		size_funcParts = split(funcDef, funcParts, "(,)|(\\()|(\\)\\;)");
		# because the empty part after ");" would count as part aswell
		size_funcParts--;
		retType_c = trim(funcParts[1]);
		retType_jna = convertCToJNAType(retType_c);
		#condRet = match(retType_c, "void") ? "" : "return ";
		fullName = funcParts[2];
		sub(/CALLING_CONV \*/, "", fullName);
		sub(/\)/, "", fullName);

		# function parameters
		paramList = "";

		for (i=3; i<=size_funcParts && !match(funcParts[i], /.*\/\/.*/); i++) {
			name = extractParamName(funcParts[i]);
			type_c = extractParamType(funcParts[i]);
			type_jna = convertCToJNAType(type_c);
			if (i == 3) {
				cond_comma = "";
			} else {
				cond_comma = ", ";
			}
			paramList = paramList cond_comma type_jna " " name;
		}

		funcFullName[fi] = fullName;
		funcRetType[fi] = retType_jna;
		funcParamList[fi] = paramList;
		storeDocLines(funcDocComment, fi);
		fi++;
	} else {
		print("warninig: function intentionally NOT wrapped: " funcDef);
	}
}



# This function has to return true (1) if a doc comment (eg: /** foo bar */)
# can be deleted.
# If there is no special condition you want to apply,
# it should always return true (1),
# cause there are additional mechanism to prevent accidential deleting.
# see: commonDoc.awk
function canDeleteDocumentation() {
	return isMultiLineFunc != 1;
}



# save function pointer info into arrays
# ... 2nd, 3rd, ... line of a function pointer definition
{
	if (isMultiLineFunc) { # function is defined on one single line
		funcIntermLine = $0;
		# remove possible comment at end of line: // fu bar
		sub(/[ \t]*\/\/.*/, "", funcIntermLine);
		funcIntermLine = trim(funcIntermLine);
		funcSoFar = funcSoFar " " funcIntermLine;
		if (match(funcSoFar, /\;$/)) {
			# function ends in this line
			wrappFunction(funcSoFar);
			isMultiLineFunc = 0;
		}
	}
}
# 1st line of a function pointer definition
/^[^\/]*CALLING_CONV.*$/ {

	funcStartLine = $0;
	# remove possible comment at end of line: // fu bar
	sub(/\/\/.*$/, "", funcStartLine);
	funcStartLine = trim(funcStartLine);
	if (match(funcStartLine, /\;$/)) {
		# function ends in this line
		wrappFunction(funcStartLine);
	} else {
		funcSoFar = funcStartLine;
		isMultiLineFunc = 1;
	}
}




END {
	# finalize things

	printInterface();
	printClass(myDefaultClass);
	printClass(myWin32Class);
}
