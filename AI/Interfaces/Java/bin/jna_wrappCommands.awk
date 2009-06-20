#!/bin/awk
#
# This awk script creates the JNA wrapper classes for the C command structs in:
# rts/ExternalAI/Interface/AISCommands.h
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
		GENERATED_SOURCE_DIR = "../java/generated";
	}

	javaSrcRoot = "../java/src";
	javaGeneratedSrcRoot = GENERATED_SOURCE_DIR;

	myPkgA = "com.springrts.ai";
	#myClass = "AICallback";
	aiFloat3Class = "AIFloat3";
	myPkgD = convertJavaNameFormAToD(myPkgA);

	myPkgCmdA = myPkgA ".command";
	myPkgCmdD = convertJavaNameFormAToD(myPkgCmdA);

	myPointerCmdWrapperClass = "AICommandWrapper";
	myPointerCmdWrapperFile = javaGeneratedSrcRoot "/" myPkgD "/" myPointerCmdWrapperClass ".java";

	indent = "	";

	ind_cmdTopics = 0;
	ind_cmdStructs = 0;
	insideCmdStruct = 0;
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
	isUnitCmd = 0;

	javaFile = javaGeneratedSrcRoot "/" myPkgCmdD "/" name "AICommand.java";
	className = name "AICommand";
	cmdInterface = "AICommand";
	firstMethod = 0;
	if (isUnitCmd) {
		cmdInterface = "UnitAICommand";
		firstMethod = 4;
	}

	printJavaCommandHeader(javaFile);
	printFunctionComment_Common(javaFile, cmdsDocComment, cmdIndex, "");
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
	for (m=firstMethod; m < cmdsNumMembers[cmdIndex]; m++) {
		name = cmdsMembers_name[cmdIndex, m];
		type_c = cmdsMembers_type_c[cmdIndex, m];
		type_jna = convertCToJNAType(type_c);
		print("	public " type_jna " " name ";") >> javaFile;
	}
	print("}") >> javaFile;
	print("") >> javaFile;
	close(javaFile);
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


# aggare te los command defines in order
/^[ \t]*COMMAND_.*$/ {

	sub(",", "", $4);
	cmdsTopicNameValue[$2] = $4;
}



# This function has to return true (1) if a doc comment (eg: /** foo bar */)
# can be deleted.
# If there is no special condition you want to apply,
# it should always return true (1),
# cause there are additional mechanism to prevent accidential deleting.
# see: commonDoc.awk
function canDeleteDocumentation() {
	return isInsideCmdStruct != 1;
}

################################################################################
### BEGINN: parsing and saving the command structs

# end of struct S*Command 
/^}; \/\/ COMMAND_.*$/ {

	cmdsNumMembers[ind_cmdStructs] = ind_cmdMember;
	cmdsTopicName[ind_cmdStructs] = $3;
	storeDocLines(cmdsDocComment, ind_cmdStructs);

	if (doWrapp(ind_cmdStructs)) {
		printCommandJava(ind_cmdStructs);
	}

	ind_cmdStructs++;
	isInsideCmdStruct = 0;
}


# inside of struct S*Command 
{
	if (isInsideCmdStruct == 1) {
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

	isInsideCmdStruct = 1;
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

### END: parsing and saving the command structs
################################################################################



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
	close(myPointerCmdWrapperFile);
}
