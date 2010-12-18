#!/usr/bin/awk -f
#
# This awk script creates a Java class with native/JNI functions,
# plus their respective native counterparts,
# to call to C functions in:
# NATIVE_GENERATED_SOURCE_DIR/CallbackFunctionPointerBridge.h
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
#       -v 'GENERATED_SOURCE_DIR=/tmp/build/AI/Interfaces/Java/src-generated/main'
#

BEGIN {
	# initialize things

	# define the field splitter(-regex)
	FS = "(,)|(\\()|(\\)\\;)";

	# Used by other scripts
	JAVA_MODE = 1;

	# These vars can be assigned externally, see file header.
	# Set the default values if they were not supplied on the command line.
	if (!GENERATED_SOURCE_DIR) {
		GENERATED_SOURCE_DIR = "../src-generated/main";
	}
	if (!JAVA_GENERATED_SOURCE_DIR) {
		JAVA_GENERATED_SOURCE_DIR = GENERATED_SOURCE_DIR "/java";
	}
	if (!NATIVE_GENERATED_SOURCE_DIR) {
		NATIVE_GENERATED_SOURCE_DIR = GENERATED_SOURCE_DIR "/native";
	}

	nativeBridge = "CallbackFunctionPointerBridge";
	bridgePrefix = "bridged__";

	jniBridge = "CallbackJNIBridge";

	myPkgA = "com.springrts.ai";
	myPkgD = convertJavaNameFormAToD(myPkgA);
	myPkgC = convertJavaNameFormAToC(myPkgA);
	myInterface = "AICallback";
	myClass     = "JniAICallback";

	fi = 0;
}

function doWrapp(funcIndex_dw) {

	paramListJava_dw = funcParamList[funcIndex_dw];
	doWrapp_dw = 1;

	fullName_dw = funcFullName[funcIndex_dw];
	if (doWrapp_dw) {
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
	} else {
		print("Java-AIInterface: NOTE: JNI level: Callback: intentionally not wrapped: " fullName_dw);
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

function printNativeJNI() {

	outFile_nh = createNativeFileName(jniBridge, 1);
	outFile_nc = createNativeFileName(jniBridge, 0);

	printCommentsHeader(outFile_nh);
	printCommentsHeader(outFile_nc);

	print("") >> outFile_nh;
	print("#ifndef __CALLBACK_JNI_BRIDGE_H") >> outFile_nh;
	print("#define __CALLBACK_JNI_BRIDGE_H") >> outFile_nh;
	print("") >> outFile_nh;
	print("#include <jni.h>") >> outFile_nh;
	print("") >> outFile_nh;
	print("#ifdef __cplusplus") >> outFile_nh;
	print("extern \"C\" {") >> outFile_nh;
	print("#endif") >> outFile_nh;
	print("") >> outFile_nh;

	print("") >> outFile_nc;
	print("#include \"" jniBridge ".h\"") >> outFile_nc;
	print("") >> outFile_nc;
	print("#include \"" nativeBridge ".h\"") >> outFile_nc;
	print("") >> outFile_nc;
	print("") >> outFile_nc;

	# print the wrapping functions
	for (i=0; i < fi; i++) {
		fullName         = funcFullName[i];
		retType          = funcRetTypeC[i];
		paramList        = funcParamListC[i];
		paramListNoTypes = removeParamTypes(paramList);
		metaInf          = funcMetaInf[i];

		if (doWrapp(i)) {
			javaName = fullName;
			sub("^" bridgePrefix, "", javaName);
			gsub(/_/, "_1", javaName);
			jni_funcName         = "Java_" myPkgC "_" myClass "_" javaName;
			jni_retType          = convertCToJNIType(retType);
			jni_paramList        = "";
			size_params = split(paramList, params, ",");
			for (p=1; p <= size_params; p++) {
				pType_c   = params[p];
				sub(/ [^ ]+$/, "", pType_c);
				pType_c = trim(pType_c);
				pType_jni = convertCToJNIType(pType_c);

				pName = params[p];
				sub(/^.* /, "", pName);

				# these are later used for conversion
				c_paramTypes[p] = pType_c;
				c_paramNames[p] = pName;
				jni_paramTypes[p] = pType_jni;
				jni_paramNames[p] = pName;

				jni_paramList = jni_paramList ", " pType_jni " " pName;
			}
			jni_paramListNoTypes = removeParamTypes(jni_paramList);
			isVoidRet = match(jni_retType, /^void$/);

			# print function declaration to *.h
			#printFunctionComment_Common(outFile_nh, funcDocComment, i, "");
			print("JNIEXPORT " jni_retType " JNICALL " jni_funcName "(JNIEnv* __env, jobject __obj" jni_paramList ");") >> outFile_nh;
			print("") >> outFile_nh;

			# print function definition to *.c
			print("JNIEXPORT " jni_retType " JNICALL " jni_funcName "(JNIEnv* __env, jobject __obj" jni_paramList ") {") >> outFile_nc;
			print("") >> outFile_nc;

			if (!isVoidRet) {
				print("\t" jni_retType " _ret;") >> outFile_nc;
				print("") >> outFile_nc;
			}

			# Return value conversion - pre call
			retType_isString = (match(retType, /^(const )?char*/) && (jni_retType == "jstring"));
			retTypeConv = 0;
			if (retType_isString) {
				print("\t" retType " _retNative;") >> outFile_nc;
				retTypeConv = 1;
			}

			# Params conversion - pre call
			for (p=1; p <= size_params; p++) {
				pType_jni = jni_paramTypes[p];

				if (pType_jni == "jstring") {
					# jstring
					c_paramNames[p] = c_paramNames[p] "_native";
					sub(" " jni_paramNames[p], " " c_paramNames[p], paramListNoTypes);
					print("\t" c_paramTypes[p] " " c_paramNames[p] " = (" c_paramTypes[p] ") (*__env)->GetStringUTFChars(__env, " jni_paramNames[p] ", NULL);") >> outFile_nc;
				} else if (match(pType_jni, /^j.+Array$/)) {
					# primitive arrray
					c_paramNames[p] = c_paramNames[p] "_native";
					sub(" " jni_paramNames[p], " " c_paramNames[p], paramListNoTypes);

					capArrType = pType_jni;
					sub(/^j/, "", capArrType);
					sub(/Array$/, "", capArrType);
					capArrType = capitalize(capArrType);

					_isPrimitive = (capArrType != "Object");
					_isString    = !_isPrimitive && match(c_paramTypes[p], /(const )?char\*\*/);

					print("\t" c_paramTypes[p] " " c_paramNames[p] " = NULL;") >> outFile_nc;
					print("\t" "if (" jni_paramNames[p] " != NULL) {") >> outFile_nc;
					if (_isPrimitive) {
						print("\t\t" c_paramNames[p] " = (" c_paramTypes[p] ") (*__env)->Get" capArrType "ArrayElements(__env, " jni_paramNames[p] ", NULL);") >> outFile_nc;
					} else if (_isString) {
						print("\t\t" "const int " c_paramNames[p] "_size = (int) (*__env)->GetArrayLength(__env, " jni_paramNames[p] ");") >> outFile_nc;
						print("\t\t" c_paramNames[p] " = (" c_paramTypes[p] ") calloc(sizeof(char*), " c_paramNames[p] "_size);") >> outFile_nc;
						print("\t\t" "int " c_paramNames[p] "_i;") >> outFile_nc;
						print("\t\t" "jstring " c_paramNames[p] "_jStr;") >> outFile_nc;
						print("\t\t" "for (" c_paramNames[p] "_i=0; " c_paramNames[p] "_i < " c_paramNames[p] "_size; ++" c_paramNames[p] "_i) {") >> outFile_nc;
						print("\t\t\t" c_paramNames[p] "_jStr = (jstring) (*__env)->GetObjectArrayElement(__env, " jni_paramNames[p] ", " c_paramNames[p] "_i);") >> outFile_nc;
						print("\t\t\t" c_paramNames[p] "[" c_paramNames[p] "_i] = (const char*) (*__env)->GetStringUTFChars(__env, " c_paramNames[p] "_jStr, NULL);") >> outFile_nc;
						print("\t\t" "}") >> outFile_nc;
					} else {
						print("ERROR: do not know how to convert parameter type: " pType_jni);
						exit(1);
					}
					print("\t" "}") >> outFile_nc;
				}
			}

			if (hasRetParam) {
				# TODO: remove, as its unused; hasRetParam is never true
				print("\t" retParamType " " retNameTmp " = " fullName "(" paramListNoTypes ");") >> outFile_nc;
				print(retParamConversion) >> outFile_nc;
			} else {
				condRet = "";
				if (!isVoidRet) {
					if (retTypeConv) {
						condRet = "_retNative = ";
					} else {
						condRet = "_ret = (" jni_retType ") ";
					}
				}
				print("\t" condRet fullName "(" paramListNoTypes ");") >> outFile_nc;
			}

			# Params conversion - post call
			for (p=1; p <= size_params; p++) {
				pType_jni = jni_paramTypes[p];

				if (pType_jni == "jstring") {
					# jstring
					print("\t" "(*__env)->ReleaseStringUTFChars(__env, " jni_paramNames[p] ", " c_paramNames[p] ");") >> outFile_nc;
				} else if (match(pType_jni, /^j.+Array$/)) {
					# primitive arrray
					capArrType = pType_jni;
					sub(/^j/, "", capArrType);
					sub(/Array$/, "", capArrType);
					capArrType = capitalize(capArrType);

					_isPrimitive = (capArrType != "Object");
					_isString    = !_isPrimitive && (c_paramTypes[p] == "char**");

					print("\t" "if (" jni_paramNames[p] " != NULL) {") >> outFile_nc;
					if (_isPrimitive) {
						_elementJNativeType = jni_paramTypes[p];
						sub(/Array$/, "", _elementJNativeType); # jfloatArray -> jfloat
						print("\t\t" "(*__env)->Release" capArrType "ArrayElements(__env, " jni_paramNames[p] ", (" _elementJNativeType "*) " c_paramNames[p] ", 0 /* copy back changes and release */);") >> outFile_nc;
					} else if (_isString) {
						print("\t\t" "const int " c_paramNames[p] "_size = (int) (*__env)->GetArrayLength(__env, " jni_paramNames[p] ");") >> outFile_nc;
						print("\t\t" c_paramNames[p] " = (" c_paramTypes[p] ") calloc(sizeof(char*), " c_paramNames[p] "_size);") >> outFile_nc;
						print("\t\t" "int " c_paramNames[p] "_i;") >> outFile_nc;
						print("\t\t" "jstring " c_paramNames[p] "_jStr;") >> outFile_nc;
						print("\t\t" "for (" c_paramNames[p] "_i=0; " c_paramNames[p] "_i < " c_paramNames[p] "_size; ++" c_paramNames[p] "_i) {") >> outFile_nc;
						print("\t\t\t" c_paramNames[p] "_jStr = (jstring) (*__env)->GetObjectArrayElement(__env, " jni_paramNames[p] ", " c_paramNames[p] "_i);") >> outFile_nc;
						print("\t\t\t" "(*__env)->ReleaseStringUTFChars(__env, " c_paramNames[p] "_jStr, " c_paramNames[p] "[" c_paramNames[p] "_i]);") >> outFile_nc;
						print("\t\t" "}") >> outFile_nc;
						print("\t\t" "free(" c_paramNames[p] ");") >> outFile_nc;
						print("\t\t" c_paramNames[p] " = NULL;") >> outFile_nc;
					}
					print("\t" "}") >> outFile_nc;
				}
			}

			# Return value conversion - post call
			if (retType_isString) {
				print("\t" "_ret = (*__env)->NewStringUTF(__env, _retNative);") >> outFile_nc;
			}

			if (!isVoidRet) {
				print("") >> outFile_nc;
				print("\t" "return _ret;") >> outFile_nc;
			}
			print("" "}") >> outFile_nc;
			print("") >> outFile_nc;
		} else {
			print("Note: The following function is intentionally not wrapped: " fullName);
		}
	}


	print("#ifdef __cplusplus") >> outFile_nh;
	print("} // extern \"C\"") >> outFile_nh;
	print("#endif") >> outFile_nh;
	print("") >> outFile_nh;
	print("#endif // __CALLBACK_JNI_BRIDGE_H") >> outFile_nh;
	print("") >> outFile_nh;

	close(outFile_nh);
	close(outFile_nc);
}


function printHeader(outFile_h, javaPkg_h, javaClassName_h) {

	printCommentsHeader(outFile_h);
	print("") >> outFile_h;
	print("package " javaPkg_h ";") >> outFile_h;
	print("") >> outFile_h;
	print("") >> outFile_h;
	print("/**") >> outFile_h;
	print(" * Lets Java Skirmish AIs call back to the Spring engine.") >> outFile_h;
	print(" * We are using JNI for best speed.") >> outFile_h;
	print(" *") >> outFile_h;
	print(" * @author	AWK wrapper script") >> outFile_h;
	print(" * @version	GENERATED") >> outFile_h;
	print(" */") >> outFile_h;
	if (javaClassName_h == myClass) {
		print("public class " javaClassName_h " implements " myInterface " {") >> outFile_h;
	} else {
		print("public interface " javaClassName_h " {") >> outFile_h;
	}
	print("") >> outFile_h;
}

function createJavaFileName(clsName_f) {
	return JAVA_GENERATED_SOURCE_DIR "/" myPkgD "/" clsName_f ".java";
}

function printJavaClsAndInt() {

	outFile_i = createJavaFileName(myInterface);
	outFile_c = createJavaFileName(myClass);

	printHeader(outFile_i, myPkgA, myInterface);
	printHeader(outFile_c, myPkgA, myClass);

	# print the static registrator
	print("\tstatic {") >> outFile_c;
	print("\t\tSystem.loadLibrary(\"AIInterface\");") >> outFile_c;
	print("\t}") >> outFile_c;
	print("") >> outFile_c;

	# print skirmishAIId getter in interface
	print("\t" "public int SkirmishAI_getSkirmishAIId();") >> outFile_i;
	print("") >> outFile_i;

	# print skirmishAIId member, constructor and getter
	print("\t" "private int skirmishAIId;") >> outFile_c;
	print("") >> outFile_c;
	print("\t" "public " myClass "(int skirmishAIId) {") >> outFile_c;
	print("\t\t" "this.skirmishAIId = skirmishAIId;") >> outFile_c;
	print("\t}") >> outFile_c;
	print("") >> outFile_c;
	print("\t" "@Override") >> outFile_c;
	print("\t" "public int SkirmishAI_getSkirmishAIId() {") >> outFile_c;
	print("\t\t" "return this.skirmishAIId;") >> outFile_c;
	print("\t}") >> outFile_c;
	print("") >> outFile_c;

	# print the callback methods
	for (i=0; i < fi; i++) {
		if (doWrapp(i)) {
			fullName  = funcFullName[i];
			retType   = funcRetTypeJ[i];
			paramList = funcParamListJ[i];
			metaInf   = funcMetaInf[i];

			paramListNoSID = paramList;
			sub(/int _skirmishAIId(, )?/, "", paramListNoSID);
			paramListNoSIDNoTypes = removeParamTypes(paramListNoSID);
			if (paramListNoSIDNoTypes != "") {
				paramListNoSIDNoTypes = ", " paramListNoSIDNoTypes;
			}
			condRet = "";
			if (retType != "void") {
				condRet = "return ";
			}

			sub("^" bridgePrefix, "", fullName);

			metaInfCommand = "";
			if (metaInf != "") {
				metaInfCommand = " // " metaInf;
			}

			# print the interface function
			printFunctionComment_Common(outFile_i, funcDocComment, i, "\t");
			print("\t" "public " retType " " fullName "(" paramListNoSID ");" metaInfCommand) >> outFile_i;
			print("") >> outFile_i;

			# print the interface implementing function
			print("\t" "@Override") >> outFile_c;
			print("\t" "public " retType " " fullName "(" paramListNoSID ") {") >> outFile_c;
			print("\t\t" condRet "this." fullName "(this.skirmishAIId" paramListNoSIDNoTypes ");") >> outFile_c;
			print("\t" "}") >> outFile_c;

			# print the private native function
			print("\t" "private native " retType " " fullName "(" paramList ");") >> outFile_c;
			print("") >> outFile_c;
		}
	}

	print("}") >> outFile_i;
	print("") >> outFile_i;
	close(outFile_i);

	print("}") >> outFile_c;
	print("") >> outFile_c;
	close(outFile_c);
}


function wrappFunction(funcDef, commentEol) {

	doParse = 1;

	if (doParse) {
		size_funcParts = split(funcDef, funcParts, "(,)|(\\()|(\\))");
		# because the empty part after ");" would count as part as well
		size_funcParts--;
		retType_c = trim(funcParts[2]);
		retType_j = convertJNIToJavaType(convertCToJNIType(retType_c));
		fullName  = trim(funcParts[3]);

		# function parameters
		paramList_c = "";
		paramList_j = "";
		paramList = "";

		for (i=4; i<=size_funcParts && !match(funcParts[i], /.*\/\/.*/); i++) {
			type_c = extractParamType(funcParts[i]);
			type_c = cleanupCType(type_c);
			type_j = convertJNIToJavaType(convertCToJNIType(type_c));
			name   = extractParamName(funcParts[i]);
			if (i == 4) {
				cond_comma   = "";
			} else {
				cond_comma   = ", ";
			}
			paramList_c = paramList_c cond_comma type_c " " name;
			paramList_j = paramList_j cond_comma type_j " " name;
		}

		funcFullName[fi]   = fullName;
		funcRetTypeC[fi]   = retType_c;
		funcRetTypeJ[fi]   = retType_j;
		funcParamListC[fi] = paramList_c;
		funcParamListJ[fi] = paramList_j;
		funcMetaInf[fi]    = trim(commentEol);
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
		# separate possible comment at end of line: // fu bar
		commentEol = funcIntermLine;
		if (sub(/.*\/\//, "", commentEol)) {
			commentEolTot = commentEolTot commentEol;
		}
		# remove possible comment at end of line: // fu bar
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
/^EXPORT\(/ {

	funcStartLine = $0;
	# separate possible comment at end of line: // foo bar
	commentEolTot = "";
	commentEol = funcStartLine;
	if (sub(/.*\/\//, "", commentEol)) {
		commentEolTot = commentEolTot commentEol;
	}
	# remove possible comment at end of line: // fu bar
	sub(/\/\/.*$/, "", funcStartLine);
	funcStartLine = trim(funcStartLine);
	if (match(funcStartLine, /\;$/)) {
		# function ends in this line
		wrappFunction(funcStartLine, commentEolTot);
	} else {
		funcSoFar = funcStartLine;
		isMultiLineFunc = 1;
	}
}




END {
	# finalize things

	printNativeJNI();
	printJavaClsAndInt();
}
