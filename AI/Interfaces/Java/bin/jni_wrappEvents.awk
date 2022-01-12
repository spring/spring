#!/usr/bin/awk -f
#
# This awk script creates a Java class with one function per event,
# plus a C wrapper to easily/comfortably call these functions from C.
# input is taken from file:
# rts/ExternalAI/Interface/AISEvents.h
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
	FS = "[ \t]+";

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

	myWrapperName   = "EventsJNIBridge";
	myWrapperPrefix = "eventsJniBridge_";

	myPkgA = "com.springrts.ai";
	myPkgD = convertJavaNameFormAToD(myPkgA);
	myAIClass         = "AI";
	myAIAbstractClass = "AbstractAI";
	myAICallbackInt   = "AICallback";

	ind_evtTopics = 0;
	ind_evtStructs = 0;
	isInsideEvtStruct = 0;
}



# Checks if a field is available and is no comment
function isFieldUsable(f) {

	valid = 0;

	if (f && !match(f, /.*\/\/.*/)) {
		valid = 1;
	}

	return valid;
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
function createJavaFileName(fileName_fn) {
	return JAVA_GENERATED_SOURCE_DIR "/" myPkgD "/" fileName_fn ".java";
}


function printNativeHeader() {

	outFile_nh = createNativeFileName(myWrapperName, 1);
	outFile_nc = createNativeFileName(myWrapperName, 0);
	printCommentsHeader(outFile_nh);
	printCommentsHeader(outFile_nc);

	# print includes (header)
	print("") >> outFile_nh;
	print("#ifndef __EVENTS_JNI_BRIDGE_H") >> outFile_nh;
	print("#define __EVENTS_JNI_BRIDGE_H") >> outFile_nh;
	print("") >> outFile_nh;
	print("#include <jni.h>") >> outFile_nh;
	print("") >> outFile_nh;
	print("#ifdef __cplusplus") >> outFile_nh;
	print("extern \"C\" {") >> outFile_nh;
	print("#endif") >> outFile_nh;
	print("") >> outFile_nh;

	# print includes (source)
	print("") >> outFile_nc;
	print("#include \"" myWrapperName ".h\"") >> outFile_nc;
	print("") >> outFile_nc;
	print("#include \"JavaBridge.h\" // for INT_AI") >> outFile_nc;
	print("#include \"JniUtil.h\"") >> outFile_nc;
	print("#include \"ExternalAI/Interface/AISEvents.h\"") >> outFile_nc;
	print("#include <stdlib.h> // for calloc(), free()") >> outFile_nc;
	print("") >> outFile_nc;
	print("") >> outFile_nc;

	# print global vars (source)
	print("size_t   skirmishAIId_size     = 0;") >> outFile_nc;
	print("jobject* skirmishAIId_callback = NULL;") >> outFile_nc;
	print("") >> outFile_nc;
}

function printNativeEventMethodVars() {

	for (e=0; e < ind_evtStructs; e++) {
		printNativeEventMethodVar(e);
	}
	print("") >> outFile_nc;
}
function printNativeEventMethodVar(evtIndex) {

	outFile_nc = createNativeFileName(myWrapperName, 0);

	eName          = evtsName[evtIndex];
	eNameLowerized = lowerize(eName);

	print("jmethodID m_ai_" eNameLowerized " = NULL;") >> outFile_nc;
}

function printNativeStaticInitFuncHead() {

	# ... header
	print("/**") >> outFile_nh;
	print(" * Initialized stuff needed for sending events to an AI library instance.") >> outFile_nh;
	print(" *") >> outFile_nh;
	print(" * @author	hoijui") >> outFile_nh;
	print(" * @version	GENERATED") >> outFile_nh;
	print(" */") >> outFile_nh;
	print("int " myWrapperPrefix "initStatic(JNIEnv* env, size_t skirmishAIId_size);") >> outFile_nh;
	print("") >> outFile_nh;
	# ... source
	print("int " myWrapperPrefix "initStatic(JNIEnv* env, size_t _skirmishAIId_size) {") >> outFile_nc;
	print("") >> outFile_nc;
	print("\t" "skirmishAIId_size = _skirmishAIId_size;") >> outFile_nc;
	print("\t" "skirmishAIId_callback = (jobject*) calloc(skirmishAIId_size, sizeof(jobject));") >> outFile_nc;
	print("\t" "size_t t;") >> outFile_nc;
	print("\t" "for (t=0; t < skirmishAIId_size; ++t) {") >> outFile_nc;
	print("\t\t" "skirmishAIId_callback[t] = NULL;") >> outFile_nc;
	print("\t" "}") >> outFile_nc;
	print("") >> outFile_nc;
	print("\t" "jobject c_aiInt = (*env)->FindClass(env, \"" myPkgD "/" myAIClass "\");") >> outFile_nc;
	print("\t" "if (jniUtil_checkException(env, \"Failed fetching AI interface class " myPkgA "." myAIClass "\")) { return -2; }") >> outFile_nc;
	print("") >> outFile_nc;
}

function printNativeEventStaticInits() {

	for (e=0; e < ind_evtStructs; e++) {
		printNativeEventStaticInit(e);
	}
	print("") >> outFile_nc;
}
function printNativeEventStaticInit(evtIndex) {

	outFile_nc = createNativeFileName(myWrapperName, 0);

	eName          = evtsName[evtIndex];
	eNameLowerized = lowerize(eName);
	eSignature     = "(";
	for (m=0; m < evtsNumMembers[evtIndex]; m++) {
		type_c     = evtsMembers_type_c[evtIndex, m];
		type_jni   = convertCToJNIType(type_c);
		type_sig   = convertJNIToSignatureType(type_jni);
		eSignature = eSignature type_sig;
	}
	eSignature = eSignature ")I";
	if (eNameLowerized == "init") {
		eSignature = "(IL" myPkgD "/" myAICallbackInt ";)I";
	}

	print("\t" "m_ai_" eNameLowerized " = jniUtil_getMethodID(env, c_aiInt, \"" eNameLowerized "\", \"" eSignature "\");") >> outFile_nc;
	print("\t" "if (jniUtil_checkException(env, \"Failed fetching Java AI method ID for: " eNameLowerized "\")) { return -3; }") >> outFile_nc;
	print("") >> outFile_nc;
}

function printNativeInitFunc() {

	print("\t" "return 0; // -> no error") >> outFile_nc;
	print("}") >> outFile_nc;
	print("") >> outFile_nc;

	# ... header
	print("/**") >> outFile_nh;
	print(" * Initialized stuff needed for an AI instance.") >> outFile_nh;
	print(" *") >> outFile_nh;
	print(" * @author	hoijui") >> outFile_nh;
	print(" * @version	GENERATED") >> outFile_nh;
	print(" */") >> outFile_nh;
	print("int " myWrapperPrefix "initAI(JNIEnv* env, int skirmishAIId, jobject callback);") >> outFile_nh;
	print("") >> outFile_nh;
	# ... source
	print("int " myWrapperPrefix "initAI(JNIEnv* env, int skirmishAIId, jobject callback) {") >> outFile_nc;
	print("") >> outFile_nc;
	print("\t" "int res = -1;") >> outFile_nc;
	print("") >> outFile_nc;
	print("\t" "skirmishAIId_callback[skirmishAIId] = callback;") >> outFile_nc;
	print("\t" "res = 0;") >> outFile_nc;
	print("") >> outFile_nc;
	print("\t" "return res;") >> outFile_nc;
	print("}") >> outFile_nc;
	print("") >> outFile_nc;
}

function printNativeHandleFuncHead() {

	# ... header
	print("/**") >> outFile_nh;
	print(" * For documentation, see SSkirmishAILibrary::handleEvent() in:") >> outFile_nh;
	print(" * ExternalAI/Interface/SSkirmishAILibrary.h") >> outFile_nh;
	print(" *") >> outFile_nh;
	print(" * @author	hoijui") >> outFile_nh;
	print(" * @version	GENERATED") >> outFile_nh;
	print(" */") >> outFile_nh;
	print("int " myWrapperPrefix "handleEvent(JNIEnv* env, jobject aiInstance, int skirmishAIId, int topic, const void* data);") >> outFile_nh;
	print("") >> outFile_nh;
	# ... source
	print("int " myWrapperPrefix "handleEvent(JNIEnv* env, jobject aiInstance, int skirmishAIId, int topic, const void* data) {") >> outFile_nc;
	print("") >> outFile_nc;
	print("\t" "int _ret = -1;") >> outFile_nc;
	print("") >> outFile_nc;
#	print("\t" "jobject o_ai = skirmishAIId_aiObject[skirmishAIId];") >> outFile_nc;
#	print("\t" "//assert(o_ai != NULL);") >> outFile_nc;
#	print("") >> outFile_nc;

	# only add this for debugging purposes
#	print("\t" "// in case we missed handling an exception elsewhere") >> outFile_nc;
#	print("\t" "// and it is therefore still pending, we handle it here") >> outFile_nc;
#	print("\t" "if ((*env)->ExceptionCheck(env)) {") >> outFile_nc;
#	print("\t\t" "(*env)->ExceptionDescribe(env);") >> outFile_nc;
#	print("\t\t" "_ret = -6;") >> outFile_nc;
#	print("\t\t" "return _ret;") >> outFile_nc;
#	print("\t" "}") >> outFile_nc;
#	print("") >> outFile_nc;

	print("\t" "switch (topic) {") >> outFile_nc;
}


function printNativeEventCases() {

	for (e=0; e < ind_evtStructs; e++) {
		printNativeEventCase(e);
	}
}
function printNativeEventCase(evtIndex) {

	outFile_nc = createNativeFileName(myWrapperName, 0);

	topicName      = evtsTopicName[evtIndex];
	topicValue     = evtsTopicNameValue[topicName];
	eName          = evtsName[evtIndex];
	eNameLowerized = lowerize(eName);
	eStruct        = "S" eName "Event";

	print("\t\t" "case " topicName ": {") >> outFile_nc;
	print("\t\t\t" "const struct " eStruct "* evt = (const struct "eStruct"*) data;") >> outFile_nc;

	paramsEvt       = "";
	conversion_pre  = "";
	for (m=0; m < evtsNumMembers[evtIndex]; m++) {
		type_c   = evtsMembers_type_c[evtIndex, m];
		name_c   = evtsMembers_name[evtIndex, m];
		value_c  = "evt->" name_c;
		name_jni = value_c;

		type_jni = convertCToJNIType(type_c);
		if (type_jni == "jstring") {
			name_jni = name_c "_jni";
			conversion_pre  = conversion_pre  "\n\t\t\t" type_jni " " name_jni " = (*env)->NewStringUTF(env, " value_c ");";
		} else if (type_jni == "jfloatArray") {
			name_jni = name_c "_jni";

			array_size = "sizeof(" value_c ")";
			if (match(name_c, /_posF3$/)) {
				array_size = "3";
			}

			conversion_pre  = conversion_pre  "\n\t\t\t" type_jni " " name_jni " = (*env)->NewFloatArray(env, " array_size ");";
			conversion_pre  = conversion_pre  "\n\t\t\t" "(*env)->SetFloatArrayRegion(env, " name_jni ", 0, " array_size ", " value_c ");";
		} else if (type_jni == "jintArray") {
			name_jni = name_c "_jni";

			array_size = "sizeof(" value_c ")";

			conversion_pre  = conversion_pre  "\n\t\t\t" type_jni " " name_jni " = (*env)->NewIntArray(env, " array_size ");";
			conversion_pre  = conversion_pre  "\n\t\t\t" "(*env)->SetIntArrayRegion(env, " name_jni ", 0, " array_size ", " value_c ");";
		}

		paramsEvt = paramsEvt ", " name_jni;
	}
	sub(/^, /, "", paramsEvt);
	if (eNameLowerized == "init") {
		sub(/evt->callback/, "skirmishAIId_callback[evt->skirmishAIId]", paramsEvt);
	}

	print(conversion_pre) >> outFile_nc;
	print("\t\t\t" "_ret = (*env)->CallIntMethod(env, aiInstance, m_ai_" eNameLowerized ", " paramsEvt ");") >> outFile_nc;

	print("\t\t\t" "break;") >> outFile_nc;
	print("\t\t" "}") >> outFile_nc;
}

function printNativeEnd() {

	outFile_nh = createNativeFileName(myWrapperName, 1);
	outFile_nc = createNativeFileName(myWrapperName, 0);

	print("#ifdef __cplusplus") >> outFile_nh;
	print("} // extern \"C\"") >> outFile_nh;
	print("#endif") >> outFile_nh;
	print("") >> outFile_nh;
	print("#endif // __EVENTS_JNI_BRIDGE_H") >> outFile_nh;
	print("") >> outFile_nh;

	print("\t\t" "default: {") >> outFile_nc;
	print("\t\t\t" "_ret = -4;") >> outFile_nc;
	print("\t\t\t" "break;") >> outFile_nc;
	print("\t\t" "}") >> outFile_nc;
	print("\t" "}") >> outFile_nc;
	print("") >> outFile_nc;
	print("\t" "if ((*env)->ExceptionCheck(env)) {") >> outFile_nc;
	print("\t\t" "(*env)->ExceptionDescribe(env);") >> outFile_nc;
	print("\t\t" "_ret = -5;") >> outFile_nc;
	print("\t" "}") >> outFile_nc;
	print("") >> outFile_nc;
	print("\t" "return _ret;") >> outFile_nc;
	print("}") >> outFile_nc;

	close(outFile_nh);
	close(outFile_nc);
}


function printGeneralJavaHeader(outFile_h, javaPkg_h, javaClassName_h) {

	printCommentsHeader(outFile_h);
	print("") >> outFile_h;
	print("package " javaPkg_h ";") >> outFile_h;
	print("") >> outFile_h;
	print("") >> outFile_h;
	print("/**") >> outFile_h;
	print(" * This is the Java entry point from events comming from the engine.") >> outFile_h;
	print(" * We are using JNI for best in speed.") >> outFile_h;
	print(" *") >> outFile_h;
	print(" * @author	AWK wrapper script") >> outFile_h;
	print(" * @version	GENERATED") >> outFile_h;
	print(" */") >> outFile_h;
	if (javaClassName_h == myAIAbstractClass) {
		print("public abstract class " javaClassName_h " implements " myAIClass " {") >> outFile_h;
	} else {
		print("public interface " javaClassName_h " {") >> outFile_h;
	}
	print("") >> outFile_h;
}

function printJavaHeader() {

	outFile_i = createJavaFileName(myAIClass);
	outFile_a = createJavaFileName(myAIAbstractClass);

	printGeneralJavaHeader(outFile_i, myPkgA, myAIClass);
	printGeneralJavaHeader(outFile_a, myPkgA, myAIAbstractClass);
}

function printJavaEvents() {

	for (e=0; e < ind_evtStructs; e++) {
		printJavaEvent(e);
	}
}
function printJavaEvent(evtIndex) {

	outFile_i = createJavaFileName(myAIClass);
	outFile_a = createJavaFileName(myAIAbstractClass);

	topicName      = evtsTopicName[evtIndex];
	topicValue     = evtsTopicNameValue[topicName];
	eName          = evtsName[evtIndex];
	eMetaComment   = evtsMetaComment[evtIndex];
	eNameLowerized = lowerize(eName);
	eCls           = eName "AIEvent";

	# Add the member comments to the main comment as @param attributes
	numEvtDocLines = evtsDocComment[evtIndex, "*"];
	for (m=firstMember; m < evtsNumMembers[evtIndex]; m++) {
		numLines = evtMbrsDocComments[evtIndex*1000 + m, "*"];
		if (numLines > 0) {
			evtsDocComment[evtIndex, numEvtDocLines] = "@param " evtsMembers_name[evtIndex, m];
		}
		for (l=0; l < numLines; l++) {
			if (l == 0) {
				_preDocLine = "@param " evtsMembers_name[evtIndex, m] "  ";
			} else {
				_preDocLine = "       " lengtAsSpaces(evtsMembers_name[evtIndex, m]) "  ";
			}
			evtsDocComment[evtIndex, numEvtDocLines] = _preDocLine evtMbrsDocComments[evtIndex*1000 + m, l];
			numEvtDocLines++;
		}
	}
	evtsDocComment[evtIndex, "*"] = numEvtDocLines;

	paramsTypes = "";
	for (m=0; m < evtsNumMembers[evtIndex]; m++) {
		name_c    = evtsMembers_name[evtIndex, m];
		type_c    = evtsMembers_type_c[evtIndex, m];
		type_java = convertJNIToJavaType(convertCToJNIType(type_c));

		paramsTypes = paramsTypes ", " type_java " " name_c;
	}
	sub(/^, /, "", paramsTypes);
	if (eNameLowerized == "init") {
		paramsTypes = "int skirmishAIId, AICallback callback";
	}

	_condMetaComments = eMetaComment;
	if (_condMetaComments != "") {
		_condMetaComments = " // " _condMetaComments;
	}

	print("") >> outFile_i;
	printFunctionComment_Common(outFile_i, evtsDocComment, evtIndex, "\t");
	print("\t" "public int " eNameLowerized "(" paramsTypes ");" _condMetaComments) >> outFile_i;

	print("") >> outFile_a;
	print("\t" "@Override") >> outFile_a;
	print("\t" "public int " eNameLowerized "(" paramsTypes ") {") >> outFile_a;
	print("") >> outFile_a;
	print("\t\t" "// signal: event handled OK") >> outFile_a;
	print("\t\t" "return 0;") >> outFile_a;
	print("\t" "}") >> outFile_a;
}

function printJavaEnd() {

	outFile_i = createJavaFileName(myAIClass);
	outFile_a = createJavaFileName(myAIAbstractClass);

	print("}") >> outFile_i;
	print("") >> outFile_i;

	print("}") >> outFile_a;
	print("") >> outFile_a;

	close(outFile_i);
	close(outFile_a);
}


function saveMember(ind_mem_s, member_s) {

	name_s = extractParamName(member_s);
	type_c_s = extractCType(member_s);

	evtsMembers_name[ind_evtStructs, ind_mem_s] = name_s;
	evtsMembers_type_c[ind_evtStructs, ind_mem_s] = type_c_s;
}


# aggare te los event defines in order
/^[ \t]*EVENT_.*$/ {

	doWrapp = !match(($0), /.*EVENT_NULL.*/) && !match(($0), /.*EVENT_TO_ID_ENGINE.*/);
	if (doWrapp) {
		sub(",", "", $4);
		evtsTopicNameValue[$2] = $4;
	} else {
		print("Java-AIInterface: NOTE: JNI level: Events: intentionally not wrapped: " $2);
	}
}


# This function has to return true (1) if a doc comment (eg: /** foo bar */)
# can be deleted.
# If there is no special condition you want to apply,
# it should always return true (1),
# cause there are additional mechanism to prevent accidential deleting.
# see: commonDoc.awk
function canDeleteDocumentation() {
	return isInsideEvtStruct != 1;
}

################################################################################
### BEGINN: parsing and saving the event structs

# end of struct S*Event
/^}; \/\/\$ EVENT_/ {

	evtsNumMembers[ind_evtStructs]  = ind_evtMember;
	evtsTopicName[ind_evtStructs]   = $3;
	_metaComment = $0;
	sub("^.*" evtsTopicName[ind_evtStructs], "", _metaComment);
	evtsMetaComment[ind_evtStructs] = trim(_metaComment);

	ind_evtStructs++;
	isInsideEvtStruct = 0;
}


# inside of struct S*Event
{
	if (isInsideEvtStruct == 1) {
		size_tmpMembers = split($0, tmpMembers, ";");
		# cause there is an empty part behind the ';'
		size_tmpMembers--;
		for (i=1; i<=size_tmpMembers; i++) {
			tmpMembers[i] = trim(tmpMembers[i]);
			if (tmpMembers[i] == "" || match(tmpMembers[i], /^\/\//)) {
				break;
			}
			# This would bork with more then 1000 members in an event,
			# or more then 1000 events
			storeDocLines(evtMbrsDocComments, ind_evtStructs*1000 + ind_evtMember);
			saveMember(ind_evtMember, tmpMembers[i]);
			ind_evtMember++;
		}
	}
}

# beginn of struct S*Event
/^struct S.*Event( \{)?/ {

	isInsideEvtStruct = 1;
	ind_evtMember = 0;
	eventName = $2;
	sub(/^S/, "", eventName);
	sub(/Event$/, "", eventName);

	evtsName[ind_evtStructs] = eventName;
	storeDocLines(evtsDocComment, ind_evtStructs);
}

### END: parsing and saving the event structs
################################################################################




END {
	# finalize things
	printNativeHeader();
	printNativeEventMethodVars();
	printNativeStaticInitFuncHead();
	printNativeEventStaticInits();
	printNativeInitFunc();
	printNativeHandleFuncHead();
	printNativeEventCases();
	printNativeEnd();

	printJavaHeader();
	printJavaEvents();
	printJavaEnd();
}
