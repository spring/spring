#!/bin/awk
#
# This awk script creates the JNA wrapper classes for the C command structs in:
# rts/ExternalAI/Interface/AISCommands.h
#

BEGIN {
	# initialize things

	# define the field splitter(-regex)
	FS="[ \t]+"

	javaSrcRoot = "../java/src";

	myPkgA = "com.clan_sy.spring.ai";
	#myClass = "AICallback";
	aiFloat3Class = "AIFloat3";
	myPkgD = convertJavaNameFormAToD(myPkgA);

	myPkgCmdA = myPkgA ".command";
	myPkgCmdD = convertJavaNameFormAToD(myPkgCmdA);

	myPointerCmdWrapperClass = "AICommandWrapper";
	myPointerCmdWrapperFile = javaSrcRoot "/" myPkgD "/" myPointerCmdWrapperClass ".java";

	# Print a warning header
	indent = "	";

	ind_cmdTopics = 0;
	ind_cmdStructs = 0;
	insideCmdStruct = 0;
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


# Some utility functions

function ltrim(s) { sub(/^[ \t]+/, "", s); return s; }
function rtrim(s) { sub(/[ \t]+$/, "", s); return s; }
function trim(s)  { return rtrim(ltrim(s)); }

function noSpaces(s)  { gsub(/[ \t]/, "", s); return s; }

function capFirst(s)  { return toupper(substr(s, 1, 1)) substr(s, 2); }

function capitalize(s)  { return toupper(substr(s, 1, 1)) substr(s, 2); }
function lowerize(s)  { return tolower(substr(s, 1, 1)) substr(s, 2); }

function startsWithCapital(s) { return match(s, /^[ABCDEFGHIJKLMNOPQRDTUVWXYZ]/); }
function startsWithLower(s) { return match(s, /^[abcdefghijklmnopqrdtuvwxyz]/); }


# Awaits this format:	com.clan_sy.spring.ai
# Returns this format:	com/clan_sy/spring/ai
function convertJavaNameFormAToD(javaNameFormA) {

	javaNameFormD = javaNameFormA;

	gsub(/\./, "/", javaNameFormD);

	return javaNameFormD;
}


# Awaits this format:	unsinged const char* varName[]
# Returns this format:	varName
function extractParamName(typeAndName) {

	name = trim(typeAndName);
	gsub(/ \*/, "* ", name);

	# Remove the type
	sub(/(const )?(unsigned )?(void|int|float|char|byte|bool|struct SAIFloat3)(\*)* /, "", name);

	# Remove possible array length specifiers ("[]" or "[2]")
	gsub(/\[.*\]/, "", name);

	name = trim(name);

	return name;
}


# Awaits this format:	const char* varName[]
# Returns this format:	const char* []
function extractCType(cTypeAndName) {

	cType = trim(cTypeAndName);

	gsub(/ \*/, "* ", cType);

	# Remove the name
	gsub(" " extractParamName(cTypeAndName), "", cType);

	gsub(/\[[^\]]*\]/, "*", cType);

	return cType;
}


# Checks if a field is available and is no comment
function isFieldUsable(f) {
	
	valid = 0;

	if (f && !match(f, /.*\/\/.*/)) {
		valid = 1;
	}

	return valid;
}


function printJavaCommandHeader(javaFile) {

	printGeneratedWarningHeader(javaFile);
	print("") >> javaFile;
	printGPLHeader(javaFile);
	print("") >> javaFile;
	print("package " myPkgCmdA ";") >> javaFile;
	print("") >> javaFile;
	print("") >> javaFile;
	print("import " myPkgA ".*;") >> javaFile;
	print("import " myPkgA ".oo.*;") >> javaFile;
	print("import com.sun.jna.*;") >> javaFile;
	print("") >> javaFile;
}


function printCommandJava(cmdIndex) {

	topicName = cmdsTopicName[cmdIndex];
	topicValue = cmdsTopicNameValue[topicName];
	name = cmdsName[cmdIndex];
	#isUnitCmd = cmdsIsUnitCmd[cmdIndex];
	isUnitCmd = 0;

	javaFile = javaSrcRoot "/" myPkgCmdD "/" name "AICommand.java";
	className = name "AICommand";
	cmdInterface = "AICommand";
	firstMethod = 0;
	if (isUnitCmd) {
		cmdInterface = "UnitAICommand";
		firstMethod = 4;
	}

	#print("########################################################");
	#print("creating file " javaFile);
	#print("### topic name: " topicName);
	#print("### topic value: " topicValue);
	#print("########################################################");
	printJavaCommandHeader(javaFile);
	print("public final class " className " extends " cmdInterface " {") >> javaFile;
	print("") >> javaFile;
	print("	public final static int TOPIC = " topicValue ";") >> javaFile;
	print("	public int getTopic() {") >> javaFile;
	print("		return " className ".TOPIC;") >> javaFile;
	print("	}") >> javaFile;

	# constructors
	print("") >> javaFile;
	print("	public " className "() {}") >> javaFile;
	print("	public " className "(Pointer memory) {") >> javaFile;
	print("") >> javaFile;
	print("		useMemory(memory);") >> javaFile;
	print("		read();") >> javaFile;
	print("	}") >> javaFile;

	if (cmdsNumMembers[cmdIndex] > 0) {
		typedMemberList = "";
		for (m=firstMethod; m < cmdsNumMembers[cmdIndex]; m++) {
			name = cmdsMembers_name[cmdIndex, m];
			type_c = cmdsMembers_type_c[cmdIndex, m];
			type_jna = convertCToJNAType(type_c);
			typedMemberList = typedMemberList ", " type_jna " " name;
		}
		sub(/\, /, "", typedMemberList);
		{ # the non-OO constructor
			print("	public " className "(" typedMemberList ") {") >> javaFile;
			print("") >> javaFile;
			for (m=firstMethod; m < cmdsNumMembers[cmdIndex]; m++) {
				name = cmdsMembers_name[cmdIndex, m];
				print("		this." name " = " name ";") >> javaFile;
			}
			print("	}") >> javaFile;
		}
		{ # the OO constructor
			typedMemberList = "";
			num_ooParams = 0;
			for (m=firstMethod; m < cmdsNumMembers[cmdIndex]; m++) {
				name = cmdsMembers_name[cmdIndex, m];
				type_c = cmdsMembers_type_c[cmdIndex, m];
				type_jna = convertCToJNAType(type_c);
				type_oo = type_jna;
				name_oo = name;
				func_oo = "";
				if (match(name, /[uU]nitId$/)) {
					sub(/Id$/, "", name_oo);
					type_oo = "Unit";
					func_oo = name_oo ".get" type_oo "Id()";
					num_ooParams++;
				} else if (match(name, /[uU]nitDefId$/)) {
					sub(/Id$/, "", name_oo);
					type_oo = "UnitDef";
					func_oo = name_oo ".get" type_oo "Id()";
					num_ooParams++;
				} else if (name == "options") {
					name_oo = "optionsList";
					type_oo = "java.util.List<AICommand.Option>";
					func_oo = "AICommand.Option.getBitField(" name_oo ")";
					num_ooParams++;
				}
				cmdsMembers_name_oo[cmdIndex, m] = name_oo;
				cmdsMembers_type_oo[cmdIndex, m] = type_oo;
				cmdsMembers_func_oo[cmdIndex, m] = func_oo;
				typedMemberList = typedMemberList ", " type_oo " " name_oo;
			}
			sub(/\, /, "", typedMemberList);
			if (num_ooParams > 0) {
				print("	public " className "(" typedMemberList ") {") >> javaFile;
				print("") >> javaFile;
				for (m=firstMethod; m < cmdsNumMembers[cmdIndex]; m++) {
					name = cmdsMembers_name[cmdIndex, m];
					name_oo = cmdsMembers_name_oo[cmdIndex, m];
					type_oo = cmdsMembers_type_oo[cmdIndex, m];
					func_oo = cmdsMembers_func_oo[cmdIndex, m];
					if (name != name_oo) {
						print("		this." name " = " func_oo ";") >> javaFile;
					} else {
						print("		this." name " = " name ";") >> javaFile;
					}
				}
				print("	}") >> javaFile;
			}
		}
	}

	print("") >> javaFile;
#print("//name: " name) >> javaFile;
#print("//topicName: " topicName) >> javaFile;
#print("//evtIndex: " evtIndex) >> javaFile;
	for (m=firstMethod; m < cmdsNumMembers[cmdIndex]; m++) {
		name = cmdsMembers_name[cmdIndex, m];
		type_c = cmdsMembers_type_c[cmdIndex, m];
		type_jna = convertCToJNAType(type_c);
		print("	public " type_jna " " name ";") >> javaFile;
	}
	print("}") >> javaFile;
	print("") >> javaFile;
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

function saveMember(ind_mem, member) {

	name = extractParamName(member);
	type_c = extractCType(member);

	cmdsMembers_name[ind_cmdStructs, ind_mem] = name;
	cmdsMembers_type_c[ind_cmdStructs, ind_mem] = type_c;
}


function doWrapp(ind_cmdStructs_dw) {
	return !match(cmdsName[ind_cmdStructs_dw], /.*SharedMemArea.*/);
}

# aggare te los command defines in order
/^[ \t]*COMMAND_.*$/ {

	sub(",", "", $4);
	cmdsTopicNameValue[$2] = $4;
}


# end of struct S*Command 
/^}; \/\/ COMMAND_.*$/ {

	cmdsNumMembers[ind_cmdStructs] = ind_cmdMember;
	cmdsTopicName[ind_cmdStructs] = $3;

	if (doWrapp(ind_cmdStructs)) {
		printCommandJava(ind_cmdStructs);
	}

	ind_cmdStructs++;
	insideCmdStruct = 0;
}


# inside of struct S*Command 
{
	if (insideCmdStruct == 1) {
		size_tmpMembers = split($0, tmpMembers, ";");
		for (i=1; i<=size_tmpMembers; i++) {
			tmpMembers[i] = trim(tmpMembers[i]);
			if (tmpMembers[i] == "" || match(tmpMembers[i], /^\/\//)) {
				break;
			}
			saveMember(ind_cmdMember++, tmpMembers[i]);
		}
	}
}

# beginn of struct S*Command
/^\struct S.*Command( \{)?/ {

	insideCmdStruct = 1;
	ind_cmdMember = 0;
	commandName = $2;
	sub(/^S/, "", commandName);
	sub(/Command$/, "", commandName);
	
	isUnitCommand = match(commandName, /.*Unit$/);

	cmdsIsUnitCmd[ind_cmdStructs] = isUnitCommand;
	cmdsName[ind_cmdStructs] = commandName;
}

# find COMMAND_TO_ID_ENGINE id
/COMMAND_TO_ID_ENGINE/ {

	cmdToIdEngine = $3;
}



function printPointerAICommandWrapperHeader(outFile_wh) {

	printGeneratedWarningHeader(outFile_wh);
	print("") >> outFile_wh;
	printGPLHeader(outFile_wh);
	print("") >> outFile_wh;
	print("package " myPkgA ";") >> outFile_wh;
	print("") >> outFile_wh;
	print("") >> outFile_wh;
	print("import " myPkgCmdA ".*;") >> outFile_wh;
	print("import com.sun.jna.*;") >> outFile_wh;
	print("import java.lang.reflect.Constructor;") >> outFile_wh;
	print("") >> outFile_wh;
	print("") >> outFile_wh;
	print("/**") >> outFile_wh;
	print(" *") >> outFile_wh;
	print(" *") >> outFile_wh;
	print(" * @author	hoijui") >> outFile_wh;
	print(" * @version	GENERATED") >> outFile_wh;
	print(" */") >> outFile_wh;
	print("public final class " myPointerCmdWrapperClass " {") >> outFile_wh;
	print("") >> outFile_wh;
	print("	public static final int COMMAND_TO_ID_ENGINE = " cmdToIdEngine ";") >> outFile_wh;
	print("") >> outFile_wh;
	print("	private static java.util.Map<Integer, Class<? extends AICommand>> topic_cmdClass = new java.util.HashMap<Integer, Class<? extends AICommand>>(" ind_cmdStructs ");") >> outFile_wh;
	print("	private static java.util.Map<Integer, Constructor<? extends AICommand>> topic_cmdCtor = new java.util.HashMap<Integer, Constructor<? extends AICommand>>(" ind_cmdStructs ");") >> outFile_wh;
	print("	static {") >> outFile_wh;
}
function printPointerAICommandWrapperPart(outFile_w, indCmd_w) {

	topicName = cmdsTopicName[indCmd_w];
	topicValue = cmdsTopicNameValue[topicName];
	cmdName = cmdsName[indCmd_w];

	print("		topic_cmdClass.put(" topicValue ", " cmdName "AICommand.class);") >> outFile_w;
}
function printPointerAICommandWrapperEnd(outFile_wh) {

	print("") >> outFile_wh;
	print("		for (int i=0; i < topic_cmdClass.size(); i++) {") >> outFile_wh;
	print("			try {") >> outFile_wh;
	print("				topic_cmdCtor.put(i, topic_cmdClass.get(i).getConstructor(Pointer.class));") >> outFile_wh;
	print("			} catch (Throwable t) {") >> outFile_wh;
	print("				// do nothing") >> outFile_wh;
	print("			}") >> outFile_wh;
	print("		}") >> outFile_wh;
	print("	}") >> outFile_wh;
	print("") >> outFile_wh;
	print("	public final static AICommand wrapp(int topic, Pointer data) {") >> outFile_wh;
	print("") >> outFile_wh;
	print("		AICommand _ret = null;") >> outFile_wh;
	print("") >> outFile_wh;
	print("		if (topic < 1 || topic >= topic_cmdClass.size()) {") >> outFile_wh;
	print("			return _ret;") >> outFile_wh;
	print("		}") >> outFile_wh;
	print("") >> outFile_wh;
	print("		try {") >> outFile_wh;
	print("			Constructor<? extends AICommand> cmdCtor = topic_cmdCtor.get(topic);") >> outFile_wh;
	print("			_ret = cmdCtor.newInstance(data);") >> outFile_wh;
	print("		} catch (Throwable t) {") >> outFile_wh;
	print("			_ret = null;") >> outFile_wh;
	print("		}") >> outFile_wh;
	print("") >> outFile_wh;
	print("		return _ret;") >> outFile_wh;
	print("	}") >> outFile_wh;
	print("") >> outFile_wh;
	print("}") >> outFile_wh;
	print("") >> outFile_wh;
}



END {
	# finalize things

	# print Pointer to AICommand wrapper helper
	printPointerAICommandWrapperHeader(myPointerCmdWrapperFile);
	for (i_f=0; i_f < ind_cmdStructs; i_f++) {
		if (doWrapp(i_f)) {
			printPointerAICommandWrapperPart(myPointerCmdWrapperFile, i_f);
		}
	}
	printPointerAICommandWrapperEnd(myPointerCmdWrapperFile);
}
