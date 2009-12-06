#!/bin/awk
#
# This awk script creates a java class in OO style to wrapp the C style
# JNI based AI Events wrapper interface.
# In other words, the output of this file wrapps:
# com/springrts/ai/AI.java
# which wrapps:
# rts/ExternalAI/Interface/AISEvents.h
#
# This script uses functions from the following files:
# * common.awk
# * commonDoc.awk
# * commonOOCallback.awk
# Variables that can be set on the command-line (with -v):
# * GENERATED_SOURCE_DIR           : the generated sources root dir
# * JAVA_GENERATED_SOURCE_DIR      : the generated java sources root dir
# * INTERFACE_SOURCE_DIR           : the Java AI Interfaces static source files root dir
# * INTERFACE_GENERATED_SOURCE_DIR : the Java AI Interfaces generated source files root dir
#

BEGIN {
	# initialize things

	# define the field splitter(-regex)
	FS = "(\\()|(\\)\\;)";
	IGNORECASE = 0;

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
	if (!INTERFACE_SOURCE_DIR) {
		INTERFACE_SOURCE_DIR = "../../../Interfaces/Java/src/main/java";
	}
	if (!INTERFACE_GENERATED_SOURCE_DIR) {
		INTERFACE_GENERATED_SOURCE_DIR = "../../../Interfaces/Java/src-generated/main/java";
	}

	javaSrcRoot = "../src/main/java";

	myParentPkgA  = "com.springrts.ai";
	myMainPkgA    = myParentPkgA ".oo";
	myPkgClbA     = myMainPkgA ".clb";
	myPkgEvtA     = myMainPkgA ".evt";
	myMainPkgD    = convertJavaNameFormAToD(myMainPkgA);
	myPkgEvtD     = convertJavaNameFormAToD(myPkgEvtA);

	aiFloat3Class       = "AIFloat3";

	myOOAIClass         = "OOAI";
	myOOAIAbstractClass = "AbstractOOAI";
	myOOEventAIClass    = "OOEventAI";
	myOOAIFile          = JAVA_GENERATED_SOURCE_DIR "/" myMainPkgD "/" myOOAIClass ".java";
	myOOAIAbstractFile  = JAVA_GENERATED_SOURCE_DIR "/" myMainPkgD "/" myOOAIAbstractClass ".java";
	myOOEventAIFile     = JAVA_GENERATED_SOURCE_DIR "/" myMainPkgD "/" myOOEventAIClass ".java";

	printOOAIHeader(myOOAIFile, myOOAIClass);
	printOOAIHeader(myOOAIAbstractFile, myOOAIAbstractClass);
	printOOEventAIHeader(myOOEventAIFile);

	ind_evt = 0;
}



function printOOAIHeader(outFile, clsName) {

	printGeneratedWarningHeader(outFile);
	print("") >> outFile;
	printGPLHeader(outFile);
	print("") >> outFile;
	print("package " myMainPkgA ";") >> outFile;
	print("") >> outFile;
	print("") >> outFile;
	#print("import java.util.Properties;") >> outFile;
	print("import " myParentPkgA ".AI;") >> outFile;
	if (clsName == myOOAIClass) {
		print("import " myParentPkgA ".AICallback;") >> outFile;
		print("import " myMainPkgA ".clb.WrappOOAICallback;") >> outFile;
		print("import " myMainPkgA ".clb.WrappUnit;") >> outFile;
		print("import " myMainPkgA ".clb.WrappWeaponDef;") >> outFile;
	}
	print("import " myMainPkgA ".AIFloat3;") >> outFile;
	print("import " myMainPkgA ".clb.OOAICallback;") >> outFile;
	print("import " myMainPkgA ".clb.Unit;") >> outFile;
	print("import " myMainPkgA ".clb.WeaponDef;") >> outFile;
	print("") >> outFile;
	print("/**") >> outFile;
	print(" * TODO: Add description here") >> outFile;
	print(" *") >> outFile;
	print(" * @author	hoijui") >> outFile;
	print(" * @version	GENERATED") >> outFile;
	print(" */") >> outFile;

	_extends = "";
	if (clsName == myOOAIAbstractClass) {
		_extends = " extends " myOOAIClass;
	}
	print("public abstract class " clsName _extends " implements AI {") >> outFile;
	print("") >> outFile;

	if (clsName == myOOAIClass) {
		print("\t" "private AICallback   clb   = null;") >> outFile;
		print("\t" "private OOAICallback clbOO = null;") >> outFile;
		print("") >> outFile;
	}
}
function printOOAIEnd(outFile) {

	print("}") >> outFile;
	print("") >> outFile;
}

function printOOEventAIHeader(outFile) {

	printGeneratedWarningHeader(outFile);
	print("") >> outFile;
	printGPLHeader(outFile);
	print("") >> outFile;
	print("package " myMainPkgA ";") >> outFile;
	print("") >> outFile;
	print("") >> outFile;
	print("import " myParentPkgA ".AI;") >> outFile;
	print("import " myMainPkgA "." myOOAIClass ";") >> outFile;
	print("import " myPkgEvtA ".*;") >> outFile;
	print("import " myPkgClbA ".*;") >> outFile;
	#print("import " myPkgA ".AICallback;") >> outFile;
	#print("import java.util.Map;") >> outFile;
	#print("import java.util.HashMap;") >> outFile;
	#print("import java.util.Properties;") >> outFile;
	print("") >> outFile;
	print("/**") >> outFile;
	print(" * TODO: Add description here") >> outFile;
	print(" *") >> outFile;
	print(" * @author	hoijui") >> outFile;
	print(" * @version	GENERATED") >> outFile;
	print(" */") >> outFile;
	print("public abstract class " myOOEventAIClass " extends " myOOAIClass " implements AI {") >> outFile;
	print("") >> outFile;
	print("\t" "/**") >> outFile;
	print("\t" " * TODO: Add description here") >> outFile;
	print("\t" " *") >> outFile;
	print("\t" " * @param  event  the AI event to handle, sent by the engine") >> outFile;
	print("\t" " * @throws AIException") >> outFile;
	print("\t" " */") >> outFile;
	print("\t" "public abstract void handleEvent(AIEvent event) throws EventAIException;") >> outFile;
	print("") >> outFile;

	if (1 == 0) {
					print("") >> outFile;
					print("	@Override") >> outFile;
					print("	public int init(int teamId, AICallback callback) {") >> outFile;
					print("") >> outFile;
					print("		int ret = 0;") >> outFile;
					print("") >> outFile;
					print("		OOAICallback ooCallback = OOAICallback.getInstance(callback, teamId);") >> outFile;
					print("		OOAI ai = null;") >> outFile;
					print("		try {") >> outFile;
					print("			ai = createAI(teamId, ooCallback);") >> outFile;
					print("			if (ai == null) {") >> outFile;
					print("				ret = 1;") >> outFile;
					print("			}") >> outFile;
					print("		} catch (Throwable t) {") >> outFile;
					print("			ret = 2;") >> outFile;
					print("			t.printStackTrace();") >> outFile;
					print("		}") >> outFile;
					print("") >> outFile;
					print("		if (ret == 0) {") >> outFile;
					print("			ais.put(teamId, ai);") >> outFile;
					print("		}") >> outFile;
					print("") >> outFile;
					print("		return ret;") >> outFile;
					print("	}") >> outFile;
					print("") >> outFile;
	}
}
function printOOEventAIEnd(outFile) {

	print("}") >> outFile;
	print("") >> outFile;
}


function convertJavaSimpleTypeToOO(paType_sto, paName_sto,
		paTypeNew_sto, paNameNew_sto, convPre_sto, convPost_sto) {

	_change = 1;

	# uses this global vars:
	#paramTypeNew
	#paramNameNew
	#conversionCode_pre
	#conversionCode_post

	paramTypeNew = paType_sto;
	paramNameNew = paName_sto;

	if (match(paName_sto, /_posF3/)) {
		# convert float[3] to AIFloat3
		sub(/_posF3/, "", paramNameNew);
		paramTypeNew = "AIFloat3";
		conversionCode_pre = conversionCode_pre "\t\t" paramTypeNew " " paramNameNew " = new " paramTypeNew "(" paName_sto ");" "\n";
		#conversionCode_pre = conversionCode_pre "\t\t" paType_sto " " paName_sto " = " paNameNew_sto ".toFloatArray();" "\n";
	} else if (match(paName_sto, /_colorS3/)) {
		# convert short[3] to java.awt.Color
		sub(/_colorS3/, "", paramNameNew);
		paramTypeNew = "java.awt.Color";
		conversionCode_pre = conversionCode_pre "\t\t" paramTypeNew " " paramNameNew " = Util.toColor(" paName_sto ");" "\n";
		#conversionCode_pre = conversionCode_pre "\t\t" paType_sto " " paName_sto " = Util.toShort3Array(" paNameNew_sto ");" "\n";
	} else if ((paType_sto == "int") && match(paName_sto, /(unit|builder|attacker|enemy)(Id)?$/)) {
		# convert int to Unit
		sub(/Id$/, "", paramNameNew);
		paramNameNew = "oo_" paramNameNew;
		paramTypeNew = "Unit";
		conversionCode_pre = conversionCode_pre "\t\t" paramTypeNew " " paramNameNew " = Wrapp" paramTypeNew ".getInstance(this.clb, " paName_sto ");" "\n";
	} else if ((paType_sto == "int[]") && match(paName_sto, /unit(s|Ids)$/)) {
		# convert int[] to List<Unit>
		sub(/(Id)?s$/, "s", paramNameNew);
		paramNameNew = "oo_" paramNameNew;
		paramTypeNew = "java.util.List<Unit>";
		conversionCode_pre = conversionCode_pre "\t\t" paramTypeNew " " paramNameNew " = new java.util.ArrayList<Unit>();" "\n";
		conversionCode_pre = conversionCode_pre "\t\t" "for (int u=0; u < " paName_sto ".length; u++) {" "\n";
		conversionCode_pre = conversionCode_pre "\t\t\t" paramNameNew ".add(WrappUnit.getInstance(this.clb, " paName_sto "[u]));" "\n";
		conversionCode_pre = conversionCode_pre "\t\t" "}" "\n";
	} else if ((paType_sto == "int") && match(paName_sto, /(weaponDef)(Id)?$/)) {
		# convert int to WeaponDef
		sub(/Id$/, "", paramNameNew);
		paramNameNew = "oo_" paramNameNew;
		paramTypeNew = "WeaponDef";
		conversionCode_pre = conversionCode_pre "\t\t" paramTypeNew " " paramNameNew " = Wrapp" paramTypeNew ".getInstance(this.clb, " paName_sto ");" "\n";
	} else if (paType_sto == "AICallback") {
		# convert AICallback to OOAICallback
		paramNameNew = "oo_" paramNameNew;
		paramTypeNew = "OOAICallback";
		conversionCode_pre = conversionCode_pre "\t\t" "this.clb   = " paName_sto ";" "\n";
		conversionCode_pre = conversionCode_pre "\t\t" "this.clbOO = Wrapp" paramTypeNew ".getInstance(" paName_sto ");" "\n";
		conversionCode_pre = conversionCode_pre "\t\t" paramTypeNew " " paramNameNew " = this.clbOO;" "\n";
	} else {
		_change = 0;
	}

	return _change;
}


function printEventsOO() {

	c_size_cs = cls_id_name["*"];
	for (e=0; e < evts_size; e++) {
		printEventOO(e);
	}
}

function printEventOO(ind_evt_em) {

	retType_em = evts_retType[ind_evt_em];
	name_em    = evts_name[ind_evt_em];
	params_em  = evts_params[ind_evt_em];
	meta_em    = evts_meta[ind_evt_em];
print("meta_em: " meta_em);

	paramsList_size_em = split(params_em, paramsList_em, ", ");

	conversionCode_pre  = "";
	conversionCode_post = "";
	ooParams_em = "";

	for (p=1; p <= paramsList_size_em; p++) {
		_param = paramsList_em[p];
		_paramType = _param;
		sub(/ [^ ]*$/, "", _paramType);
		_paramName = _param;
		sub(/^[^ ]* /, "", _paramName);

		paramTypeNew = "";
		paramNameNew = "";
		convertJavaSimpleTypeToOO(_paramType, _paramName);

		if (!match(_paramName, /_size$/)) {
			ooParams_em = ooParams_em ", " paramTypeNew " " paramNameNew;
		}
	}
	sub(/^\, /, "", ooParams_em);

	if (1 == 0) {
				if (eNameLowerized == "playerCommand") {
					paramsTypes = "java.util.List<Unit> units, AICommand command, int playerId";
					paramsEvt = "units, command, evt.playerId";
				}
	}


	_equalMethod = (ooParams_em == params_em);
	_isVoid      = (retType_em == "void");
	if (_isVoid) {
		_condRet = "";
	} else {
		_condRet = "_ret = ";
	}

	print("") >> myOOAIFile;

	if (!_equalMethod) {
		ooParamNames_em = removeParamTypes(ooParams_em);
		print("\t" "@Override") >> myOOAIFile;
		print("\t" "public final " retType_em " " name_em "(" params_em ") {") >> myOOAIFile;
		print("") >> myOOAIFile;

		if (!_isVoid) {
			print("\t\t" retType_em " _ret;") >> myOOAIFile;
			print("") >> myOOAIFile;
		}
		if (conversionCode_pre != "") {
			print(conversionCode_pre) >> myOOAIFile;
		}

		print("\t\t" _condRet "this." name_em "(" ooParamNames_em ");") >> myOOAIFile;

		if (conversionCode_post != "") {
			print(conversionCode_post) >> myOOAIFile;
		}
		if (!_isVoid) {
			print("") >> myOOAIFile;
			print("\t\t" "return _ret;") >> myOOAIFile;
		}

		print("\t" "}") >> myOOAIFile;
	}

	printFunctionComment_Common(myOOAIFile, evts_docComment, ind_evt_em, "\t");
	if (_equalMethod) {
		print("\t" "@Override") >> myOOAIFile;
	}
	print("\t" "public abstract " retType_em " " name_em "(" ooParams_em ");") >> myOOAIFile;

	print("") >> myOOAIAbstractFile;
	printFunctionComment_Common(myOOAIAbstractFile, evts_docComment, ind_evt_em, "\t");
	print("\t" "@Override") >> myOOAIAbstractFile;
	ooParamsCleaned_em = ooParams_em;
	gsub(/ oo_/, " ", ooParamsCleaned_em);
	print("\t" "public " retType_em " " name_em "(" ooParamsCleaned_em ") {") >> myOOAIAbstractFile;
	print("\t\t" "return 0; // signaling: OK") >> myOOAIAbstractFile;
	print("\t" "}") >> myOOAIAbstractFile;

	printOOEventWrapper(retType_em, name_em, ooParamsCleaned_em, meta_em, ind_evt_em);
}

function printOOEventWrapper(retType_ei, mthName_ei, ooParams_ei, meta_ei, ind_evt_ei) {

	outFile = myOOEventAIFile;
	ooParamsNoTypes_ei = removeParamTypes(ooParams_ei);
	evtName_ei = capitalize(mthName_ei) "AIEvent";

	print("\t" "@Override") >> outFile;
	print("\t" "public final " retType_ei " " mthName_ei "(" ooParams_ei ") {") >> outFile;
	print("") >> outFile;
	print("\t\t" "AIEvent evt = new " evtName_ei "(" ooParamsNoTypes_ei ");") >> outFile;
	print("\t\t" "try {") >> outFile;
	print("\t\t\t" "this.handleEvent(evt);") >> outFile;
	print("\t\t\t" "return 0; // everything OK") >> outFile;
	print("\t\t" "} catch (EventAIException ex) {") >> outFile;
	print("\t\t\t" "return ex.getErrorNumber();") >> outFile;
	print("\t\t" "}") >> outFile;
	print("\t" "}") >> outFile;
	print("") >> outFile;

	printOOEventClass(retType_ei, evtName_ei, ooParams_ei, meta_ei, ind_evt_ei);
}

function printOOEventClass(retType_ec, evtName_ec, ooParams_ec, meta_ec, ind_evt_ec) {

	outFile = JAVA_GENERATED_SOURCE_DIR "/" myPkgEvtD "/" evtName_ec ".java";

	ooParamsList_size_ec = split(ooParams_ec, ooParamsList_ec, ", ");

	# print comments header
	printGeneratedWarningHeader(outFile);
	print("") >> outFile;
	printGPLHeader(outFile);
	print("") >> outFile;

	print("package " myPkgEvtA ";") >> outFile;
	print("") >> outFile;
	print("") >> outFile;

	# print imports
	print("import " myMainPkgA ".AIEvent;") >> outFile;
	print("import " myMainPkgA ".AIFloat3;") >> outFile;
	print("import " myPkgClbA ".*;") >> outFile;
	print("") >> outFile;

	# print class header
	printFunctionComment_Common(outFile, evts_docComment, ind_evt_ec, "");
	print("public class " evtName_ec " implements AIEvent {") >> outFile;
	print("") >> outFile;

	# print member vars
	for (_p=1; _p <= ooParamsList_size_ec; _p++) {
		print("\t" "private " ooParamsList_ec[_p] ";") >> outFile;
	}
	print("") >> outFile;

	print("\t" "public " evtName_ec "(" ooParams_ec ") {") >> outFile;
	print("") >> outFile;
	#print("\t\t" "AIEvent evt = new " evtName_ei "(" ooParamsNoTypes_ei ");") >> outFile;
	print("\t" "}") >> outFile;
	print("") >> outFile;

	# print member getters
	for (_p=1; _p <= ooParamsList_size_ec; _p++) {
		_type = extractParamType(ooParamsList_ec[_p]);
		_name = extractParamName(ooParamsList_ec[_p]);

		print("\t" "public " _type " get" capitalize(_name) "() {") >> outFile;
		print("\t\t" "return this." _name ";") >> outFile;
		print("\t" "}") >> outFile;
	}
	print("") >> outFile;

	print("}") >> outFile;
}


function printJavaEventHeader(javaFile) {

	printGeneratedWarningHeader(javaFile);
	print("") >> javaFile;
	printGPLHeader(javaFile);
	print("") >> javaFile;
	print("package " myPkgEvtA ";") >> javaFile;
	print("") >> javaFile;
	print("") >> javaFile;
	print("import " myPkgA ".*;") >> javaFile;
	print("") >> javaFile;
}

function printEventJava(evtIndex) {

	topicName = evtsTopicName[evtIndex];
	topicValue = evtsTopicNameValue[topicName];
	eName = evtsName[evtIndex];

	className = eName "AIEvent";
	javaFile = JAVA_GENERATED_SOURCE_DIR "/" myPkgEvtD "/" className ".java";
	evtInterface = "AIEvent";
	clsMods = "public final ";
	printJavaEventHeader(javaFile);

	if (eName == "Init") {
		printEventJavaCls(evtIndex);

		className = "Default" eName "AIEvent";
		javaFile = JAVA_GENERATED_SOURCE_DIR "/" myPkgEvtD "/" className ".java";
		printJavaEventHeader(javaFile);
		printEventJavaCls(evtIndex);
	} else {
		printEventJavaCls(evtIndex);
	}
	close(javaFile);
}

function printEventJavaCls(evtIndex) {

	printFunctionComment_Common(javaFile, evtsDocComment, evtIndex, "");

	print(clsMods "class " className " extends " evtInterface " {") >> javaFile;
	print("") >> javaFile;
	print("	public final static int TOPIC = " topicValue ";") >> javaFile;
	print("	public int getTopic() {") >> javaFile;
	print("		return " className ".TOPIC;") >> javaFile;
	print("	}") >> javaFile;
	print("") >> javaFile;

	print("	public " className "(Pointer memory) {") >> javaFile;
	print("") >> javaFile;
	if (className == "InitAIEvent") {
		print("		DefaultInitAIEvent initEvtImpl =  new DefaultInitAIEvent(memory);") >> javaFile;
		for (m=0; m < evtsNumMembers[evtIndex]; m++) {
			name = evtsMembers_name[evtIndex, m];
			if (name == "callback") {
				print("		this." name " = new DefaultAICallback();") >> javaFile;
			} else {
				print("		this." name " = initEvtImpl." name ";") >> javaFile;
			}
		}
	} else {
		if (evtsNumMembers[evtIndex] == 0) {
			print("		// JNA thinks a 0 size struct is an error,") >> javaFile;
			print("		// when it evaluates the size,") >> javaFile;
			print("		// so we set it manually to 1,") >> javaFile;
			print("		// because 0 would fail.") >> javaFile;
			print("		// This workaround is no longer possible sinze JNA 3.2.") >> javaFile;
			print("		// Therefore we require all structs to be non empty,") >> javaFile;
			print("		throw new RuntimeException(\"" className " error:.AI event structs have to be of size > 0 (ie. no empty stucts/no structs with 0 members)\");") >> javaFile;
		} else {
			print("		super(memory);") >> javaFile;
			print("		read();") >> javaFile;
		}
	}
	print("	}") >> javaFile;
	print("") >> javaFile;
	for (m=0; m < evtsNumMembers[evtIndex]; m++) {
		name = evtsMembers_name[evtIndex, m];
		type_c = evtsMembers_type_c[evtIndex, m];
		type_jna = convertCToJNAType(type_c);
		memMods = "public ";
		if ((name == "callback") && (className == "DefaultInitAIEvent")) {
			type_jna = "Pointer";
		}
		if (type_jna == "int[]") {
			type_jna = "Pointer";
		}
		print("	" memMods type_jna " " name ";") >> javaFile;
	}
	print("}") >> javaFile;
	print("") >> javaFile;
}



# This function has to return true (1) if a doc comment (eg: /** foo bar */)
# can be deleted.
# If there is no special condition you want to apply,
# it should always return true (1),
# cause there are additional mechanism to prevent accidential deleting.
# see: commonDoc.awk
function canDeleteDocumentation() {
	return 1;
}

################################################################################
### BEGINN: parsing and saving the event methods

# beginn of struct S*Event
/^\tpublic .+\);/ {

	_head   = $1;
	_params = $2;
	_tail   = $3;

	_retType = _head;
	sub(/^\tpublic /, "", _retType); # remove pre
	sub(/ .*$/, "", _retType);       # remove post

	_name = _head;
	sub(/^\tpublic [^ ]+ /, "", _name); # remove pre

	_meta = trim(_tail);
	sub(/^\/\/[ \t]*/, "", _meta); # remove pre

	evts_retType[ind_evt] = _retType;
	evts_name[ind_evt]    = _name;
	evts_params[ind_evt]  = _params;
	evts_meta[ind_evt]    = _meta;
	storeDocLines(evts_docComment, ind_evt);
#print(_retType " " _name "(" _params ") // " _meta);
	ind_evt++;
}

### END: parsing and saving the event methods
################################################################################




END {
	# finalize things

	evts_size = ind_evt;

	printEventsOO();

	printOOAIEnd(myOOAIFile);
	printOOAIEnd(myOOAIAbstractFile);
	printOOEventAIEnd(myOOEventAIFile);

	close(myOOAIFile);
	close(myOOAIAbstractFile);
	close(myOOEventAIFile);
}
