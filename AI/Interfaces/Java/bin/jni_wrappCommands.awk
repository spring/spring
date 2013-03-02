#!/usr/bin/awk -f
#
# This awk script creates a Java class containing important enumerations.
# These enumerations are taken from:  rts/ExternalAI/Interface/AISCommands.h
# It currently only takes CommandTopic and UnitCommandOptions enumerations
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

	myPkgA = "com.springrts.ai";
	myPkgD = convertJavaNameFormAToD(myPkgA);
    myClass = "Enumerations";

    #create empty arrays, holding names and values of the two enumerations
	cmdsTopicNamesLength = 0;
    split("", cmdsTopicNames);
	cmdsTopicValuesLength = 0;
    split("", cmdsTopicValues);
	unitCmdsTopicNamesLength = 0;
    split("", unitCmdsTopicNames);
	unitCmdsTopicValuesLength = 0;
    split("", unitCmdsTopicValues);
}


function createJavaFileName(fileName_fn) {
	return JAVA_GENERATED_SOURCE_DIR "/" myPkgD "/" fileName_fn ".java";
}

function printGeneralJavaHeader(outFile_h, javaPkg_h, javaClassName_h) {

	printCommentsHeader(outFile_h);
	print("") >> outFile_h;
	print("package " javaPkg_h ";") >> outFile_h;
	print("") >> outFile_h;
	print("") >> outFile_h;
	print("/**") >> outFile_h;
	print(" * These are the Java exposed enumerations.") >> outFile_h;
	print(" * We are not calling the engine, this is a pure Java class.") >> outFile_h;
	print(" *") >> outFile_h;
	print(" * @author	AWK wrapper script") >> outFile_h;
	print(" * @version	GENERATED") >> outFile_h;
	print(" */") >> outFile_h;
    print("public abstract class " javaClassName_h " {") >> outFile_h;
	print("") >> outFile_h;
}

function printJavaHeader() {

	outFile_i = createJavaFileName(myClass);

	printGeneralJavaHeader(outFile_i, myPkgA, myClass);
}

function printJavaEnums(enums) {
    printJavaEnum("CommandTopic", cmdsTopicNames, cmdsTopicNamesLength, cmdsTopicValues);
    print("") >> outFile_i;
    printJavaEnum("UnitCommandOptions", unitCmdsTopicNames, unitCmdsTopicNamesLength, unitCmdsTopicValues);
}

function printJavaEnum(enumName, names, namesLength, values) {
	outFile_i = createJavaFileName(myClass);
    printEnumHeader(enumName);

    # Prints the enum members and values
    first = 0;
	for (i=0; i<namesLength;i++) {
        if (first == 0) {
            first = 1;
            printf("\t\t") >> outFile_i;
        } else {
            printf(",\n\t\t") >> outFile_i;
        }
        printJavaEnumItem(names[i], values[i]);
	}

    if (first != 0) {
        print(";\n") >> outFile_i;
    }
    printEnumEnd(enumName);
}

function printEnumHeader(name) {
	outFile_i = createJavaFileName(myClass);

    # Prints the enum start
    print("\tpublic enum " name " {") >> outFile_i;
}

function printEnumEnd(name) {
	outFile_i = createJavaFileName(myClass);

    # Prints the enum end
    print("\t\tprivate final int id;") >> outFile_i;
    print("\t\t" name "(int id) { this.id = id; }") >> outFile_i;
    print("\t\tpublic int getValue() { return id; }") >> outFile_i;
    print("\t}") >> outFile_i;
}

function printJavaEnumItem(key, value) {
    printf(key "(" value ")") >> outFile_i;
}

function printJavaEnd() {

	outFile_i = createJavaFileName(myClass);

	print("}") >> outFile_i;
	print("") >> outFile_i;

	close(outFile_i);
}

/^[ \t]*COMMAND_.*$/ {

	doWrapp = !match(($0), /.*COMMAND_NULL.*/);
	if (doWrapp) {
        #parse enumeration of format x = number1,
		sub(",", "", $4);
		cmdsTopicNames[cmdsTopicNamesLength] = $2;
		cmdsTopicNamesLength++;
		cmdsTopicValues[cmdsTopicValuesLength] = $4;
		cmdsTopicValuesLength++;
	} else {
		print("Java-AIInterface: NOTE: JNI level: COMMANDS: intentionally not wrapped: " $2);
	}
}

/^[ \t]*UNIT_COMMAND_.*$/ {
	doWrapp = !match(($0), /.*COMMAND_NULL.*/);
	if (doWrapp) {
		unitCmdsTopicNames[unitCmdsTopicNamesLength] = $2;
		unitCmdsTopicNamesLength++;
        #parse enumeration of format x = (number1 << number2),
        sub("[ \t]*//.*", "", $0);
		sub(",", "", $0);
		sub($3, "", $0);
		sub($2, "", $0);
		sub("[ \t]*", "", $0);
		unitCmdsTopicValues[unitCmdsTopicValuesLength] = $0;
		unitCmdsTopicValuesLength++;
	} else {
		print("Java-AIInterface: NOTE: JNI level: UNIT_COMMANDS: intentionally not wrapped: " $2);
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

END {
	# finalize things
	printJavaHeader();
	printJavaEnums();
	printJavaEnd();
}
