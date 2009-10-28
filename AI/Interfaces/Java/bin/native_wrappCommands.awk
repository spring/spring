#!/bin/awk
#
# This awk script creates the C functions for wrapping the C command structs in:
# rts/ExternalAI/Interface/AISCommands.h
#
# Before running this script, you have to wrapp the native callback struct
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
	FS = "[ \t]+";

	# These vars can be assigned externally, see file header.
	# Set the default values if they were not supplied on the command line.
	if (!GENERATED_SOURCE_DIR) {
		GENERATED_SOURCE_DIR = "../src-generated/main";
	}
	if (!NATIVE_GENERATED_SOURCE_DIR) {
		NATIVE_GENERATED_SOURCE_DIR = GENERATED_SOURCE_DIR "/native";
	}

	nativeBridge = "FunctionPointerBridge";
	bridgePrefix = "bridged__";

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




function saveMember(ind_mem, member) {

	name = extractParamName(member);
	type_c = extractCType(member);

	cmdsMembers_name[ind_cmdStructs, ind_mem] = name;
	cmdsMembers_type_c[ind_cmdStructs, ind_mem] = type_c;
}


function doWrapp(ind_cmdStructs_dw) {
	return !match(cmdsName[ind_cmdStructs_dw], /.*SharedMemArea.*/);
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

	print("// END: COMMAND_WRAPPERS") >> outFile_nh;
	print("") >> outFile_nh;
	print("") >> outFile_nc;

	# print the command wrapping functions
	for (cmdIndex=0; cmdIndex < ind_cmdStructs; cmdIndex++) {
		topicName  = cmdsTopicName[cmdIndex];
		topicValue = cmdsTopicNameValue[topicName];
		name       = cmdsName[cmdIndex];
		metaInf    = cmdsMetaInfo[cmdIndex];
		fullName   = metaInf;
		sub(/ .*$/, "", fullName);
		fullName   = "Clb_" fullName;
		sub(/^[^ \t]*/, "", metaInf);

		hasRetType = 0;
		retType    = "int";
		retParam   = "";
		paramList  = "int _teamId";
		firstMember = 0;
		if (cmdsNumMembers[cmdIndex] > 0) {
			for (m=firstMember; m < cmdsNumMembers[cmdIndex]; m++) {
				memName   = cmdsMembers_name[cmdIndex, m];
				memType_c = cmdsMembers_type_c[cmdIndex, m];

				if (match(memName, /^ret_/) && 1 == 0) {
					retParam   = memName;
					retType    = memType_c;
					hasRetType = 1;
					#metaInf    = metaInf "return:" bla;
				} else {
					paramList = paramList ", " memType_c " " memName;
				}
			}
		}
		if (!hasRetType) {
			metaInf = metaInf " error-return:0=OK";
		}
		paramListNoTypes = removeParamTypes(paramList);
		metaInf = trim(metaInf);


		if (doWrapp(cmdIndex)) {
			# print function declaration to *.h

			# Add the member comments to the main comment as @param attributes
			numCmdDocLines = cmdsDocComment[cmdIndex, "*"];
			for (m=firstMember; m < cmdsNumMembers[cmdIndex]; m++) {
				numLines = cmdMbrsDocComments[cmdIndex*100 + m, "*"];
				if (numLines > 0) {
					cmdsDocComment[cmdIndex, numCmdDocLines] = "@param " cmdsMembers_name[cmdIndex, m];
				}
				for (l=0; l < numLines; l++) {
					cmdsDocComment[cmdIndex, numCmdDocLines] = cmdsDocComment[cmdIndex, numCmdDocLines] " " cmdMbrsDocComments[cmdIndex*100 + m, l];
				}
				if (numLines > 0) {
					numCmdDocLines++;
				}
			}
			cmdsDocComment[cmdIndex, "*"] = numCmdDocLines;

			# print the doc comment for this command/function
			printFunctionComment_Common(outFile_nh, cmdsDocComment, cmdIndex, "");

			outName = fullName;
			sub(/Clb_/, bridgePrefix, outName);

			commentEol = "";
			if (metaInf != "") {
				commentEol = " // " metaInf;
			}
			print("EXPORT(" retType ") " outName "(" paramList ");" commentEol) >> outFile_nh;
			print("") >> outFile_nh;

			# print function definition to *.c
			print("") >> outFile_nc;
			print("EXPORT(" retType ") " outName "(" paramList ") {") >> outFile_nc;
			print("") >> outFile_nc;

			print("\t" "struct S" name "Command commandData;") >> outFile_nc;
			for (m=firstMember; m < cmdsNumMembers[cmdIndex]; m++) {
				memName   = cmdsMembers_name[cmdIndex, m];
				memType_c = cmdsMembers_type_c[cmdIndex, m];

				if (match(name, /^ret_/)) {
					# do nothing
				} else {
					print("\t" "commandData." memName " = " memName ";") >> outFile_nc;
				}
			}
			print("") >> outFile_nc;

			print("\t" "int _ret = id_clb[_teamId]->Clb_Engine_handleCommand(_teamId, COMMAND_TO_ID_ENGINE, -1, " topicName ", &commandData);") >> outFile_nc;
			print("") >> outFile_nc;

			if (hasRetType) {
				print("\t" "if (_ret == 0) {") >> outFile_nc;
				print("\t\t" "return commandData." retParam ";") >> outFile_nc;
				print("\t" "} else {") >> outFile_nc;
				print("\t\t" "return NULL;") >> outFile_nc;
				print("\t" "}") >> outFile_nc;
			} else {
				print("\t" "return _ret;") >> outFile_nc;
			}

			print("" "}") >> outFile_nc;
		} else {
			print("Note: The following command is intentionally not wrapped: " fullName);
		}
	}

	print("// END: COMMAND_WRAPPERS") >> outFile_nh;
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
	cmdsTopicName[ind_cmdStructs]  = $3;
	cmdsMetaInfo[ind_cmdStructs]   = $0;
	sub(/^}; \/\/ COMMAND_[^ \t]+[ \t]+/, "",  cmdsMetaInfo[ind_cmdStructs]);

	if (doWrapp(ind_cmdStructs)) {
		#printCommandJava(ind_cmdStructs);
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
			# This would bork with more then 100 members in a command
			storeDocLines(cmdMbrsDocComments, ind_cmdStructs*100 + ind_cmdMember);
			saveMember(ind_cmdMember, tmpMembers[i]);
			ind_cmdMember++;
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
	storeDocLines(cmdsDocComment, ind_cmdStructs);
}

# find COMMAND_TO_ID_ENGINE id
/COMMAND_TO_ID_ENGINE/ {

	cmdToIdEngine = $3;
}

### END: parsing and saving the command structs
################################################################################



END {
	# finalize things

	printNativeFP2F()
}
