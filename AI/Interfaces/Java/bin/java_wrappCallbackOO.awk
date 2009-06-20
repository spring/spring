#!/bin/awk
#
# This awk script creates a Java class in OO style to wrapp the C style
# JNA based AI Callback wrapper.
# In other words, the output of this file wrapps:
# com/springrts/ai/AICallback.java
# which wrapps:
# rts/ExternalAI/Interface/SSkirmishAICallback.h
#
# This script uses functions from the following files:
# * common.awk
# * commonDoc.awk
# * commonOOCallback.awk
# Variables that can be set on th ecommand-line (with -v):
# * GENERATED_SOURCE_DIR: will contain the generated sources
#
# usage:
# 	awk -f thisScript.awk -f common.awk -f commonDoc.awk -f commonOOCallback.awk
# 	awk -f thisScript.awk -f common.awk -f commonDoc.awk -f commonOOCallback.awk -v 'GENERATED_SOURCE_DIR=/tmp/build/AI/Interfaces/Java/generated-java-src'
#

BEGIN {
	# initialize things

	# define the field splitter(-regex)
	#FS="[ \t]+";
	FS="(\\()|(\\)\\;)";
	IGNORECASE = 0;

	# These vars can be assigned externally, see file header.
	# Set the default values if they were not supplied on the command line.
	if (!GENERATED_SOURCE_DIR) {
		GENERATED_SOURCE_DIR = "../java/generated";
	}

	javaSrcRoot = "../java/src";
	javaGeneratedSrcRoot = GENERATED_SOURCE_DIR;

	myParentPkgA = "com.springrts.ai";
	myPkgA = myParentPkgA ".oo";
	myPkgD = convertJavaNameFormAToD(myPkgA);
	myClass = "OOAICallback";
	myClassVar = "ooClb";
	myWrapClass = "AICallback";
	myWrapVar = "innerCallback";
	mySourceFile = javaGeneratedSrcRoot "/" myPkgD "/" myClass ".java";
	MAX_IDS = 1024;
	defMapJavaImpl = "HashMap";

	myBufferedClasses["_UnitDef"] = 1;
	myBufferedClasses["_WeaponDef"] = 1;
	myBufferedClasses["_FeatureDef"] = 1;

	size_funcs = 0;
	size_classes = 0;
	size_interfaces = 0;
	# initialize all arrays
	#funcFullName[""] = 0;
	#funcRetType[""] = 0;
	#funcParams[""] = 0;
	#funcInnerParams[""] = 0;
	#class_ancestors[""] = 0;
}




function printHeader(outFile_h, javaPkg_h, javaClassName_h, isOrHasInterface_h) {

	classOrInterface_h = "class";
	implementedInterfacesPart_h = "";
	interfacesPref_h = "";

	if (isOrHasInterface_h == 1) {
		# is interface
		classOrInterface_h = "interface";
	} else {
		# is class
		interfacesPref_h = " implements "
		implementedInterfacesPart_h = "Comparable<" javaClassName_h ">";
		if (isOrHasInterface_h != 0) {
			# has interface
			implementedInterfacesPart_h = isOrHasInterface_h ", " implementedInterfacesPart_h;
		}
	}
	implementedInterfacesPart_h = interfacesPref_h implementedInterfacesPart_h;

	printCommentsHeader(outFile_h);
	print("") >> outFile_h;
	print("package " javaPkg_h ";") >> outFile_h;
	print("") >> outFile_h;
	print("") >> outFile_h;
	#print("import " myParentPkgA ".AIFloat3;") >> outFile_h;
	#print("import " myParentPkgA ".AICallback;") >> outFile_h;
	print("import " myParentPkgA ".*;") >> outFile_h;
	print("") >> outFile_h;
	print("/**") >> outFile_h;
	print(" * @author	AWK wrapper script") >> outFile_h;
	print(" * @version	GENERATED") >> outFile_h;
	print(" */") >> outFile_h;
	print("public " classOrInterface_h " " javaClassName_h implementedInterfacesPart_h " {") >> outFile_h;
	print("") >> outFile_h;
}

function createJavaFileName(clsName_c) {
	return javaGeneratedSrcRoot "/" myPkgD "/" clsName_c ".java";
}

function printInterfaces() {

	for (clsName in class_ancestors) {
		#size_ancestorParts = split(class_ancestors[clsName], ancestorParts, ",");

		# check if an interface is needed
		if (clsName in interfaces) {
			printInterface(clsName);
		}
	}
}
function printInterface(clsName_i) {

	outFile_i = createJavaFileName(clsName_i);
	printHeader(outFile_i, myPkgA, clsName_i, 1);

	size_addInds = additionalIntIndices[clsName_i "*"];

	# print member functions
	size_funcs = interfaceOwnerOfFunc[clsName_i "*"];
	for (f=0; f < size_funcs; f++) {
		fullName_i = interfaceOwnerOfFunc[clsName "#" f];
		printMember(outFile_i, fullName_i, size_addInds, 1);
	}

	# print member class fetchers (single, multi, multi-fetch-single)
	size_memCls = split(interface_class[clsName], memCls, ",");
	for (mc=0; mc < size_memCls; mc++) {
		memberClass = memCls[mc+1];
		fullNameMultiSize_i = ancestorsInterface_isMulti[clsName "-" memberClass];
		if (fullNameMultiSize_i != "") {
			if (match(fullNameMultiSize_i, /^.*0MULTI1[^0]*3/)) { # wants a different function name then the default one
				fn = fullNameMultiSize_i;
				sub(/^.*0MULTI1[^3]*3/, "", fn); # remove pre MULTI 3
				sub(/[0-9].*$/, "", fn); # remove post MULTI 3
			} else {
				fn = "get" memberClass "s";
			}
			params = funcParams[fullNameMultiSize_i];
		} else {
			fn = "get" memberClass;
			params = "";
		}
		# remove additional indices from params
		for (ai=0; ai < size_addInds; ai++) {
			sub(/int [_a-zA-Z0-9]+(, )?/, "", params);
		}
		print("\t" memberClass " " fn "(" params ");") >> outFile_i;
	}

	print("}") >> outFile_i;
	print("") >> outFile_i;
	close(outFile_i);
}





function printClasses() {

	for (clsName in class_ancestors) {
		size_ancestorParts = split(class_ancestors[clsName], ancestorParts, ",");

		# check if an interface is used/available
		#hasInterface = 0;
		#if (clsName in interfaces) {
		#	hasInterface = 1;
		#}
#if (match(clsName, /Clb/)) {
#print("clsName: " clsName);
#print("size_ancestorParts: " size_ancestorParts);
#print("class_ancestors[clsName]: " class_ancestors[clsName]);
#}
		if (size_ancestorParts == 0) {
			printClass("", clsName);
		}

		for (a=0; a < size_ancestorParts; a++) {
			printClass(ancestorParts[a+1], clsName);
		}
	}
}
function printClass(ancestors_c, clsName_c) {

	clsNameExternal_c = clsName_c;
	isClbRootCls = 0;
	myWrapper = myClassVar ".getInnerCallback()";
	myTeamId = myClassVar ".getTeamId()";
	myClassVarLocal = myClassVar;
	if (match(clsName_c, /^Clb$/)) {
		clsNameExternal_c = myClass;
		isClbRootCls = 1;
		myWrapper = myWrapVar;
		myTeamId = "teamId";
		myClassVarLocal = "this";
	}

	clsId_c = ancestors_c "-" clsName_c;
	clsFull_c = ancestors_c "_" clsName_c;
	hasInterface = (interfaces[clsName] == 1);
	intName_c = 0;
	if (hasInterface) {
		intName_c = clsName_c;
		clsName_c = implClsNames[clsId_c];
		clsNameExternal_c = clsName_c;
	}

	outFile_c = createJavaFileName(clsNameExternal_c);
	printHeader(outFile_c, myPkgA, clsNameExternal_c, intName_c);

	size_addInds = additionalClsIndices[clsId_c "*"];


	# print private vars
	if (isClbRootCls) {
		print("\t" "private " myWrapClass " " myWrapVar " = null;") >> outFile_c;
		print("\t" "private int teamId = -1;") >> outFile_c;
	} else {
		print("\t" "private " myClass " " myClassVar " = null;") >> outFile_c;
	}
	# print additionalVars
	for (ai=0; ai < size_addInds; ai++) {
		print("\t" "private int " additionalClsIndices[clsId_c "#" ai] " = -1;") >> outFile_c;
	}
	print("") >> outFile_c;


	# print constructor
	if (isClbRootCls) {
		ctorParams = myWrapClass " " myWrapVar ", int teamId";
	} else {
		ctorParams = myClass " " myClassVar;
	}
	addIndPars_c = "";
	for (ai=0; ai < size_addInds; ai++) {
		addIndPars_c = addIndPars_c ", int " additionalClsIndices[clsId_c "#" ai];
	}
	ctorParams = ctorParams addIndPars_c;
	ctorParamsNoTypes = removeParamTypes(ctorParams);
	sub(/^, /, "", addIndPars_c);
	addIndParsNoTypes_c = removeParamTypes(addIndPars_c);
	condaddIndPars_c = (addIndPars_c == "") ? "" : ", ";
	
	print("\t" "private " clsNameExternal_c "(" ctorParams ") {") >> outFile_c;
	print("") >> outFile_c;
	if (isClbRootCls) {
		print("\t\t" "this." myWrapVar " = " myWrapVar ";") >> outFile_c;
		print("\t\t" "this.teamId = teamId;") >> outFile_c;
	} else {
		print("\t\t" "this." myClassVar " = " myClassVar ";") >> outFile_c;
	}
	# init additionalVars
	for (ai=0; ai < size_addInds; ai++) {
		addIndName = additionalClsIndices[clsId_c "#" ai];
		print("\t\t" "this." addIndName " = " addIndName ";") >> outFile_c;
	}
	print("\t" "}") >> outFile_c;
	print("") >> outFile_c;
	
	# print additional vars fetchers
	for (ai=0; ai < size_addInds; ai++) {
		addIndName = additionalClsIndices[clsId_c "#" ai];
		#if (ai < (size_addInds-1)) {
			print("\t" "public int get" capitalize(addIndName) "() {") >> outFile_c;
		#} else { # do this for the last additional indices
		#	print("\t" "public int getId() {") >> outFile_c;
		#}
		print("\t\t" "return " addIndName ";") >> outFile_c;
		print("\t" "}") >> outFile_c;
		print("") >> outFile_c;
	}

	# print static instance fetcher method
	{
		clsIsBuffered_c = isBufferedClass(clsNameExternal_c);
		fullNameAvailable_c = ancestorsClass_available[clsId_c];

		if (clsIsBuffered_c) {
			print("\t" "private static java.util.Map<Integer, " clsNameExternal_c "> _buffer_instances = new java.util.HashMap<Integer, " clsNameExternal_c ">();") >> outFile_c;
			print("") >> outFile_c;
		}
		print("\t" "static " clsNameExternal_c " getInstance(" ctorParams ") {") >> outFile_c;
		print("") >> outFile_c;
		if (clsNameExternal_c == "Unit") {
			# the first valid unit id is 1
			print("\t\t" "if (unitId <= 0) {") >> outFile_c;
			print("\t\t\t" "return null;") >> outFile_c;
			print("\t\t" "}") >> outFile_c;
		} else if (match(ctorParams, /^[^ \t]+Id$/)) {
			# ... for all other *Id's, the first valid one is 0
			print("\t\t" "if (" ctorParams " < 0) {") >> outFile_c;
			print("\t\t\t" "return null;") >> outFile_c;
			print("\t\t" "}") >> outFile_c;
		}
		print("") >> outFile_c;
		print("\t\t" clsNameExternal_c " _ret = null;") >> outFile_c;
		if (fullNameAvailable_c == "") {
			print("\t\t" "_ret = new " clsNameExternal_c "(" ctorParamsNoTypes ");") >> outFile_c;
		} else {
			print("\t\t" "boolean isAvailable = " myClassVar ".getInnerCallback()." fullNameAvailable_c "(" myClassVar ".getTeamId()" condaddIndPars_c addIndParsNoTypes_c ");") >> outFile_c;
			print("\t\t" "if (isAvailable) {") >> outFile_c;
			print("\t\t\t" "_ret = new " clsNameExternal_c "(" ctorParamsNoTypes ");") >> outFile_c;
			print("\t\t" "}") >> outFile_c;
		}
		if (clsIsBuffered_c) {
			if (fullNameAvailable_c == "") {
				print("\t\t" "{") >> outFile_c;
			} else {
				print("\t\t" "if (_ret != null) {") >> outFile_c;
			}
			print("\t\t\t" "Integer indexHash = _ret.hashCode();") >> outFile_c;
			print("\t\t\t" "if (_buffer_instances.containsKey(indexHash)) {") >> outFile_c;
			print("\t\t\t\t" "_ret = _buffer_instances.get(indexHash);") >> outFile_c;
			print("\t\t\t" "} else {") >> outFile_c;
			print("\t\t\t\t" "_buffer_instances.put(indexHash, _ret);") >> outFile_c;
			print("\t\t\t" "}") >> outFile_c;
			print("\t\t" "}") >> outFile_c;
		}
		print("\t\t" "return _ret;") >> outFile_c;
		print("\t" "}") >> outFile_c;
		print("") >> outFile_c;
	}

	if (isClbRootCls) {
		# print inner-callback fetcher method
		print("\t" myWrapClass " getInnerCallback() {") >> outFile_c;
		print("\t\t" "return this." myWrapVar ";") >> outFile_c;
		print("\t" "}") >> outFile_c;
		print("") >> outFile_c;
		# print teamId fetcher method
		print("\t" "public int getTeamId() {") >> outFile_c;
		print("\t\t" "return this.teamId;") >> outFile_c;
		print("\t" "}") >> outFile_c;
		print("") >> outFile_c;
	}


	# print compareTo(other) method
	{
		print("\t" "@Override") >> outFile_c;
		print("\t" "public int compareTo(" clsNameExternal_c " other) {") >> outFile_c;
		print("\t\t" "final int BEFORE = -1;") >> outFile_c;
		print("\t\t" "final int EQUAL  =  0;") >> outFile_c;
		print("\t\t" "final int AFTER  =  1;") >> outFile_c;
		print("") >> outFile_c;
		print("\t\t" "if (this == other) return EQUAL;") >> outFile_c;
		print("") >> outFile_c;

		if (isClbRootCls) {
			print("\t\t" "if (this.teamId < other.teamId) return BEFORE;") >> outFile_c;
			print("\t\t" "if (this.teamId > other.teamId) return AFTER;") >> outFile_c;
			print("\t\t" "return EQUAL;") >> outFile_c;
		} else {
			for (ai=0; ai < size_addInds; ai++) {
				addIndName = additionalClsIndices[clsId_c "#" ai];
				print("\t\t" "if (this.get" capitalize(addIndName) "() < other.get" capitalize(addIndName) "()) return BEFORE;") >> outFile_c;
				print("\t\t" "if (this.get" capitalize(addIndName) "() > other.get" capitalize(addIndName) "()) return AFTER;") >> outFile_c;
			}
			print("\t\t" "return this." myClassVar ".compareTo(other." myClassVar ");") >> outFile_c;
		}
		print("\t" "}") >> outFile_c;
		print("") >> outFile_c;
	}


	# print equals(other) method
	if (!isClbRootCls) {
	#{
		print("\t" "@Override") >> outFile_c;
		print("\t" "public boolean equals(Object otherObject) {") >> outFile_c;
		print("") >> outFile_c;
		print("\t\t" "if (this == otherObject) return true;") >> outFile_c;
		print("\t\t" "if (!(otherObject instanceof " clsNameExternal_c ")) return false;") >> outFile_c;
		print("\t\t" clsNameExternal_c " other = (" clsNameExternal_c ") otherObject;") >> outFile_c;
		print("") >> outFile_c;

		if (isClbRootCls) {
			print("\t\t" "if (this.teamId != other.teamId) return false;") >> outFile_c;
			print("\t\t" "return true;") >> outFile_c;
		} else {
			for (ai=0; ai < size_addInds; ai++) {
				addIndName = additionalClsIndices[clsId_c "#" ai];
				print("\t\t" "if (this.get" capitalize(addIndName) "() != other.get" capitalize(addIndName) "()) return false;") >> outFile_c;
			}
			print("\t\t" "return this." myClassVar ".equals(other." myClassVar ");") >> outFile_c;
		}
		print("\t" "}") >> outFile_c;
		print("") >> outFile_c;
	}


	# print hashCode() method
	if (!isClbRootCls) {
	#{
		print("\t" "@Override") >> outFile_c;
		print("\t" "public int hashCode() {") >> outFile_c;
		print("") >> outFile_c;

		if (isClbRootCls) {
			print("\t\t" "int _res = 0;") >> outFile_c;
			print("") >> outFile_c;
			print("\t\t" "_res += this.teamId * 10E8;") >> outFile_c;
		} else {
			print("\t\t" "int _res = 23;") >> outFile_c;
			print("") >> outFile_c;
			# NOTE: This could go wrong if we have more then 7 additional indices
			# see 10E" (7-ai) below
			# the conversion to int is nessesarry,
			# as otherwise it would be a double,
			# which would be higher then max int,
			# and most hashes would end up being max int,
			# when converted from double to int
			for (ai=0; ai < size_addInds; ai++) {
				addIndName = additionalClsIndices[clsId_c "#" ai];
				print("\t\t" "_res += this.get" capitalize(addIndName) "() * (int) (10E" (7-ai) ");") >> outFile_c;
			}
			print("\t\t" "_res += this." myClassVar ".hashCode();") >> outFile_c;
		}
		print("") >> outFile_c;
		print("\t\t" "return _res;") >> outFile_c;
		print("\t" "}") >> outFile_c;
		print("") >> outFile_c;
	}


	# print toString() method
	{
		print("\t" "@Override") >> outFile_c;
		print("\t" "public String toString() {") >> outFile_c;
		print("") >> outFile_c;
		print("\t\t" "String _res = this.getClass().toString();") >> outFile_c;
		print("") >> outFile_c;

		if (isClbRootCls) {
			print("\t\t" "_res = _res + \"(teamId=\" + this.teamId + \", \";") >> outFile_c;
		} else {
			print("\t\t" "_res = _res + \"(clbHash=\" + this." myClassVar ".hashCode() + \", \";") >> outFile_c;
			print("\t\t" "_res = _res + \"teamId=\" + this." myClassVar ".getTeamId() + \", \";") >> outFile_c;
			for (ai=0; ai < size_addInds; ai++) {
				addIndName = additionalClsIndices[clsId_c "#" ai];
				print("\t\t" "_res = _res + \"" addIndName "=\" + this.get" capitalize(addIndName) "() + \", \";") >> outFile_c;
			}
		}
		print("\t\t" "_res = _res + \")\";") >> outFile_c;
		print("") >> outFile_c;
		print("\t\t" "return _res;") >> outFile_c;
		print("\t" "}") >> outFile_c;
		print("") >> outFile_c;
	}


	# print member functions
	size_funcs = ownerOfFunc[clsId_c "*"];
	for (f=0; f < size_funcs; f++) {
		fullName_c = ownerOfFunc[clsId_c "#" f];
		printMember(outFile_c, fullName_c, size_addInds, 0);
	}


	# print member class fetchers (single, multi, multi-fetch-single)
	size_memCls = split(ancestors_class[clsFull_c], memCls, ",");
	for (mc=0; mc < size_memCls; mc++) {
		memberClass_c = memCls[mc+1];
		printMemberClassFetchers(outFile_c, clsFull_c, clsId_c, memberClass_c, isInterface_c);
	}


	print("}") >> outFile_c;
	print("") >> outFile_c;
	close(outFile_c);
}

function printMemberClassFetchers(outFile_mc, clsFull_mc, clsId_mc, memberClsName_mc, isInterface_mc) {

		memberClassId_mc = clsFull_mc "-" memberClsName_mc;
		size_multi_mc = ancestorsClass_multiSizes[memberClassId_mc "*"];
		isMulti_mc = (size_multi_mc != "");
		if (isMulti_mc) { # multi element fetcher(s)
			for (mmc=0; mmc < size_multi_mc; mmc++) {
				fullNameMultiSize_mc = ancestorsClass_multiSizes[memberClassId_mc "#" mmc];
				printMemberClassFetcher(outFile_mc, clsFull_mc, clsId_mc, memberClsName_mc, isInterface_mc, fullNameMultiSize_mc);
			}
		} else { # single element fetcher
			printMemberClassFetcher(outFile_mc, clsFull_mc, clsId_mc, memberClsName_mc, isInterface_mc, 0);
		}
}
# fullNameMultiSize_mf is 0 if it is no multi element
function printMemberClassFetcher(outFile_mf, clsFull_mf, clsId_mf, memberClsName_mf, isInterface_mf, fullNameMultiSize_mf) {

		memberClassId_mf = clsFull_mf "-" memberClsName_mf;
		isMulti_mf = fullNameMultiSize_mf != 0;
		if (interfaces[memberClsName_mf] != "") {
			memberClassImpl_mf = implClsNames[memberClassId_mf];
		} else {
			memberClassImpl_mf = memberClsName_mf;
		}
		if (isMulti_mf) { # multi element fetcher
			if (match(fullNameMultiSize_mf, /^.*0MULTI1[^0]*3/)) { # wants a different function name then the default one
				fn = fullNameMultiSize_mf;
				sub(/^.*0MULTI1[^3]*3/, "", fn); # remove pre MULTI 3
				sub(/[0-9].*$/, "", fn); # remove post MULTI 3
			} else {
				fn = memberClsName_mf "s";
			}
			fn = "get" fn;

			params = funcParams[fullNameMultiSize_mf];
			innerParams = funcInnerParams[fullNameMultiSize_mf];

			fullNameMultiVals_mf = fullNameMultiSize_mf;
			sub(/0MULTI1SIZE/, "0MULTI1VALS", fullNameMultiVals_mf);
			hasMultiVals_mf = (funcRetType[fullNameMultiVals_mf] != "");
		} else { # single element fetcher
			fn = "get" memberClsName_mf;
			params = "";
		}

		# remove additional indices from params
		for (ai=0; ai < size_addInds; ai++) {
			sub(/int [_a-zA-Z0-9]+(, )?/, "", params);
		}

		retType = memberClsName_mf;
		if (isMulti_mf) {
			retTypeInterface = "java.util.List<" retType ">";
			retType = "java.util.ArrayList<" retType ">";
		} else {
			retTypeInterface = retType;
		}
		condInnerParamsComma = (innerParams == "") ? "" : ", ";

		# concatenate additional indices
		indexParams_mf = "";
		for (ai=0; ai < size_addInds; ai++) {
			addIndName = additionalClsIndices[clsId_mf "#" ai];
			indexParams_mf = indexParams_mf  ", " addIndName;
		}
		sub(/^\, /, "", indexParams_mf); # remove comma at the end
		condIndexComma_mf = (indexParams_mf == "") ? "" : ", ";

		indent_mf = "\t";

		isBuffered_mf = isBufferedFunc(clsFull_mf "_" memberClsName_mf);
		if (!isInterface_mf && isBuffered_mf) {
			print(indent_mf "private " retType " _buffer_" fn ";") >> outFile_mf;
			print(indent_mf "private boolean _buffer_isInitialized_" fn " = false;") >> outFile_mf;
		}

		printFunctionComment_Common(outFile_mf, funcDocComment, fullNameMultiSize_mf, indent_mf);

		print(indent_mf "public " retTypeInterface " " fn "(" params ") {") >> outFile_mf;
		print("") >> outFile_mf;
		indent_mf = indent_mf "\t";
		if (isBuffered_mf) {
			print(indent_mf retType " _ret = _buffer_" fn ";") >> outFile_mf;
			print(indent_mf "if (!_buffer_isInitialized_" fn ") {") >> outFile_mf;
			indent_mf = indent_mf "\t";
		} else {
			print(indent_mf retType " _ret;") >> outFile_mf;
		}
		if (isMulti_mf) {
			print(indent_mf "int size = " myWrapper "." fullNameMultiSize_mf "(" myTeamId condInnerParamsComma innerParams ");") >> outFile_mf;
			if (hasMultiVals_mf) {
				print(indent_mf "int[] tmpArr = new int[size];") >> outFile_mf;
				print(indent_mf myWrapper "." fullNameMultiVals_mf "(" myTeamId condInnerParamsComma innerParams ", tmpArr, size);") >> outFile_mf;
				print(indent_mf retType " arrList = new " retType "(size);") >> outFile_mf;
				print(indent_mf "for (int i=0; i < size; i++) {") >> outFile_mf;
				print(indent_mf "\t" "arrList.add(" memberClassImpl_mf ".getInstance(" myClassVarLocal condIndexComma_mf indexParams_mf ", tmpArr[i]));") >> outFile_mf;
				print(indent_mf "}") >> outFile_mf;
				print(indent_mf "_ret = arrList;") >> outFile_mf;
			} else {
				print(indent_mf retType " arrList = new " retType "(size);") >> outFile_mf;
				print(indent_mf "for (int i=0; i < size; i++) {") >> outFile_mf;
				print(indent_mf "\t" "arrList.add(" memberClassImpl_mf ".getInstance(" myClassVarLocal condIndexComma_mf indexParams_mf ", i));") >> outFile_mf;
				print(indent_mf "}") >> outFile_mf;
				print(indent_mf "_ret = arrList;") >> outFile_mf;
			}
		} else {
			print(indent_mf "_ret = " memberClassImpl_mf ".getInstance(" myClassVarLocal condIndexComma_mf indexParams_mf ");") >> outFile_mf;
		}
		if (isBuffered_mf) {
			print(indent_mf "_buffer_" fn " = _ret;") >> outFile_mf;
			print(indent_mf "_buffer_isInitialized_" fn " = true;") >> outFile_mf;
			sub(/\t/, "", indent_mf);
			print(indent_mf "}") >> outFile_mf;
			print("") >> outFile_mf;
		}
		print(indent_mf "return _ret;") >> outFile_mf;
		sub(/\t/, "", indent_mf);
		print("\t" "}") >> outFile_mf;
}





function printMember(outFile_m, fullName_m, additionalIndices_m, isInterface_m) {

	if (part_isMultiValues(fullName_m) || part_isArrayValues(fullName_m) || part_isMapKeys(fullName_m) || part_isMapValues(fullName_m)) {
		return;
	}

	indent_m = "\t";
	retType = funcRetType[fullName_m];
	retTypeInterface = "";
	params = funcParams[fullName_m];
	innerParams = funcInnerParams[fullName_m];
	memName = extractNormalPart(fullName_m);
	sub(/^.*_/, "", memName);
	isVoid_m = (retType == "void");
	isBuffered_m = !isVoid_m && isBufferedFunc(fullName_m);

	if (memName == "handleCommand") {
		sub(/int commandTopic\, Pointer commandData/, "AICommand command", params);
		sub(/commandTopic\, commandData/, "command.getTopic(), command.getPointer()", innerParams);
	}

#print("fullName_m: " fullName_m);
#print("additionalIndices_m: " additionalIndices_m);

	isArraySize = part_isArraySize(fullName_m);
	if (isArraySize) {
		fullNameArraySize = fullName_m;

		fullNameArrayVals = fullNameArraySize;
		sub(/0ARRAY1SIZE/, "0ARRAY1VALS", fullNameArrayVals);
		params = funcParams[fullNameArrayVals];

		sub(/\, int [_a-zA-Z0-9]+$/, "", params); # remove max
		arrayType = params; # getArrayType
		sub(/^.*\, /, "", arrayType); # remove pre array type
		hadBraces = sub(/\[\] .*$/, "", arrayType); # remove post array type
		if (!hadBraces) {
			sub(/[^ \t]*$/, "", arrayType); # remove param name
		}
		referenceType = arrayType;
		if (match(fullNameArraySize, /0ARRAY1SIZE1/)) {
			# we want to reference objects, of a certain class as arrayType
			referenceType = fullNameArraySize;
			sub(/^.*0ARRAY1SIZE1/, "", referenceType); # remove pre ref array type
			sub(/[0123].*$/, "", referenceType); # remove post ref array type
		}
		arrType_java = convertJavaBuiltinTypeToClass(referenceType);
		retTypeInterface = "java.util.List<" arrType_java ">";
		retType = "java.util.ArrayList<" arrType_java ">";
		sub(/(\, )?[^ ]+ [_a-zA-Z0-9]+$/, "", params); # remove array
	}

	isMapSize = part_isMapSize(fullName_m);
	if (isMapSize) {
		fullNameMapSize = fullName_m;

		fullNameMapKeys = fullNameMapSize;
		sub(/0MAP1SIZE/, "0MAP1KEYS", fullNameMapKeys);
#print("fullNameMapKeys: " fullNameMapKeys) >> outFile_m;
		keyType = funcParams[fullNameMapKeys];
		sub(/\[\].*$/, "", keyType); # remove everything after array type
		sub(/^.* /, "", keyType); # remove everything before array type

		fullNameMapVals = fullNameMapSize;
		sub(/0MAP1SIZE/, "0MAP1VALS", fullNameMapVals);
		valType = funcParams[fullNameMapVals];
		sub(/\[\].*$/, "", valType); # remove everything after array type
		sub(/^.* /, "", valType); # remove everything before array type

		sub(/\, int [_a-zA-Z0-9]+$/, "", params); # remove max
		retType = "java.util.Map<" convertJavaBuiltinTypeToClass(keyType) ", " convertJavaBuiltinTypeToClass(valType) ">"; # getArrayType
		#sub(/^.*\, /, "", retType);
		#sub(/ .*$/, "", retType);
		sub(/\, [^ ]+ [_a-zA-Z0-9]+$/, "", params); # remove array
	}

	isSingleFetch = part_isSingleFetch(fullName_m);
	if (isSingleFetch) {
		fetchClass_m = fullName_m;
		sub(/0[^0]*$/, "", fetchClass_m); # remove everything after array type
		sub(/.*0SINGLE1FETCH[^2]*2/, "", fetchClass_m); # remove everything before array type
		innerRetType = retType;
		retType = fetchClass_m;

		indName_m = additionalClsIndices[clsId_c "#" (additionalClsIndices[clsId_c "*"]-1)];
		#instanceInnerParams = innerParams;
		#sub("(, )?" indName_m, "", instanceInnerParams); # remove index parameter
		instanceInnerParams = "";
	}

	refObjsFullName_m = fullName_m;
	hasReferenceObject_m = part_hasReferenceObject(refObjsFullName_m);
#print("hasRef: " hasReferenceObject_m " / " fullName_m);
	while (hasReferenceObject_m) {
#0REF1Resource2resourceId0
		refObj_m = refObjsFullName_m;
		sub(/^.*0REF/, "", refObj_m); # remove everything before ref type
		sub(/0.*$/, "", refObj_m); # remove everything after ref type

		refCls_m = refObj_m;
		sub(/^[^1]*1/, "", refCls_m); # remove everything before ref cls
		sub(/[123].*$/, "", refCls_m); # remove everything after ref cls

		refParamName_m = refObj_m;
		sub(/^[^2]*2/, "", refParamName_m); # remove everything before ref param name
		sub(/[123].*$/, "", refParamName_m); # remove everything after ref param name
#print("Ref: " refCls_m " / " refParamName_m " / " refObj_m " / " fullName_m);

		#refParamNameClass_m[refParamName_m] = refCls_m; # eg: refParamNameClass_m["resourceId"] = "Resource"
		sub("int " refParamName_m, refCls_m " c_" refParamName_m, params); # remove everything before ref param name
		sub(refParamName_m, "c_" refParamName_m ".get" capitalize(refParamName_m) "()", innerParams); # remove everything before ref param name

		sub("0REF" refObj_m, "", refObjsFullName_m); # remove everything after array type
		hasReferenceObject_m = part_hasReferenceObject(refObjsFullName_m);
	}

	# remove additional indices from params
	for (ai=0; ai < additionalIndices_m; ai++) {
		sub(/int [_a-zA-Z0-9]+(, )?/, "", params);
	}

	firstLineEnd = ";";
	mod_m = "";
	if (!isInterface_m) {
		firstLineEnd = " {";
		mod_m = "public ";
	}

	retType = trim(retType);
	if (retTypeInterface == "") {
		retTypeInterface = retType;
	}

	if (!isInterface_m && isBuffered_m) {
		print(indent_m retType " _buffer_" memName ";") >> outFile_m;
		print(indent_m "boolean _buffer_isInitialized_" memName " = false;") >> outFile_m;
	}

	printFunctionComment_Common(outFile_m, funcDocComment, fullName_m, indent_m);

	print(indent_m mod_m retTypeInterface " " memName "(" params ")" firstLineEnd) >> outFile_m;
	if (!isInterface_m) {
		condRet = isVoid_m ? "" : "_ret = ";
		condInnerParamsComma = (innerParams == "") ? "" : ", ";
		indent_m = indent_m "\t";
		if (memName == "handleCommand") {
			print(indent_m "command.write();") >> outFile_m;
		}
		if (isBuffered_m) {
			print(indent_m retType " _ret = _buffer_" memName ";") >> outFile_m;
			print(indent_m "if (!_buffer_isInitialized_" memName ") {") >> outFile_m;
			indent_m = indent_m "\t";
		} else if (!isVoid_m) {
			print(indent_m retType " _ret;") >> outFile_m;
		}
		if (isArraySize) {
			print("") >> outFile_m;
			print(indent_m "int size = " myWrapper "." fullNameArraySize "(" myTeamId condInnerParamsComma innerParams ");") >> outFile_m;
			print(indent_m retType " arrList = new " retType "(size);") >> outFile_m;
			print(indent_m "if (size > 0) {") >> outFile_m;
			indent_m = indent_m "\t";
			print(indent_m arrayType "[] tmpArr = new " arrayType "[size];") >> outFile_m;
			print(indent_m myWrapper "." fullNameArrayVals "(" myTeamId condInnerParamsComma innerParams ", tmpArr, size);") >> outFile_m;
			print(indent_m "for (int i=0; i < size; i++) {") >> outFile_m;
			indent_m = indent_m "\t";
			if (arrayType == referenceType) {
				print(indent_m "arrList.add(tmpArr[i]);") >> outFile_m;
			} else {
				print(indent_m "arrList.add(" referenceType ".getInstance(" myClassVarLocal ", tmpArr[i]));") >> outFile_m;
			}
			sub(/\t/, "", indent_m);
			print(indent_m "}") >> outFile_m;
			sub(/\t/, "", indent_m);
			print(indent_m "}") >> outFile_m;
			print(indent_m "_ret = arrList;") >> outFile_m;
		} else if (isMapSize) {
			print("") >> outFile_m;
			print(indent_m "int size = " myWrapper "." fullNameMapSize "(" myTeamId condInnerParamsComma innerParams ");") >> outFile_m;
			retMapImplType = retType;
			sub(/Map/, defMapJavaImpl, retMapImplType);
			print(indent_m retType " retMap = new " retMapImplType "(size);") >> outFile_m;
			print(indent_m "if (size > 0) {") >> outFile_m;
			indent_m = indent_m "\t";
			print(indent_m keyType "[] tmpKeysArr = new " keyType "[size];") >> outFile_m;
			print(indent_m myWrapper "." fullNameMapKeys "(" myTeamId condInnerParamsComma innerParams ", tmpKeysArr);") >> outFile_m;
			print(indent_m valType "[] tmpValsArr = new " valType "[size];") >> outFile_m;
			print(indent_m myWrapper "." fullNameMapVals "(" myTeamId condInnerParamsComma innerParams ", tmpValsArr);") >> outFile_m;
			print(indent_m "for (int i=0; i < size; i++) {") >> outFile_m;
			print(indent_m "\t" "retMap.put(tmpKeysArr[i], tmpValsArr[i]);") >> outFile_m;
			print(indent_m "}") >> outFile_m;
			sub(/\t/, "", indent_m);
			print(indent_m "}") >> outFile_m;
			print(indent_m "_ret = retMap;") >> outFile_m;
		} else if (isSingleFetch) {
			condInstanceInnerParamsComma = (instanceInnerParams == "") ? "" : ", ";
			print("") >> outFile_m;
			print(indent_m innerRetType " innerRet = " myWrapper "." fullName_m "(" myTeamId condInnerParamsComma innerParams ");") >> outFile_m;
			print(indent_m "_ret = " retType ".getInstance(" myClassVarLocal ", innerRet" condInstanceInnerParamsComma instanceInnerParams ");") >> outFile_m;
		} else {
			print(indent_m condRet myWrapper "." fullName_m "(" myTeamId condInnerParamsComma innerParams ");") >> outFile_m;
		}
		if (isBuffered_m) {
			print(indent_m "_buffer_" memName " = _ret;") >> outFile_m;
			print(indent_m "_buffer_isInitialized_" memName " = true;") >> outFile_m;
			sub(/\t/, "", indent_m);
			print(indent_m "}") >> outFile_m;
			print("") >> outFile_m;
		}
		if (!isVoid_m) {
			print(indent_m "return _ret;") >> outFile_m;
		}
		sub(/\t/, "", indent_m);
		print(indent_m "}") >> outFile_m;
	}
}



# This function has to return true (1) if a doc comment (eg: /** foo bar */)
# can be deleted.
# If there is no special condition you want to apply,
# it should always return true (1),
# cause there are additional mechanism to prevent accidential deleting.
# see: commonDoc.awk
function canDeleteDocumentation() {
	return isMultiLineFunc != 1;
}


# grab callback functions info
# 2nd, 3rd, ... line of a function definition
{
	if (isMultiLineFunc) { # function is defined on one single line
		funcIntermLine = $0;
		# remove possible comment at end of line: // fu bar
		sub(/[ \t]*\/\/.*/, "", funcIntermLine);
		funcIntermLine = trim(funcIntermLine);
		funcSoFar = funcSoFar " " funcIntermLine;
		if (match(funcSoFar, /\;$/)) {
			# function ends in this line
			wrappFunctionDef(funcSoFar);
			isMultiLineFunc = 0;
		}
	}
}
# 1st line of a function definition
#/^\t[^ ]+ Clb_[^ ]+\(int teamId.*\)\;$/ {
/Clb_/ {

	funcStartLine = $0;
	# remove possible comment at end of line: // fu bar
	sub(/\/\/.*$/, "", funcStartLine);
	funcStartLine = trim(funcStartLine);
	if (match(funcStartLine, /\;$/)) {
		# function ends in this line
		wrappFunctionDef(funcStartLine);
	} else {
		funcSoFar = funcStartLine;
		isMultiLineFunc = 1;
	}
}

function wrappFunctionDef(funcDef) {

	size_funcParts = split(funcDef, funcParts, "(\\()|(\\)\\;)");
	# because the empty part after ");" would count as part aswell
	size_funcParts--;

	fullName = funcParts[1];
	fullName = trim(fullName);
	sub(/.*[ \t]+/, "", fullName);

	retType = funcParts[1];
	sub(fullName, "", retType);
	retType = trim(retType);

	params = funcParts[2];

	wrappFunction(retType, fullName, params);
}


END {
	# finalize things
	storeClassesAndInterfaces();
	printInterfaces();
	printClasses();
}
