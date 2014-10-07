#!/usr/bin/awk -f
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
#                               default: "../src-generated"
#
# usage:
# 	awk -f thisScript.awk -f common.awk -f commonDoc.awk
# 	awk -f thisScript.awk -f common.awk -f commonDoc.awk \
#       -v 'GENERATED_SOURCE_DIR=/tmp/build/AI/Wrappers/Cpp/src-generated'
#

BEGIN {
	# initialize things

	# define the field splitter(-regex)
	FS = "[ \t]+";

	# These vars can be assigned externally, see file header.
	# Set the default values if they were not supplied on the command line.
	if (!GENERATED_SOURCE_DIR) {
		GENERATED_SOURCE_DIR = "../src-generated";
	}

	nativeBridge = "CombinedCallbackBridge";
	bridgePrefix = "bridged_";

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

	doWrp_dw = 1;

	if (match(cmdsName[ind_cmdStructs_dw], /SharedMemArea/)) {
		doWrp_dw = 0;
	} else if (match(cmdsName[ind_cmdStructs_dw], /^SCallLuaRulesCommand$/)) {
		doWrp_dw = 0;
	}

	return doWrp_dw;
}


function createNativeFileName(fileName_fn, isHeader_fn) {

	absFileName_fn = GENERATED_SOURCE_DIR "/" fileName_fn;
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
		sub(/^[^ \t]*/, "", metaInf);

		hasRetType = 0;
		retType    = "int";
		retParam   = "";
		paramList  = "int skirmishAIId";
		firstMember = 0;
		if (cmdsNumMembers[cmdIndex] > 0) {
			for (m=firstMember; m < cmdsNumMembers[cmdIndex]; m++) {
				memName   = cmdsMembers_name[cmdIndex, m];
				memType_c = cmdsMembers_type_c[cmdIndex, m];

				if (match(memName, /^ret_/) && !match(memType_c, /\*/)) {
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
				numLines = cmdMbrsDocComments[cmdIndex*1000 + m, "*"];
				if (numLines > 0) {
					cmdsDocComment[cmdIndex, numCmdDocLines] = "@param " cmdsMembers_name[cmdIndex, m];
				}
				for (l=0; l < numLines; l++) {
					if (l == 0) {
						_preDocLine = "@param " cmdsMembers_name[cmdIndex, m] "  ";
					} else {
						_preDocLine = "       " lengtAsSpaces(cmdsMembers_name[cmdIndex, m]) "  ";
					}
					cmdsDocComment[cmdIndex, numCmdDocLines] = _preDocLine cmdMbrsDocComments[cmdIndex*1000 + m, l];
					numCmdDocLines++;
				}
			}
			cmdsDocComment[cmdIndex, "*"] = numCmdDocLines;

			outName = bridgePrefix fullName;

			commentEol = "";
			if (metaInf != "") {
				commentEol = " // " metaInf;
			}

			if (match(fullName, /^Unit_/)) {
				# To make this fit in smoothly with the OO structure,
				# we want to present each unit command once for the unit class
				# and once for the Group class.

				# An other thing we do, is move the common UnitAICommand params
				# to the end of the params list, so we can supply default values
				# for them more easily later on.
				paramList_commonEnd = paramList;
				sub(/, short options, int timeOut/, "", paramList_commonEnd);
				paramList_commonEnd = paramList_commonEnd ", short options, int timeOut";
				
				# Unit version:
				paramList_unit = paramList_commonEnd;
				sub(/int groupId, /, "", paramList_unit);
				printFunctionComment_Common(outFile_nh, cmdsDocComment, cmdIndex, "");
				print("EXPORT(" retType ") " outName "(" paramList_unit ");" commentEol) >> outFile_nh;
				print("") >> outFile_nh;
				
				# Group version:
				paramList_group = paramList_commonEnd;
				sub(/int unitId, /, "", paramList_group);
				outName_group = outName;
				sub(/Unit_/, "Group_", outName_group);
				printFunctionComment_Common(outFile_nh, cmdsDocComment, cmdIndex, "");
				print("EXPORT(" retType ") " outName_group "(" paramList_group ");" commentEol) >> outFile_nh;
			} else {
				printFunctionComment_Common(outFile_nh, cmdsDocComment, cmdIndex, "");
				print("EXPORT(" retType ") " outName "(" paramList ");" commentEol) >> outFile_nh;
			}
			print("") >> outFile_nh;

			# print function definition to *.c
			print("") >> outFile_nc;
			if (match(fullName, /^Unit_/)) {
				# inner version:
				print("static " retType " internal_" outName "(" paramList ") {") >> outFile_nc;
			} else {
				print("EXPORT(" retType ") " outName "(" paramList ") {") >> outFile_nc;
			}
			print("") >> outFile_nc;

			print("\t" "struct S" name "Command commandData;") >> outFile_nc;
			print("\t" "int internal_ret;") >> outFile_nc;

			for (m=firstMember; m < cmdsNumMembers[cmdIndex]; m++) {
				memName   = cmdsMembers_name[cmdIndex, m];
				memType_c = cmdsMembers_type_c[cmdIndex, m];

				if (memName == retParam) {
					# do nothing
				} else {
					print("\t" "commandData." memName " = " memName ";") >> outFile_nc;
				}
			}
			print("") >> outFile_nc;

			print("\t" "internal_ret = id_clb[skirmishAIId]->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, " topicName ", &commandData);") >> outFile_nc;
			print("") >> outFile_nc;

			if (hasRetType) {
				print("\t" "if (internal_ret == 0) {") >> outFile_nc;
				print("\t\t" "return commandData." retParam ";") >> outFile_nc;
				print("\t" "} else {") >> outFile_nc;
				print("\t\t" "return (" retType ")0;") >> outFile_nc;
				print("\t" "}") >> outFile_nc;
			} else {
				print("\t" "return internal_ret;") >> outFile_nc;
			}
			print("}") >> outFile_nc;

			if (match(fullName, /^Unit_/)) {
				paramListNoTypes = removeParamTypes(paramList);

				# Unit version:
				print("") >> outFile_nc;
				print("EXPORT(" retType ") " outName "(" paramList_unit ") {" commentEol) >> outFile_nc;
				print("") >> outFile_nc;
				print("\t" "const int groupId = -1;") >> outFile_nc;
				print("\t" "return internal_" outName "(" paramListNoTypes ");") >> outFile_nc;
				print("}") >> outFile_nc;
				print("") >> outFile_nc;
	
				# Group version:
				print("EXPORT(" retType ") " outName_group "(" paramList_group ") {" commentEol) >> outFile_nc;
				print("") >> outFile_nc;
				print("\t" "const int unitId = -1;") >> outFile_nc;
				print("\t" "return internal_" outName "(" paramListNoTypes ");") >> outFile_nc;
				print("}") >> outFile_nc;
			}
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
	print("#endif // _COMBINED_CALLBACK_BRIDGE_H") >> outFile_nh;
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
/^}; \/\/\$ COMMAND_.*$/ {

	cmdsNumMembers[ind_cmdStructs] = ind_cmdMember;
	cmdsTopicName[ind_cmdStructs]  = $3;
	cmdsMetaInfo[ind_cmdStructs]   = $0;
	sub(/^}; \/\/\$ COMMAND_[^ \t]+[ \t]+/, "", cmdsMetaInfo[ind_cmdStructs]);

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
		# cause there is an empty part behind the ';'
		size_tmpMembers--;
		for (i=1; i<=size_tmpMembers; i++) {
			tmpMembers[i] = trim(tmpMembers[i]);
			if (tmpMembers[i] == "" || match(tmpMembers[i], /^\/\//)) {
				break;
			}
			# This would bork with more then 1000 members in a command,
			# or more then 1000 commands
			storeDocLines(cmdMbrsDocComments, ind_cmdStructs*1000 + ind_cmdMember);
			saveMember(ind_cmdMember, tmpMembers[i]);
			ind_cmdMember++;
		}
	}
}

# beginn of struct S*Command
/^struct S.*Command( \{)?/ {

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
