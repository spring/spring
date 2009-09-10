#!/bin/awk
#
# This awk script creates a Java class with JNA functions
# to call to C function pointers in:
# rts/ExternalAI/Interface/SSkirmishAICallback.h
#
# This script uses functions from the following files:
# * common.awk
# * commonDoc.awk
# Variables that can be set on the command-line (with -v):
# * GENERATED_SOURCE_DIR: the generated sources root dir
#
# usage:
# 	awk -f thisScript.awk -f common.awk -f commonDoc.awk
# 	awk -f thisScript.awk -f common.awk -f commonDoc.awk \
#       -v 'GENERATED_SOURCE_DIR=/tmp/build/AI/Interfaces/Java/src-generated'
#

BEGIN {
	# initialize things

	# define the field splitter(-regex)
	#FS = /,|\(|\)\;/
	FS = "(,)|(\\()|(\\)\\;)";

	# Used by other scripts
	JAVA_MODE = 1;

	# These vars can be assigned externally, see file header.
	# Set the default values if they were not supplied on the command line.
	if (!GENERATED_SOURCE_DIR) {
		GENERATED_SOURCE_DIR = "../src-generated";
	}
	if (!JAVA_GENERATED_SOURCE_DIR) {
		JAVA_GENERATED_SOURCE_DIR = GENERATED_SOURCE_DIR "/java";
	}
	if (!NATIVE_GENERATED_SOURCE_DIR) {
		NATIVE_GENERATED_SOURCE_DIR = GENERATED_SOURCE_DIR "/native";
	}

	javaSrcRoot = "../java/src";
	javaGeneratedSrcRoot = JAVA_GENERATED_SOURCE_DIR;
	nativeGeneratedSrcRoot = NATIVE_GENERATED_SOURCE_DIR;

	nativeBridge = "FunctionPointerBridge";
	bridgePrefix = "bridged__";

	myPkgA = "com.springrts.ai";
	myPkgD = convertJavaNameFormAToD(myPkgA);
	myInterface = "AICallback";
	myDefaultClass = "DefaultAICallback";
	myWin32Class = "Win32AICallback";

	fi = 0;
}

function createNativeFileName(fileName_fn, isHeader_fn) {

	absFileName_fn = nativeGeneratedSrcRoot "/" fileName_fn;
	if (isHeader_fn) {
		absFileName_fn = absFileName_fn ".h";
	} else {
		absFileName_fn = absFileName_fn ".c";
	}

	return absFileName_fn;
}

function printNativeFP2F() {

	outFile_nh = createNativeFileName(nativeBridge, 1);
	outFile_nc = createNativeFileName(nativeBridge, 0);

	printCommentsHeader(outFile_nh);
	printCommentsHeader(outFile_nc);

	print("") >> outFile_nh;
	print("#ifndef __FUNCTION_POINTER_BRIDGE_H") >> outFile_nh;
	print("#define __FUNCTION_POINTER_BRIDGE_H") >> outFile_nh;
	print("") >> outFile_nh;
	print("#include \"ExternalAI/Interface/aidefines.h\"") >> outFile_nh;
	print("#include \"ExternalAI/Interface/SAIFloat3.h\"") >> outFile_nh;
	print("") >> outFile_nh;
	print("#include <stdlib.h>  // size_t") >> outFile_nh;
	print("#include <stdbool.h> // bool, true, false") >> outFile_nh;
	print("") >> outFile_nh;
	print("struct SSkirmishAICallback;") >> outFile_nh;
	print("") >> outFile_nh;
	print("#ifdef __cplusplus") >> outFile_nh;
	print("extern \"C\" {") >> outFile_nh;
	print("#endif") >> outFile_nh;
	print("") >> outFile_nh;
	print("void funcPntBrdg_addCallback(const size_t teamId, const struct SSkirmishAICallback* clb);") >> outFile_nh;
	print("void funcPntBrdg_removeCallback(const size_t teamId);") >> outFile_nh;
	print("") >> outFile_nh;

	print("") >> outFile_nc;
	print("#include \"" nativeBridge ".h\"") >> outFile_nc;
	print("") >> outFile_nc;
	print("#include \"ExternalAI/Interface/SSkirmishAICallback.h\"") >> outFile_nc;
	print("") >> outFile_nc;
	print("") >> outFile_nc;
	#print("static const size_t id_clb_sizeMax = 8 * 1024;") >> outFile_nc;
	print("#define id_clb_sizeMax 8192") >> outFile_nc;
	print("static const struct SSkirmishAICallback* id_clb[id_clb_sizeMax];") >> outFile_nc;
	print("") >> outFile_nc;
	print("void funcPntBrdg_addCallback(const size_t teamId, const struct SSkirmishAICallback* clb) {") >> outFile_nc;
	print("	//assert(skirmishAIId < id_clb_sizeMax);") >> outFile_nc;
	#print("	id_clb[skirmishAIId] = clb;") >> outFile_nc;
	print("	id_clb[teamId] = clb;") >> outFile_nc;
	print("}") >> outFile_nc;
	print("void funcPntBrdg_removeCallback(const size_t teamId) {") >> outFile_nc;
	print("	//assert(skirmishAIId < id_clb_sizeMax);") >> outFile_nc;
	#print("	id_clb[skirmishAIId] = NULL;") >> outFile_nc;
	print("	id_clb[teamId] = NULL;") >> outFile_nc;
	print("}") >> outFile_nc;
	print("") >> outFile_nc;

	for (i=0; i < fi; i++) {
		fullName         = funcFullName[i];
		retType          = funcRetTypeC[i];
		paramList        = funcParamListC[i];
		paramListNoTypes = removeParamTypes(paramList);

		printFunctionComment_Common(outFile_nh, funcDocComment, i, "");
		#print("" retType " " bridgePrefix fullName "(const size_t skirmishAIId, " paramList ");") >> outFile_nh;
		print("EXPORT(" retType ") " bridgePrefix fullName "(" paramList ");") >> outFile_nh;

		#print("" retType " " bridgePrefix fullName "(const size_t skirmishAIId, " paramList ") {") >> outFile_nc;
		print("EXPORT(" retType ") " bridgePrefix fullName "(" paramList ") {") >> outFile_nc;
		condRet = "return ";
		if (retType == "void") {
			condRet = "";
		}
		#print("\t" condRet "id_clb[skirmishAIId]->" fullName "(" paramListNoTypes ");") >> outFile_nc;
		print("\t" condRet "id_clb[teamId]->" fullName "(" paramListNoTypes ");") >> outFile_nc;
		print("" "}") >> outFile_nc;
	}


	print("") >> outFile_nh;
	print("#ifdef __cplusplus") >> outFile_nh;
	print("} // extern \"C\"") >> outFile_nh;
	print("#endif") >> outFile_nh;
	print("") >> outFile_nh;
	print("#endif // __FUNCTION_POINTER_BRIDGE_H") >> outFile_nh;
	print("") >> outFile_nh;

	print("") >> outFile_nc;

	close(outFile_nh);
	close(outFile_nc);
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
		fullName  = funcFullName[i];
		retType   = funcRetType[i];
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
		paramList_c = "";
		paramList = "";

		for (i=3; i<=size_funcParts && !match(funcParts[i], /.*\/\/.*/); i++) {
			name = extractParamName(funcParts[i]);
			type_c = extractParamType(funcParts[i]);
			type_jna = convertCToJNAType(type_c);
			if (i == 3) {
				cond_comma   = "";
				cond_comma_n = "";
			} else {
				cond_comma   = ", ";
				cond_comma_n = ",";
			}
			paramList_c = paramList_c cond_comma_n funcParts[i];
			paramList   = paramList cond_comma type_jna " " name;
		}

		funcFullName[fi] = fullName;
		funcRetTypeC[fi] = retType_c;
		funcRetType[fi] = retType_jna;
		funcParamListC[fi] = paramList_c;
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

	printNativeFP2F();
	printInterface();
	printClass(myDefaultClass);
	printClass(myWin32Class);
}
