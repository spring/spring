#!/bin/awk
#
# This awk script creates the JNA wrapper classes for the C event structs in:
# rts/ExternalAI/Interface/AISEvents.h
#
# this script uses functions from common.awk, use like this:
# 	awk -f thisScript.awk -f common.awk [additional-params]
#

BEGIN {
	# initialize things

	# define the field splitter(-regex)
	FS = "[ \t]+";

	javaSrcRoot = "../java/src";

	myPkgA = "com.clan_sy.spring.ai";
	aiFloat3Class = "AIFloat3";
	myPkgD = convertJavaNameFormAToD(myPkgA);

	myPkgEvtA = myPkgA ".event";
	myPkgEvtD = convertJavaNameFormAToD(myPkgEvtA);

	myPkgOOA = myPkgA ".oo";
	myPkgOOD = convertJavaNameFormAToD(myPkgOOA);
	myOOAIClass = "OOAI";
	myOOAIAbstractClass = "AbstractOOAI";
	myOOAIFactoryClass = "OOAIFactory";
	myOOAIFile = javaSrcRoot "/" myPkgOOD "/" myOOAIClass ".java";
	myOOAIAbstractFile = javaSrcRoot "/" myPkgOOD "/" myOOAIAbstractClass ".java";
	myOOAIFactoryFile = javaSrcRoot "/" myPkgOOD "/" myOOAIFactoryClass ".java";

	printOOAIHeader(myOOAIFile, myOOAIClass);
	printOOAIHeader(myOOAIAbstractFile, myOOAIAbstractClass);
	printOOAIFactoryHeader(myOOAIFactoryFile);

	ind_evtTopics = 0;
	ind_evtStructs = 0;
	insideEvtStruct = 0;
}




# Checks if a field is available and is no comment
function isFieldUsable(f) {
	
	valid = 0;

	if (f && !match(f, /.*\/\/.*/)) {
		valid = 1;
	}

	return valid;
}



function printOOAIHeader(outFile, clsName) {

	printGeneratedWarningHeader(outFile);
	print("") >> outFile;
	printGPLHeader(outFile);
	print("") >> outFile;
	print("package " myPkgOOA ";") >> outFile;
	print("") >> outFile;
	print("") >> outFile;
	print("import java.util.Properties;") >> outFile;
	print("import com.clan_sy.spring.ai.AICommand;") >> outFile;
	print("import com.clan_sy.spring.ai.AIFloat3;") >> outFile;
	print("") >> outFile;
	print("/**") >> outFile;
	print(" *") >> outFile;
	print(" *") >> outFile;
	print(" * @author	hoijui") >> outFile;
	print(" * @version	GENERATED") >> outFile;
	print(" */") >> outFile;
	if (clsName == myOOAIAbstractClass) {
		print("public abstract class " clsName " implements " myOOAIClass " {") >> outFile;
	} else {
		print("public interface " clsName " {") >> outFile;
	}
	print("") >> outFile;
}
function printOOAIEnd(outFile) {

	print("}") >> outFile;
	print("") >> outFile;
}

function printOOAIFactoryHeader(outFile) {

	printGeneratedWarningHeader(outFile);
	print("") >> outFile;
	printGPLHeader(outFile);
	print("") >> outFile;
	print("package " myPkgOOA ";") >> outFile;
	print("") >> outFile;
	print("") >> outFile;
	print("import com.clan_sy.spring.ai.AI;") >> outFile;
	print("import com.clan_sy.spring.ai.AICommand;") >> outFile;
	print("import com.clan_sy.spring.ai.AICommandWrapper;") >> outFile;
	print("import com.clan_sy.spring.ai.event.*;") >> outFile;
	print("import com.sun.jna.Pointer;") >> outFile;
	print("import java.util.Map;") >> outFile;
	print("import java.util.HashMap;") >> outFile;
	print("import java.util.Properties;") >> outFile;
	print("") >> outFile;
	print("/**") >> outFile;
	print(" *") >> outFile;
	print(" *") >> outFile;
	print(" * @author	hoijui") >> outFile;
	print(" * @version	GENERATED") >> outFile;
	print(" */") >> outFile;
	print("public abstract class OOAIFactory implements AI {") >> outFile;
	print("") >> outFile;
	print("	private Map<Integer, OOAI> ais = new HashMap<Integer, OOAI>();") >> outFile;
	print("	private Map<Integer, OOAICallback> ooClbs = new HashMap<Integer, OOAICallback>();") >> outFile;
	print("") >> outFile;
	print("	public final int init(int teamId, Properties info, Properties options) {") >> outFile;
	print("") >> outFile;
	print("		OOAI ai = createAI(teamId, info, options);") >> outFile;
	print("		if (ai != null) {") >> outFile;
	print("			ais.put(teamId, ai);") >> outFile;
	print("		}") >> outFile;
	print("		return (ai == null) ? -1 : 0;") >> outFile;
	print("	}") >> outFile;
	print("") >> outFile;
	print("	public final int release(int teamId) {") >> outFile;
	print("") >> outFile;
	print("		OOAI ai = ais.remove(teamId);") >> outFile;
	print("		ooClbs.remove(teamId);") >> outFile;
	print("		return (ai == null) ? -1 : 0;") >> outFile;
	print("	}") >> outFile;
	print("") >> outFile;
	print("	public final int handleEvent(int teamId, int topic, Pointer event) {") >> outFile;
	print("") >> outFile;
	print("		int _ret = -1;") >> outFile;
	print("") >> outFile;
	print("		OOAI ai = ais.get(teamId);") >> outFile;
	print("		OOAICallback ooClb = ooClbs.get(teamId);") >> outFile;
	print("") >> outFile;
	print("		if (ai != null) {") >> outFile;
	print("			try {") >> outFile;
	print("				switch (topic) {") >> outFile;
	print("") >> outFile;
}
function printOOAIFactoryEnd(outFile) {

	print("					default:") >> outFile;
	print("						_ret = 1;") >> outFile;
	print("				}") >> outFile;
	print("			} catch (Throwable t) {") >> outFile;
	print("				_ret = 2;") >> outFile;
	print("			}") >> outFile;
	print("		}") >> outFile;
	print("") >> outFile;
	print("		return _ret;") >> outFile;
	print("	}") >> outFile;
	print("") >> outFile;
	print("	public abstract OOAI createAI(int teamId, Properties info, Properties options);") >> outFile;
	print("}") >> outFile;
	print("") >> outFile;
}


function printEventOO(evtIndex) {

	topicName = evtsTopicName[evtIndex];
	topicValue = evtsTopicNameValue[topicName];
	eName = evtsName[evtIndex];
	eNameLowerized = lowerize(eName);
	eCls = eName "AIEvent";

	paramsTypes = "";
	paramsEvt = "";
	for (m=0; m < evtsNumMembers[evtIndex]; m++) {
		name = evtsMembers_name[evtIndex, m];
		type_c = evtsMembers_type_c[evtIndex, m];
		type_jna = convertCToJNAType(type_c);
		
		paramsTypes = paramsTypes ", " type_jna " " name;
		paramsEvt = paramsEvt ", evt." name;
	}
	sub(/^\, /, "", paramsTypes);
	sub(/^\, /, "", paramsEvt);

	sub(/int unit(Id)?/, "Unit unit", paramsTypes);
	sub(/int attacker(Id)?/, "Unit attacker", paramsTypes);
	sub(/int enemy(Id)?/, "Unit enemy", paramsTypes);
	sub(/int weaponDef(Id)?/, "WeaponDef weaponDef", paramsTypes);

	unitRepls = sub(/evt.unitId/, "Unit.getInstance(ooClb, evt.unitId)", paramsEvt);
	if (unitRepls == 0) {
		sub(/evt.unit/, "Unit.getInstance(ooClb, evt.unit)", paramsEvt);
	}
	sub(/evt.attacker/, "Unit.getInstance(ooClb, evt.attacker)", paramsEvt);
	sub(/evt.enemy/, "Unit.getInstance(ooClb, evt.enemy)", paramsEvt);
	sub(/evt.weaponDefId/, "WeaponDef.getInstance(ooClb, evt.weaponDefId)", paramsEvt);

	if (eNameLowerized == "init") {
		paramsTypes = "int teamId, OOAICallback callback, Properties info, Properties options";
		paramsEvt = "evt.team, ooClb, evt.info, evt.options";
	} else if (eNameLowerized == "playerCommand") {
		paramsTypes = "java.util.List<Unit> units, AICommand command, int playerId";
		paramsEvt = "units, command, evt.playerId";
	}

	print("") >> myOOAIFile;
	printFunctionComment_Common(myOOAIFile, evtsDocComment, evtIndex, "\t");

	print("	int " eNameLowerized "(" paramsTypes ");") >> myOOAIFile;

	print("") >> myOOAIAbstractFile;
	print("	public int " eNameLowerized "(" paramsTypes ") {") >> myOOAIAbstractFile;
	print("		return 0; // signaling: OK") >> myOOAIAbstractFile;
	print("	}") >> myOOAIAbstractFile;

	print("\t\t\t\t\t" "case " eCls ".TOPIC:") >> myOOAIFactoryFile;
	print("\t\t\t\t\t\t" "{") >> myOOAIFactoryFile;
	print("\t\t\t\t\t\t\t" eCls " evt = new " eCls "(event);") >> myOOAIFactoryFile;
	if (eNameLowerized == "init") {
		print("\t\t\t\t\t\t\t" "ooClb = OOAICallback.getInstance(evt.callback, evt.team);") >> myOOAIFactoryFile;
		print("\t\t\t\t\t\t\t" "ooClbs.put(teamId, ooClb);") >> myOOAIFactoryFile;
	} else if (eNameLowerized == "playerCommand") {
		print("\t\t\t\t\t\t\t" "java.util.ArrayList<Unit> units = new java.util.ArrayList<Unit>(evt.numUnitIds);") >> myOOAIFactoryFile;
		print("\t\t\t\t\t\t\t" "for (int i=0; i < evt.numUnitIds; i++) {") >> myOOAIFactoryFile;
		print("\t\t\t\t\t\t\t\t" "units.add(Unit.getInstance(ooClb, evt.unitIds[i]));") >> myOOAIFactoryFile;
		print("\t\t\t\t\t\t\t" "}") >> myOOAIFactoryFile;
		print("\t\t\t\t\t\t\t" "AICommand command = AICommandWrapper.wrapp(evt.commandTopic, evt.commandData);") >> myOOAIFactoryFile;
	}
	print("\t\t\t\t\t\t\t" "_ret = ai." eNameLowerized "(" paramsEvt ");") >> myOOAIFactoryFile;
	print("\t\t\t\t\t\t\t" "break;") >> myOOAIFactoryFile;
	print("\t\t\t\t\t\t" "}") >> myOOAIFactoryFile;
}

function printEventOOAIFactory(evtIndex) {

	topicName = evtsTopicName[evtIndex];
	topicValue = evtsTopicNameValue[topicName];
	eName = evtsName[evtIndex];
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
	print("import com.sun.jna.*;") >> javaFile;
	print("") >> javaFile;
}

function printEventJava(evtIndex) {

	topicName = evtsTopicName[evtIndex];
	topicValue = evtsTopicNameValue[topicName];
	eName = evtsName[evtIndex];

	className = eName "AIEvent";
	javaFile = javaSrcRoot "/" myPkgEvtD "/" className ".java";
	evtInterface = "AIEvent";
	clsMods = "public final ";
	printJavaEventHeader(javaFile);

	if (eName == "Init") {
		#clsMods = "public abstract ";
		printEventJavaCls(evtIndex);

		className = "Win32" eName "AIEvent";
		javaFile = javaSrcRoot "/" myPkgEvtD "/" className ".java";
		#evtInterface = "InitAIEvent";
		#clsMods = "public final ";
		printJavaEventHeader(javaFile);
		printEventJavaCls(evtIndex);

		className = "Default" eName "AIEvent";
		javaFile = javaSrcRoot "/" myPkgEvtD "/" className ".java";
		printJavaEventHeader(javaFile);
		printEventJavaCls(evtIndex);
	} else {
		printEventJavaCls(evtIndex);
	}
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
	if (className == "InitAIEvent") {
		print("	public static boolean isWin32() {") >> javaFile;
		print("") >> javaFile;
		print("		String os = System.getProperty(\"os.name\", \"UNKNOWN\");") >> javaFile;
		print("		boolean isWindows = os.matches(\"[Ww]in\");") >> javaFile;
		print("		String archDataModel = System.getProperty(\"sun.arch.data.model\", \"32\");") >> javaFile;
		print("		boolean is32bit = archDataModel.equals(\"32\");") >> javaFile;
		print("		return isWindows && is32bit;") >> javaFile;
		print("	}") >> javaFile;
		print("") >> javaFile;
	}

	print("	public " className "(Pointer memory) {") >> javaFile;
	print("") >> javaFile;
	if (className == "InitAIEvent") {
		print("		if (isWin32()) {") >> javaFile;
		print("			Win32InitAIEvent initEvtImpl = new Win32InitAIEvent(memory);") >> javaFile;
		for (m=0; m < evtsNumMembers[evtIndex]; m++) {
			name = evtsMembers_name[evtIndex, m];
			if (!match(name, /(info|options)(Size|Keys|Values)/)) {
				print("			this." name " = initEvtImpl." name ";") >> javaFile;
			}
		}
		print("") >> javaFile;
		print("			Pointer[] pOptionKeys = initEvtImpl.optionsKeys.getPointerArray(0L, initEvtImpl.optionsSize);") >> javaFile;
		print("			Pointer[] pOptionValues = initEvtImpl.optionsValues.getPointerArray(0L, initEvtImpl.optionsSize);") >> javaFile;
		print("			java.util.Properties tmpOptions = new java.util.Properties();") >> javaFile;
		print("			for (int i=0; i < initEvtImpl.optionsSize; i++) {") >> javaFile;
		print("				tmpOptions.setProperty(pOptionKeys[i].getString(0L), pOptionValues[i].getString(0L));") >> javaFile;
		print("			}") >> javaFile;
		print("			this.options = tmpOptions;") >> javaFile;
		print("") >> javaFile;
		print("			Pointer[] pInfoKeys = initEvtImpl.infoKeys.getPointerArray(0L, initEvtImpl.infoSize);") >> javaFile;
		print("			Pointer[] pInfoValues = initEvtImpl.infoValues.getPointerArray(0L, initEvtImpl.infoSize);") >> javaFile;
		print("			java.util.Properties tmpInfo = new java.util.Properties();") >> javaFile;
		print("			for (int i=0; i < initEvtImpl.infoSize; i++) {") >> javaFile;
		print("				tmpInfo.setProperty(pInfoKeys[i].getString(0L), pInfoValues[i].getString(0L));") >> javaFile;
		print("			}") >> javaFile;
		print("			this.info = tmpInfo;") >> javaFile;
		print("		} else {") >> javaFile;
		print("			DefaultInitAIEvent initEvtImpl =  new DefaultInitAIEvent(memory);") >> javaFile;
		for (m=0; m < evtsNumMembers[evtIndex]; m++) {
			name = evtsMembers_name[evtIndex, m];
			if (!match(name, /(sizeInfo)|(info)|(sizeOptions)|(optionKeys)|(optionValues)/)) {
				print("			this." name " = initEvtImpl." name ";") >> javaFile;
			}
		}
		print("") >> javaFile;
		print("			Pointer[] pOptionKeys = initEvtImpl.optionsKeys.getPointerArray(0L, initEvtImpl.optionsSize);") >> javaFile;
		print("			Pointer[] pOptionValues = initEvtImpl.optionsValues.getPointerArray(0L, initEvtImpl.optionsSize);") >> javaFile;
		print("			java.util.Properties tmpOptions = new java.util.Properties();") >> javaFile;
		print("			for (int i=0; i < initEvtImpl.optionsSize; i++) {") >> javaFile;
		print("				tmpOptions.setProperty(pOptionKeys[i].getString(0L), pOptionValues[i].getString(0L));") >> javaFile;
		print("			}") >> javaFile;
		print("			this.options = tmpOptions;") >> javaFile;
		print("") >> javaFile;
		print("			Pointer[] pInfoKeys = initEvtImpl.infoKeys.getPointerArray(0L, initEvtImpl.infoSize);") >> javaFile;
		print("			Pointer[] pInfoValues = initEvtImpl.infoValues.getPointerArray(0L, initEvtImpl.infoSize);") >> javaFile;
		print("			java.util.Properties tmpInfo = new java.util.Properties();") >> javaFile;
		print("			for (int i=0; i < initEvtImpl.infoSize; i++) {") >> javaFile;
		print("				tmpInfo.setProperty(pInfoKeys[i].getString(0L), pInfoValues[i].getString(0L));") >> javaFile;
		print("			}") >> javaFile;
		print("			this.info = tmpInfo;") >> javaFile;
		print("		}") >> javaFile;
	} else {
		print("		useMemory(memory);") >> javaFile;
		print("		read();") >> javaFile;
	}
	print("	}") >> javaFile;
	print("") >> javaFile;
	for (m=0; m < evtsNumMembers[evtIndex]; m++) {
		name = evtsMembers_name[evtIndex, m];
		type_c = evtsMembers_type_c[evtIndex, m];
		type_jna = convertCToJNAType(type_c);
		memMods = "public ";
		if (name == "callback") {
			if (className == "Win32InitAIEvent") {
				type_jna = "Win32AICallback";
			} else if (className == "DefaultInitAIEvent") {
				type_jna = "DefaultAICallback";
			}
		} else if (className == "InitAIEvent" && match(name, /(sizeInfo)|(info)|(sizeOptions)|(optionKeys)|(optionValues)/)) {
			continue;
		}
		print("	" memMods type_jna " " name ";") >> javaFile;
	}
	if (className == "InitAIEvent") {
		print("	" memMods "java.util.Properties info;") >> javaFile;
		print("	" memMods "java.util.Properties options;") >> javaFile;
	}
	print("}") >> javaFile;
	print("") >> javaFile;
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
/^}; \/\/ EVENT_.*$/ {

	evtsNumMembers[ind_evtStructs] = ind_evtMember;
	evtsTopicName[ind_evtStructs] = $3;
	storeDocLines(evtsDocComment, ind_evtStructs);

	printEventJava(ind_evtStructs);
	printEventOO(ind_evtStructs);

	ind_evtStructs++;
	isInsideEvtStruct = 0;
}


# inside of struct S*Event
{
	if (isInsideEvtStruct == 1) {
		size_tmpMembers = split($0, tmpMembers, ";");
		for (i=1; i<=size_tmpMembers; i++) {
			tmpMembers[i] = trim(tmpMembers[i]);
			if (tmpMembers[i] == "" || match(tmpMembers[i], /^\/\//)) {
				break;
			}
			saveMember(ind_evtMember++, tmpMembers[i]);
		}
	}
}

# beginn of struct S*Event
/^\struct S.*Event( \{)?/ {

	isInsideEvtStruct = 1;
	ind_evtMember = 0;
	eventName = $2;
	sub(/^S/, "", eventName);
	sub(/Event$/, "", eventName);

	evtsName[ind_evtStructs] = eventName;
}

### END: parsing and saving the event structs
################################################################################




END {
	# finalize things

	printOOAIEnd(myOOAIFile);
	printOOAIEnd(myOOAIAbstractFile);
	printOOAIFactoryEnd(myOOAIFactoryFile);
}
