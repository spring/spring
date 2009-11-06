#!/bin/awk
#
# This awk script creates C functions which call C function pointers in:
# rts/ExternalAI/Interface/SSkirmishAICallback.h
#
# Right after running this script, you have to wrapp the native command structs
# into functions.
#
# This script uses functions from the following files:
# * common.awk
# * commonDoc.awk
# Variables that can be set on the command-line (with -v):
# * GENERATED_SOURCE_DIR        the root generated sources dir
#                               default: "../src-generated/main"
# * NATIVE_GENERATED_SOURCE_DIR the native generated sources dir
#                               default: GENERATED_SOURCE_DIR + "/native"
#
# usage:
# 	awk -f thisScript.awk -f common.awk -f commonDoc.awk
# 	awk -f thisScript.awk -f common.awk -f commonDoc.awk \
#       -v 'NATIVE_GENERATED_SOURCE_DIR=/tmp/build/AI/Interfaces/Java/src-generated/main/native'
#

BEGIN {
	# initialize things

	# define the field splitter(-regex)
	FS = "(,)|(\\()|(\\)\\;)";

	# These vars can be assigned externally, see file header.
	# Set the default values if they were not supplied on the command line.
	if (!GENERATED_SOURCE_DIR) {
		GENERATED_SOURCE_DIR = "../src-generated/main";
	}
	if (!NATIVE_GENERATED_SOURCE_DIR) {
		NATIVE_GENERATED_SOURCE_DIR = GENERATED_SOURCE_DIR "/native";
	}

	nativeBridge = "CallbackFunctionPointerBridge";
	bridgePrefix = "bridged__";

	retParamName  = "__retVal";
	retParamCTypes["struct SAIFloat3"] = 1;
	retParamCTypes["*"] = 1;

	fi = 0;
}

function doWrapp(funcIndex_dw) {

	paramListC_dw = funcParamListC[funcIndex_dw];
	doWrapp_dw = !match(paramListC_dw, /char\*\*/) && !match(paramListJava_dw, /SAIFloat3/);

	if (doWrapp_dw) {
		fullName_dw = funcFullName[funcIndex_dw];
		metaInf_dw  = funcMetaInf[funcIndex_dw];
		
		if (match(metaInf_dw, /ARRAY:/)) {
			#doWrapp_dw = 0;
		}
		if (match(metaInf_dw, /MAP:/)) {
			#doWrapp_dw = 0;
		}
		if (match(fullName_dw, "^" bridgePrefix "File_")) {
			doWrapp_dw = 0;
		}
		if (fullName_dw == "Engine_handleCommand") {
			doWrapp_dw = 0;
		}
	}

	return doWrapp_dw;
}

function createNativeFileName(fileName_fn, isHeader_fn) {

	absFileName_fn = NATIVE_GENERATED_SOURCE_DIR "/" fileName_fn;
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
	print("#ifndef __CALLBACK_FUNCTION_POINTER_BRIDGE_H") >> outFile_nh;
	print("#define __CALLBACK_FUNCTION_POINTER_BRIDGE_H") >> outFile_nh;
	print("") >> outFile_nh;
	print("#include \"ExternalAI/Interface/aidefines.h\"") >> outFile_nh;
	#print("#include \"ExternalAI/Interface/SAIFloat3.h\"") >> outFile_nh;
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
	print("#include \"ExternalAI/Interface/AISCommands.h\"") >> outFile_nc;
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

	# print the wrapping functions
	for (i=0; i < fi; i++) {
		fullName         = funcFullName[i];
		retType          = funcRetTypeC[i];
		paramList        = funcParamListC[i];
		sub(/int teamId/, "int _teamId", paramList);
		paramListNoTypes = removeParamTypes(paramList);
		commentEol       = funcCommentEol[i];

		# TODO: remove cause unused (though it could be adapted to do: return float[3] -> out-param float[3])
		# Move some return values to an output parameter form,
		# for example, convert the first to the second:
		# struct SAIFloat3 Unit_getPos(int unitId);
		#             void Unit_getPos(int unitId, struct SAIFloat __retVal);
		hasRetParam = 0;
		sAIFloat3ParamNames = "";
		if (retParamCTypes[retType] == 1) {
			hasRetParam = 1;
			paramList        = paramList ", " retType " " retParamName;
			retNameTmp = "__ret_tmp";
			retParamType = retType;
			if (retType == "struct SAIFloat3") {
				#funcRetType[i]   = "float[]";
				funcParamList[i] = funcParamList[i] ", float[] " retParamName;
				retParamConversion = "\t" retParamName "[0] = " retNameTmp ".x;\n";
				retParamConversion = retParamConversion "\t" retParamName "[1] = " retNameTmp ".y;\n";
				retParamConversion = retParamConversion "\t" retParamName "[2] = " retNameTmp ".z;";
				#sAIFloat3ParamNames = sAIFloat3ParamNames " " retParamName;
			}
			retType = "void";
			# for the JNA/Java part to reflect the changes
			funcRetType[i]    = "void";
		}

		if (doWrapp(i)) {
			# print function declaration to *.h
			printFunctionComment_Common(outFile_nh, funcDocComment, i, "");
			#print("" retType " " bridgePrefix fullName "(const size_t skirmishAIId, " paramList ");") >> outFile_nh;

			outName = bridgePrefix fullName;

			if (commentEol != "") {
				commentEol = " // " commentEol;
			}
			print("EXPORT(" retType ") " outName "(" paramList ");" commentEol) >> outFile_nh;
			print("") >> outFile_nh;

			# print function definition to *.c
			#print("" retType " " bridgePrefix fullName "(const size_t skirmishAIId, " paramList ") {") >> outFile_nc;
			print("EXPORT(" retType ") " outName "(" paramList ") {") >> outFile_nc;

			print(preConversion) >> outFile_nc;
			if (hasRetParam) {
				#print("\t" retParamType " " retNameTmp " = id_clb[skirmishAIId]->" fullName "(" paramListNoTypes ");") >> outFile_nc;
				print("\t" retParamType " " retNameTmp " = id_clb[_teamId]->" fullName "(" paramListNoTypes ");") >> outFile_nc;
				print(retParamConversion) >> outFile_nc;
			} else {
				condRet = "return ";
				if (retType == "void") {
					condRet = "";
				}
				#print("\t" condRet "id_clb[skirmishAIId]->" fullName "(" paramListNoTypes ");") >> outFile_nc;
				print("\t" condRet "id_clb[_teamId]->" fullName "(" paramListNoTypes ");") >> outFile_nc;
			}
			print("" "}") >> outFile_nc;
		} else {
			print("Note: The following function is intentionally not wrapped: " fullName);
		}
	}


	print("") >> outFile_nh;
	print("") >> outFile_nc;

	close(outFile_nh);
	close(outFile_nc);
}


function wrappFunction(funcDef, commentEolTot) {

	doParse = 1; # add a function in case you want to exclude some

	if (doParse) {
		size_funcParts = split(funcDef, funcParts, "(,)|(\\()|(\\)\\;)");
		# because the empty part after ");" would count as part as well
		size_funcParts--;
		retType_c   = trim(funcParts[1]);
		fullName    = funcParts[2];
		sub(/CALLING_CONV \*/, "", fullName);
		sub(/\)/, "", fullName);

		# function parameters
		paramList_c = "";
		paramList = "";

		for (i=3; i<=size_funcParts && !match(funcParts[i], /.*\/\/.*/); i++) {
			type_c   = extractParamType(funcParts[i]);
			type_c   = cleanupCType(type_c);
			name     = extractParamName(funcParts[i]);
			if (i == 3) {
				cond_comma   = "";
			} else {
				cond_comma   = ", ";
			}
			paramList_c = paramList_c cond_comma type_c " " name;
		}

		funcFullName[fi]   = fullName;
		funcRetTypeC[fi]   = retType_c;
		funcParamListC[fi] = paramList_c;
		funcParamList[fi]  = paramList;
		funcCommentEol[fi] = commentEolTot
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
		# separate possible comment at end of line: // foo bar
		commentEol = funcIntermLine;
		if (sub(/.*\/\//, "", commentEol)) {
			commentEolTot = commentEolTot commentEol;
		}
		# remove possible comment at end of line: // foo bar
		sub(/[ \t]*\/\/.*/, "", funcIntermLine);
		funcIntermLine = trim(funcIntermLine);
		funcSoFar = funcSoFar " " funcIntermLine;
		if (match(funcSoFar, /\;$/)) {
			# function ends in this line
			wrappFunction(funcSoFar, commentEolTot);
			isMultiLineFunc = 0;
		}
	}
}
# 1st line of a function pointer definition
/^[^\/]*CALLING_CONV.*$/ {

	funcStartLine = $0;
	# separate possible comment at end of line: // foo bar
	commentEolTot = "";
	commentEol = funcStartLine;
	if (sub(/.*\/\//, "", commentEol)) {
		commentEolTot = commentEolTot commentEol;
	}
	# remove possible comment at end of line: // foo bar
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
}
