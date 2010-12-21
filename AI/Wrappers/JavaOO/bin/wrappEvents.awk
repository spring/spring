#!/usr/bin/awk -f
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

	myOOAIClass          = "OOAI";
	myOOAIInterface      = "I" myOOAIClass;
	myOOAIAbstractClass  = "AbstractOOAI";
	myOOEventAIClass     = "OOEventAI";
	myOOEventAIInterface = "I" myOOEventAIClass;
	myOOAIFile               = JAVA_GENERATED_SOURCE_DIR "/" myMainPkgD "/" myOOAIClass ".java";
	myOOAIInterfaceFile      = JAVA_GENERATED_SOURCE_DIR "/" myMainPkgD "/" myOOAIInterface ".java";
	myOOAIAbstractFile       = JAVA_GENERATED_SOURCE_DIR "/" myMainPkgD "/" myOOAIAbstractClass ".java";
	myOOEventAIFile          = JAVA_GENERATED_SOURCE_DIR "/" myMainPkgD "/" myOOEventAIClass ".java";
	myOOEventAIInterfaceFile = JAVA_GENERATED_SOURCE_DIR "/" myMainPkgD "/" myOOEventAIInterface ".java";

	printOOAIHeader(myOOAIFile, myOOAIClass);
	printOOAIHeader(myOOAIInterfaceFile, myOOAIInterface);
	printOOAIHeader(myOOAIAbstractFile, myOOAIAbstractClass);
	printOOEventAIHeader(myOOEventAIFile);
	printOOEventAIHeader(myOOEventAIInterfaceFile);

	ind_evt = 0;
}



function printOOAIHeader(outFile, clsName) {

	printCommentsHeader(outFile);
	print("") >> outFile;
	print("package " myMainPkgA ";") >> outFile;
	print("") >> outFile;
	print("") >> outFile;
	if (clsName != myOOAIInterface) {
		print("import " myParentPkgA ".AI;") >> outFile;
	}
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

	_type       = "abstract class";
	_extends    = "";
	_implements = " implements " myOOAIInterface ", AI";
	if (clsName == myOOAIAbstractClass) {
		_extends = " extends " myOOAIClass;
	}
	if (clsName == myOOAIInterface) {
		_type       = "interface";
		_implements = "";
	}
	print("public " _type " " clsName _extends _implements " {") >> outFile;
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

	printCommentsHeader(outFile);
	print("") >> outFile;
	print("package " myMainPkgA ";") >> outFile;
	print("") >> outFile;
	print("") >> outFile;
	print("import " myParentPkgA ".AI;") >> outFile;
	print("import " myMainPkgA "." myOOAIClass ";") >> outFile;
	print("import " myPkgEvtA ".*;") >> outFile;
	print("import " myPkgClbA ".*;") >> outFile;
	print("") >> outFile;
	print("/**") >> outFile;
	print(" * TODO: Add description here") >> outFile;
	print(" *") >> outFile;
	print(" * @author	hoijui") >> outFile;
	print(" * @version	GENERATED") >> outFile;
	print(" */") >> outFile;
	if (outFile == myOOEventAIFile) {
		print("public abstract class " myOOEventAIClass " extends " myOOAIClass " implements " myOOEventAIInterface ", AI {") >> outFile;
	} else {
		print("public interface " myOOEventAIInterface " {") >> outFile;
	}
	print("") >> outFile;

	print("\t" "/**") >> outFile;
	print("\t" " * TODO: Add description here") >> outFile;
	print("\t" " *") >> outFile;
	print("\t" " * @param  event  the AI event to handle, sent by the engine") >> outFile;
	print("\t" " * @throws AIException") >> outFile;
	print("\t" " */") >> outFile;
	_modifyer = "";
	if (outFile == myOOEventAIFile) {
		_modifyer = " abstract";
	}
	print("\t" "public" _modifyer " void handleEvent(AIEvent event) throws EventAIException;") >> outFile;
	print("") >> outFile;
}
function printOOEventAIEnd(outFile) {

	print("}") >> outFile;
	print("") >> outFile;
}


function convertJavaSimpleTypeToOO(paType_sto, paName_sto,
		paTypeNew_sto, paNameNew_sto, convPre_sto, convPost_sto) {

	_change = 1;

	# uses this global vars:
	# - paramTypeNew
	# - paramNameNew
	# - conversionCode_pre
	# - conversionCode_post

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

	# agarra te los event interfaces
	for (e=0; e < evts_size; e++) {
		meta_es    = evts_meta[e];

		interfList_size_es = 0;
		if (match(meta_es, /INTERFACES:/)) {
			interfMeta_es = meta_es;
			sub(/^.*INTERFACES:/, "", interfMeta_es);
			sub(/[ \t].*$/, "", interfMeta_es);
			interfList_size_es = split(interfMeta_es, interfList_es, /\)\,/);

			for (i=1; i <= interfList_size_es; i++) {
				_intName = interfList_es[i];
				sub(/\(.*$/, "", _intName);
				_intParams = interfList_es[i];
				sub(/^.*\(/, "", _intParams);
				sub(/\)$/,    "", _intParams);

				if (!(_intName in int_names)) {
					int_names[_intName] = _intParams;
				}
				evts_intNames[e      "," (i-1)] = _intName;
				evts_intParams[e     "," (i-1)] = _intParams;
			}
		}
		evts_intNames[e ",*"] = interfList_size_es;
	}

	# print the event classes
	for (e=0; e < evts_size; e++) {
		printEventOO(e);
	}

	# print the event interfaces
	for (intName in int_names) {
		printOOEventInterface(intName);
	}
}

function printEventOO(ind_evt_em) {

	retType_em  = evts_retType[ind_evt_em];
	name_em     = evts_name[ind_evt_em];
	params_em   = evts_params[ind_evt_em];
	meta_em     = evts_meta[ind_evt_em];

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
	print("\t" "@Override") >> myOOAIFile;
	print("\t" "public abstract " retType_em " " name_em "(" ooParams_em ");") >> myOOAIFile;

	ooParamsCleaned_em = ooParams_em;
	gsub(/ oo_/, " ", ooParamsCleaned_em);

	print("") >> myOOAIInterfaceFile;
	printFunctionComment_Common(myOOAIInterfaceFile, evts_docComment, ind_evt_em, "\t");
	print("\t" "public " retType_em " " name_em "(" ooParamsCleaned_em ");") >> myOOAIInterfaceFile;

	print("") >> myOOAIAbstractFile;
	printFunctionComment_Common(myOOAIAbstractFile, evts_docComment, ind_evt_em, "\t");
	print("\t" "@Override") >> myOOAIAbstractFile;
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
	int_size_ec = evts_intNames[ind_evt_ec ",*"];

	# list up interface parameters
	# clear arrays
	split("", intParams_toPrint_ec);
	split("", myIntParams_intParams_ec);
	split("", myIntParams_int_ec);
	for (_ii=0; _ii < int_size_ec; _ii++) {
		_name        = evts_intNames[ind_evt_ec "," _ii];
		_myIntParams = evts_intParams[ind_evt_ec "," _ii];
		_intParams   = int_names[_name];

		_myIntParamsList_size = split(_myIntParams, _myIntParamsList, ",");
		_intParamsList_size   = split(_intParams,   _intParamsList,   ",");

		for (_p=1; _p <= _intParamsList_size; _p++) {
			_myIntParam = _myIntParamsList[_p];
			_intParam   = _intParamsList[_p];

			intParams_toPrint_ec[_intParam]     = 1;
			myIntParams_int_ec[_myIntParam] = _name;
			if (_myIntParam != _intParam) {
				myIntParams_intParams_ec[_myIntParam] = _intParam;
			}
		}
	}

	_addIntLst = "";
	for (i=0; i < int_size_ec; i++) {
		_addIntLst = _addIntLst ", " evts_intNames[ind_evt_ec "," i] "AIEvent";
	}

	# print comments header
	printCommentsHeader(outFile);
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
	print("public class " evtName_ec " implements AIEvent" _addIntLst " {") >> outFile;
	print("") >> outFile;

	# print member vars
	for (_p=1; _p <= ooParamsList_size_ec; _p++) {
		print("\t" "private " ooParamsList_ec[_p] ";") >> outFile;
	}
	print("") >> outFile;

	# print constructor
	print("\t" "public " evtName_ec "(" ooParams_ec ") {") >> outFile;
	print("") >> outFile;
	for (_p=1; _p <= ooParamsList_size_ec; _p++) {
		_name = extractParamName(ooParamsList_ec[_p]);
		print("\t\t" "this." _name " = " _name ";") >> outFile;
	}
	print("\t" "}") >> outFile;
	print("") >> outFile;

	# print member getters
	for (_p=1; _p <= ooParamsList_size_ec; _p++) {
		_type = extractParamType(ooParamsList_ec[_p]);
		_name = extractParamName(ooParamsList_ec[_p]);

		# save the type for later writing of interfaces,
		# in case it is a ninterface param
		if (_name in intParams_toPrint_ec) {
			_intName = _name;
			if (_name in myIntParams_intParams_ec) {
				_intName = myIntParams_intParams_ec[_name];
			}
			_int     = myIntParams_int_ec[_name];

			int_name_param_type[_int "," _intName] = _type;
			# hacky
			int_name_param_type[_int "," _name]    = _type;
		}

		# print out @Override if this is an interface member
		if (_name in intParams_toPrint_ec && intParams_toPrint_ec[_name] == 1) {
			print("\t" "@Override") >> outFile;
			intParams_toPrint_ec[_name]   = 0;
		}

		print("\t" "public " _type " get" capitalize(_name) "() {") >> outFile;
		print("\t\t" "return this." _name ";") >> outFile;
		print("\t" "}") >> outFile;

		# print out an other getter if the interface param name is different
		if (_name in myIntParams_intParams_ec) {
			_intName = myIntParams_intParams_ec[_name];
			intParams_toPrint_ec[_intName] = 0;

			print("\t" "@Override") >> outFile;
			print("\t" "public " _type " get" capitalize(_intName) "() {") >> outFile;
			print("\t\t" "return this.get" capitalize(_name) "();") >> outFile;
			print("\t" "}") >> outFile;
		}

		print("") >> outFile;
	}

	print("}") >> outFile;

	close(outFile);
}

function printOOEventInterface(int_name_ei) {
	
	outFile = JAVA_GENERATED_SOURCE_DIR "/" myPkgEvtD "/" int_name_ei "AIEvent.java";

	int_params_ei = int_names[int_name_ei];

	int_paramsList_size_ei = split(int_params_ei, int_paramsList_ei, ",");
	int_size_ei = evts_intNames[ind_evt_ec ",*"];

	_addIntLst = "";
	for (i=0; i < int_size_ei; i++) {
		_addIntLst = _addIntLst ", " evts_intNames[ind_evt_ec "," i] "AIEvent";
	}

	# print comments header
	printCommentsHeader(outFile);
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
	print("public interface " int_name_ei "AIEvent extends AIEvent {") >> outFile;
	print("") >> outFile;

	# print getters
	for (_p=1; _p <= int_paramsList_size_ei; _p++) {
		_name = int_paramsList_ei[_p];
		_type = int_name_param_type[int_name_ei "," _name];

		print("\t" "public " _type " get" capitalize(_name) "();") >> outFile;
	}
	print("") >> outFile;

	print("}") >> outFile;
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

	_meta = $0;
	_hasMeta = sub(/.*\);[ \t]*\/\/[ \t]*/, "", _meta); # remove pre
	if (!_hasMeta) {
		_meta = "";
	}

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
	printOOAIEnd(myOOAIInterfaceFile);
	printOOAIEnd(myOOAIAbstractFile);
	printOOEventAIEnd(myOOEventAIFile);
	printOOEventAIEnd(myOOEventAIInterfaceFile);

	close(myOOAIFile);
	close(myOOAIInterfaceFile);
	close(myOOAIAbstractFile);
	close(myOOEventAIFile);
	close(myOOEventAIInterfaceFile);
}
