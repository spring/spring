#!/bin/awk
#
# This awk script creates the C++ wrapper classes for the C event structs in:
# rts/ExternalAI/Interface/AISEvents.h
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
	FS = "[ \t]+";

	# These vars can be assigned externally, see file header.
	# Set the default values if they were not supplied on the command line.
	if (!GENERATED_SOURCE_DIR) {
		GENERATED_SOURCE_DIR = "../src-generated/main/native";
	}

	mySrc = GENERATED_SOURCE_DIR;
	myNameSpace = "springai";
	myAIClass = "AI";
	myAbstractAIClass = "AbstractAI";
	myAIFactoryClass = "AIFactory";
	myAIFile_h = mySrc "/" myAIClass ".h";
	#myAIFile_cpp = mySrc "/" myAIClass ".cpp";
	myAbstractAIFile_h = mySrc "/" myAbstractAIClass ".h";
	#myAbstractAIFile_cpp = mySrc "/" myAbstractAIClass ".cpp";
	myAIFactoryFile_h = mySrc "/" myAIFactoryClass ".h";
	#myAIFactoryFile_cpp = mySrc "/" myAIFactoryClass ".cpp";

	printAIHeader(myAIFile_h, myAIClass);
	printAIHeader(myAbstractAIFile_h, myAbstractAIClass);
	printAIFactoryHeader(myAIFactoryFile_h);

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



function printAIHeader(outFile, clsName) {

	printGeneratedWarningHeader(outFile);
	print("") >> outFile;
	printGPLHeader(outFile);
	print("") >> outFile;
	if (clsName == myAbstractAIClass) {
		print("#include \"" myAIClass ".h\"") >> outFile;
		print("") >> outFile;
	} else {
		print("#include \"AICallback.h\"") >> outFile;
		print("#include \"Unit.h\"") >> outFile;
		print("#include \"WeaponDef.h\"") >> outFile;
		print("#include \"ExternalAI/Interface/AISCommands.h\"") >> outFile;
		print("#include \"ExternalAI/Interface/AISCommands.h\"") >> outFile;
		print("#include \"ExternalAI/Interface/SAIFloat3.h\"") >> outFile;
		print("") >> outFile;
		print("#include <string>") >> outFile;
		print("#include <vector>") >> outFile;
		print("#include <map>") >> outFile;
	}
	print("") >> outFile;
	print("/**") >> outFile;
	print(" *") >> outFile;
	print(" *") >> outFile;
	print(" * @author	hoijui") >> outFile;
	print(" * @version	GENERATED") >> outFile;
	print(" */") >> outFile;
	print("namespace " myNameSpace " {") >> outFile;
	if (clsName == myAbstractAIClass) {
		print("class " clsName " : public " myAIClass " {") >> outFile;
	} else {
		print("class " clsName " {") >> outFile;
	}
	print("") >> outFile;
	print("public:") >> outFile;
	print("") >> outFile;
	if (clsName == myAIClass) {
		print("	typedef std::map<std::string, std::string> Properties;") >> outFile;
		print("") >> outFile;
	}
}
function printAIEnd(outFile) {

	print("}; // end of class") >> outFile;
	print("} // end of namespace") >> outFile;
	print("") >> outFile;
}

function printAIFactoryHeader(outFile) {

	printGeneratedWarningHeader(outFile);
	print("") >> outFile;
	printGPLHeader(outFile);
	print("") >> outFile;
	print("#include \"ExternalAI/Interface/AISCommands.h\"") >> outFile;
	print("#include \"ExternalAI/Interface/SAIFloat3.h\"") >> outFile;
	print("") >> outFile;
	print("#include <string>") >> outFile;
	print("#include <map>") >> outFile;
	print("") >> outFile;
	print("/**") >> outFile;
	print(" * TODO: Add description here") >> outFile;
	print(" *") >> outFile;
	print(" * @author	hoijui") >> outFile;
	print(" * @version	GENERATED") >> outFile;
	print(" */") >> outFile;
	print("namespace " myNameSpace " {") >> outFile;
	print("class " myAIFactoryClass " : public AI {") >> outFile;
	print("") >> outFile;
	print("private:") >> outFile;
	print("") >> outFile;
	print("	std::map<int, AI*> ais;") >> outFile;
	print("	std::map<int, AICallback*> clbs;") >> outFile;
	print("") >> outFile;
	print("	AI* GetAI(int teamId, bool remove = false) {") >> outFile;
	print("") >> outFile;
	print("		AI* ai = NULL;") >> outFile;
	print("") >> outFile;
	print("		std::map<int, AI*>::iterator ai_it;") >> outFile;
	print("		ai_it = ais.find(teamId);") >> outFile;
	print("		if (ai_it != ais.end()) {") >> outFile;
	print("			ai = *ai_it;") >> outFile;
	print("		}") >> outFile;
	print("		if (remove && ai != NULL) {") >> outFile;
	print("			ais.remove(ai_it);") >> outFile;
	print("		}") >> outFile;
	print("") >> outFile;
	print("	AICallback* GetAICallback(int teamId, bool remove = false) {") >> outFile;
	print("") >> outFile;
	print("		AICallback* ai = NULL;") >> outFile;
	print("") >> outFile;
	print("		std::map<int, AICallback*>::iterator ai_it;") >> outFile;
	print("		clb_it = clbs.find(teamId);") >> outFile;
	print("		if (clb_it != ais.end()) {") >> outFile;
	print("			clb = *clb_it;") >> outFile;
	print("		}") >> outFile;
	print("		if (remove && clb != NULL) {") >> outFile;
	print("			clbs.remove(clb_it);") >> outFile;
	print("		}") >> outFile;
	print("") >> outFile;
	print("		return clb;") >> outFile;
	print("	}") >> outFile;
	print("") >> outFile;
	print("public:") >> outFile;
	print("") >> outFile;
	print("	typedef std::map<std::string, std::string> Properties;") >> outFile;
	print("") >> outFile;
	print("	int Init(int teamId, Properties info, Properties options) {") >> outFile;
	print("") >> outFile;
	print("		AI* ai = CreateAI(teamId, info, options);") >> outFile;
	print("		if (ai != NULL) {") >> outFile;
	print("			ais[teamId] = ai);") >> outFile;
	print("		}") >> outFile;
	print("		return (ai == NULL) ? -1 : 0;") >> outFile;
	print("	}") >> outFile;
	print("") >> outFile;
	print("	int Release(int teamId) {") >> outFile;
	print("") >> outFile;
	print("		AI* ai = GetAI(teamId, true);") >> outFile;
	print("		AICallback* clb = GetAICallback(teamId, true);") >> outFile;
	print("		int ret = (ai == NULL) ? -1 : 0;") >> outFile;
	print("		delete ai;") >> outFile;
	print("		delete clb;") >> outFile;
	print("		return ret;") >> outFile;
	print("	}") >> outFile;
	print("") >> outFile;
	print("	int HandleEvent(int teamId, int topic, const void* event) {") >> outFile;
	print("") >> outFile;
	print("		int _ret = -1;") >> outFile;
	print("") >> outFile;
	print("		AI* ai = GetAI(teamId);") >> outFile;
	print("		AICallback* clb = GetAICallback(teamId);") >> outFile;
	print("") >> outFile;
	print("		if (ai != NULL) {") >> outFile;
	print("			try {") >> outFile;
	print("				switch (topic) {") >> outFile;
	print("") >> outFile;
}
function printAIFactoryEnd(outFile) {

	print("					default: {") >> outFile;
	print("						_ret = 1;") >> outFile;
	print("					}") >> outFile;
	print("				}") >> outFile;
	print("			} catch (Throwable t) { // TODO: this works?") >> outFile;
	print("				_ret = 2;") >> outFile;
	print("			}") >> outFile;
	print("		}") >> outFile;
	print("") >> outFile;
	print("		return _ret;") >> outFile;
	print("	}") >> outFile;
	print("") >> outFile;
	print("	virtual AI* CreateAI(int teamId, Properties info, Properties options) = 0;") >> outFile;
	print("}; // end of class") >> outFile;
	print("} // end of namespace") >> outFile;
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
	sub(/int builder(Id)?/, "Unit builder", paramsTypes);
	sub(/int attacker(Id)?/, "Unit attacker", paramsTypes);
	sub(/int enemy(Id)?/, "Unit enemy", paramsTypes);
	sub(/int weaponDef(Id)?/, "WeaponDef weaponDef", paramsTypes);

	unitRepls = sub(/evt.unitId/, "Unit.getInstance(ooClb, evt.unitId)", paramsEvt);
	if (unitRepls == 0) {
		sub(/evt.unit/, "Unit.getInstance(ooClb, evt.unit)", paramsEvt);
	}
	sub(/evt.builder/, "Unit.getInstance(ooClb, evt.builder)", paramsEvt);
	sub(/evt.attacker/, "Unit.getInstance(ooClb, evt.attacker)", paramsEvt);
	sub(/evt.enemy/, "Unit.getInstance(ooClb, evt.enemy)", paramsEvt);
	sub(/evt.weaponDefId/, "WeaponDef.getInstance(ooClb, evt.weaponDefId)", paramsEvt);

	if (eName == "Init") {
		paramsTypes = "int teamId, OOAICallback callback";
		paramsEvt = "evt.team, ooClb";
	} else if (eNameLowerized == "playerCommand") {
		paramsTypes = "std::vector<Unit> units, AICommand command, int playerId";
		paramsEvt = "units, command, evt.playerId";
	}

	print("") >> myAIFile_h;
	printFunctionComment_Common(myAIFile_h, evtsDocComment, evtIndex, "\t");

	print("	virtual int " eName "(" paramsTypes ") = 0;") >> myAIFile_h;

	print("") >> myAbstractAIFile_h;
	print("	virtual int " eName "(" paramsTypes ") {") >> myAbstractAIFile_h;
	print("		return 0; // signaling: OK") >> myAbstractAIFile_h;
	print("	}") >> myAbstractAIFile_h;

	print("\t\t\t\t\t" "case " eCls ": {") >> myAIFactoryFile_h;
	print("\t\t\t\t\t\t" eCls " evt = (" eCls ") event;") >> myAIFactoryFile_h;
	if (eName == "Init") {
		print("\t\t\t\t\t\t" "clb = AICallback.getInstance(evt.callback, evt.team);") >> myAIFactoryFile_h;
		print("\t\t\t\t\t\t" "clbs.put(teamId, clb);") >> myAIFactoryFile_h;
	} else if (eName == "PlayerCommand") {
		print("\t\t\t\t\t\t" "std::vector<Unit> units;") >> myAIFactoryFile_h;
		print("\t\t\t\t\t\t" "for (int i=0; i < evt.numUnitIds; i++) {") >> myAIFactoryFile_h;
		print("\t\t\t\t\t\t\t" "units.push_back(Unit.getInstance(clb, evt.unitIds[i]));") >> myAIFactoryFile_h;
		print("\t\t\t\t\t\t" "}") >> myAIFactoryFile_h;
		print("\t\t\t\t\t\t" "AICommand command = AICommandWrapper.wrapp(evt.commandTopic, evt.commandData);") >> myAIFactoryFile_h;
	}
	print("\t\t\t\t\t\t" "_ret = ai." eName "(" paramsEvt ");") >> myAIFactoryFile_h;
	print("\t\t\t\t\t\t" "break;") >> myAIFactoryFile_h;
	print("\t\t\t\t\t" "}") >> myAIFactoryFile_h;
}

function printEventOOAIFactory(evtIndex) {

	topicName = evtsTopicName[evtIndex];
	topicValue = evtsTopicNameValue[topicName];
	eName = evtsName[evtIndex];
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

	printAIEnd(myAIFile_h);
	printAIEnd(myAbstractAIFile_h);
	printAIFactoryEnd(myAIFactoryFile_h);

	close(myAIFile_h);
	close(myAbstractAIFile_h);
	close(myAIFactoryFile_h);
}
