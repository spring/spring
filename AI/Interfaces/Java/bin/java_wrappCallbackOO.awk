#!/bin/awk
#
# This awk script creates a Java class in OO style to wrapp the C style
# JNA based AI Callback wrapper.
# In other words, the output of this file wrapps:
# com/clan_sy/spring/ai/AICallback.java
# which wrapps:
# rts/ExternalAI/Interface/SAICallback.h
#

BEGIN {
	# initialize things

	# define the field splitter(-regex)
	#FS="[ \t]+";
	FS="(\\()|(\\)\\;)";
	IGNORECASE = 0;

	javaSrcRoot = "../java/src";

	myParentPkgA = "com.clan_sy.spring.ai";
	myPkgA = myParentPkgA ".oo";
	myPkgD = convertJavaNameFormAToD(myPkgA);
	myClass = "OOAICallback";
	myClassVar = "ooClb";
	myWrapClass = "AICallback";
	myWrapVar = "innerCallback";
	mySourceFile = javaSrcRoot "/" myPkgD "/" myClass ".java";
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


# Some utility functions

function ltrim(s) { sub(/^[ \t]+/, "", s); return s; }
function rtrim(s) { sub(/[ \t]+$/, "", s); return s; }
function trim(s)  { return rtrim(ltrim(s)); }

function noSpaces(s)  { gsub(/[ \t]/, "", s); return s; }

function capitalize(s)  { return toupper(substr(s, 1, 1)) substr(s, 2); }
function lowerize(s)  { return tolower(substr(s, 1, 1)) substr(s, 2); }

function startsWithCapital(s) { return match(s, /^[ABCDEFGHIJKLMNOPQRDTUVWXYZ]/); }
function startsWithLower(s) { return match(s, /^[abcdefghijklmnopqrdtuvwxyz]/); }

# sort function -- sorts an array based on values (wether numbers or strings)
# in ascending order; first array element at index [1]
function mySort(array, size, temp, i, j) {

	for (i = 2; i <= size; ++i) {
		for (j = i; array[j-1] > array[j]; --j) {
			temp = array[j];
			array[j] = array[j-1];
			array[j-1] = temp;
		}
	}
}

# Awaits this format:	com.clan_sy.spring.ai
# Returns this format:	com/clan_sy/spring/ai
function convertJavaNameFormAToD(javaNameFormA) {

	javaNameFormD = javaNameFormA;

	gsub(/\./, "/", javaNameFormD);

	return javaNameFormD;
}

# Awaits this format:	int / float / String
# Returns this format:	Integer / Float / String
function convertJavaBuiltinTypeToClass(javaBuiltinType_bt) {

	javaClassType_bt = javaBuiltinType_bt;

	sub(/int/, "Integer", javaClassType_bt);
	sub(/float/, "Float", javaClassType_bt);

	sub(/boolean/, "Boolean", javaClassType_bt);
	sub(/byte/, "Byte", javaClassType_bt);
	sub(/char/, "Character", javaClassType_bt);
	sub(/double/, "Double", javaClassType_bt);
	sub(/float/, "Float", javaClassType_bt);
	sub(/int/, "Integer", javaClassType_bt);
	sub(/long/, "Long", javaClassType_bt);
	sub(/short/, "Short", javaClassType_bt);

	return javaClassType_bt;
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


function printCommentsHeader(outFile) {

	printGeneratedWarningHeader(outFile);
	print("") >> outFile;
	printGPLHeader(outFile);
}

function printHeader(outFile_h, javaPkg_h, javaClassName_h, isOrHasInterface_h) {

	classOrInterface = "class";
	implementedInterfacePart = "";
	if (isOrHasInterface_h == 1) {
		classOrInterface = "interface";
	} else if (isOrHasInterface_h != 0) {
		implementedInterfacePart = " implements " isOrHasInterface_h;
	}

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
	print("public " classOrInterface " " javaClassName_h implementedInterfacePart " {") >> outFile_h;
	print("") >> outFile_h;
}



function removeParamTypes(params) {

	innerParams = params;

	sub(/^[^ ]* /, "", innerParams);
	gsub(/, [^ ]*/, ",", innerParams);

	return innerParams;
}

function matchesAnyKey(toSearch, matchArray) {

	for (pattern in matchArray) {
		if (match(toSearch, pattern)) {
			return 1;
		}
	}

	return 0;
}

function isBufferedFunc(funcFullName_b) {

	np_b = extractNormalPart(funcFullName_b);

	return matchesAnyKey(np_b, myBufferedClasses);
}

function isBufferedClass(clsName_bc) {

	np_bc = extractNormalPart(clsName_bc);

	return matchesAnyKey(np_bc, myBufferedClasses);
}

function extractNormalPart(namePart_a) {

	normalPart_a = namePart_a;

	do {
		replaced_a = sub(/0[^0]*0/, "0", normalPart_a);
	} while (replaced_a);

	sub(/0/, "", normalPart_a);

	return normalPart_a;
}


function createJavaFileName(clsName_c) {
	return javaSrcRoot "/" myPkgD "/" clsName_c ".java";
}

function part_isClass(namePart_p) {
	sub(/0.*0/, "", namePart_p);
	return startsWithCapital(namePart_p);
}
function part_isStatic(namePart_p) {
	return match(namePart_p, /0STATIC0/);
}
function part_isArraySize(namePart_p) {
	return match(namePart_p, /0ARRAY1SIZE[01]/);
}
function part_isArrayValues(namePart_p) {
	return match(namePart_p, /0ARRAY1VALS[01]/);
}
function part_isMapSize(namePart_p) {
	return match(namePart_p, /0MAP1SIZE0/);
}
function part_isMapKeys(namePart_p) {
	return match(namePart_p, /0MAP1KEYS0/);
}
function part_isMapValues(namePart_p) {
	return match(namePart_p, /0MAP1VALS0/);
}
function part_isMultiSize(namePart_p) {
	return match(namePart_p, /0MULTI1SIZE[0123]/);
}
function part_isMultiValues(namePart_p) {
	return match(namePart_p, /0MULTI1SIZE[0123]/);
}
function part_isMultiFetch(namePart_p) {
	return match(namePart_p, /0MULTI1FETCH[0123]/);
}
function part_isSingleFetch(namePart_p) {
	return match(namePart_p, /0SINGLE1FETCH[0123]/);
}
function part_isAvailable(namePart_p) {
	return match(namePart_p, /0AVAILABLE0/);
}
function part_hasReferenceObject(namePart_p) {
	return match(namePart_p, /0REF[^0]*0/);
}
function part_getClsName(ancestorsPlusName_p) {

	normalAPN_p = extractNormalPart(ancestorsPlusName_p);

	if (ancestorsPlusName_clsName[normalAPN_p] != "") {
		return ancestorsPlusName_clsName[normalAPN_p];
	}

	clsName_a = ancestorsPlusName_p;

	isClsMemberNameDifferent = match(ancestorsPlusName_p, /0MULTI1SIZE[^0]*2[_a-zA-Z]+[^0]*0/);
#if (match(normalAPN_p, /Clb/)) { print("normalAPN_p: " normalAPN_p); }
	if (isClsMemberNameDifferent) {
		sub(/^.*0MULTI1SIZE[^2]2*/, "", clsName_a);
		sub(/[0123].*$/, "", clsName_a);
	} else {
		sub(/^.*_/, "", clsName_a);
		clsName_a = extractNormalPart(clsName_a);
	}

	ancestorsPlusName_clsName[normalAPN_p] = clsName_a;

	return clsName_a;
}

function store_class(ancestors_s, clsName_s) {

#if (match(clsName_s, /Clb$/)) { print("clsName_s: " clsName_s); }
#if (match(ancestors_s, /Clb$/)) { print("ancestors_s: " ancestors_s); }
	if (!(clsName_s in class_ancestors)) {
		class_ancestors[clsName_s] = ancestors_s;
	} else if (!match(class_ancestors[clsName_s], "((^)|(,))" ancestors_s "(($)|(,))")) {
	#} else if (!match(class_ancestors[clsName_s], ancestors_s)) {
		class_ancestors[clsName_s] = class_ancestors[clsName_s] "," ancestors_s;
	}

	if (!(ancestors_s in ancestors_class)) {
		ancestors_class[ancestors_s] = clsName_s;
	} else if (!match(ancestors_class[ancestors_s], "((^)|(,))" clsName_s "(($)|(,))")) {
	#} else if (!match(ancestors_class[ancestors_s], clsName_s)) {
		ancestors_class[ancestors_s] = ancestors_class[ancestors_s] "," clsName_s;
	}
}




function storeClassesAndInterfaces() {

	mySort(funcFullNames);

	additionalClsIndices["*"] = 0;
	additionalClsIndices["-" myClass "*"] = 0;

	# store classes and assign functions
	for (f=0; f < size_funcs; f++) {
		fullName = funcFullName[f];
		retType = funcRetType[fullName];
		params = funcParams[fullName];
		innerParams = funcInnerParams[fullName];

		size_nameParts = split(fullName, nameParts, "_");

		# go through the classes
		ancestorsP = "";
		last_ancestorsP = "";
		clsName = myClass;
		clsId = ancestorsP "-" clsName;
		last_clsId = "";
		for (np=0; np < size_nameParts-1; np++) {
			last_clsName = clsName;
			last_clsId = clsId;

			nameP = nameParts[np+1];
#if (match(nameP, /Clb/)) { print("nameP: " nameP); }
			normalP = extractNormalPart(nameP);

			# register class
			clsName = part_getClsName(ancestorsP "_" nameP);
			#if (clsName == myClass) { normalP = clsName; }
			clsId = ancestorsP "-" clsName;
			store_class(ancestorsP, clsName);
#if (match(ancestorsP, /Clb$/)) { print("ancestorsP/clsName: " ancestorsP " / " clsName); }

			if (additionalClsIndices[clsId "*"] == "") {
				additionalClsIndices[clsId "*"] = additionalClsIndices[last_clsId "*"];
#print("additionalClsIndices I init : " clsId " / "  last_clsId " / " additionalClsIndices[last_clsId "*"]);
			}
			if (part_isMultiSize(nameP)) {
				parentNumInd = additionalClsIndices[last_clsId "*"];
				#size_paramNames = split(innerParams, paramNames, ", ");
				#indName = paramNames[1 + parentNumInd];
				#additionalClsIndices[clsId "#" parentNumInd] = indName;
				additionalClsIndices[clsId "*"] = parentNumInd + 1;
#print("additionalClsIndices I ++ : " clsId " / "  last_clsId " / " additionalClsIndices[clsId "*"]);
			}
			
			secondLast_clsName = last_clsName;
			last_ancestorsP = ancestorsP;
			ancestorsP = ancestorsP "_" normalP;
#print("clsName: " clsName);
#print("ancestorsP: " ancestorsP);
#print("last_ancestorsP: " last_ancestorsP);
		}

		nameP = nameParts[size_nameParts];
		normalP = extractNormalPart(nameP);
		secondLast_clsName = last_clsName;
		last_clsName = clsName;
		clsName = part_getClsName(ancestorsP "_" nameP);
		last_clsId = clsId;
		clsId = ancestorsP "-" clsName;

		isClass = part_isClass(normalP);

#TODO: remove this
#if (match(fullName, /BuildOption/)) {
#	print isClass;
#}
#print("normalP: " normalP);
#print("startsWithCapital X: " match("X", /^[ABCDEFGHIJKLMNOPQRDTUVWXYZ]/));
#print("startsWithCapital X: " tolower("ABCDEFGHIJKLMNOPQRDTUVWXYZ"));
#print("startsWithCapital x: " match("x", /^[ABCX]/));
#print("startsWithLower X: " match("X", /^[a-z]/));
#print("startsWithLower x: " match("x", /^[a-z]/));
#print("startsWithCapital 3: " match("3", /^[A-Z]/));
#print("startsWithLower 3: " match("3", /^[a-z]/));
		if (isClass) {
#print("isClass == true");
			if (additionalClsIndices[clsId "*"] == "") {
				additionalClsIndices[clsId "*"] = additionalClsIndices[last_clsId "*"];
#print("additionalClsIndices init : " clsId " / "  last_clsId " / " additionalClsIndices[last_clsId "*"]);
			}
			if (part_isMultiSize(nameP)) {
#print("part_isMultiSize clsId fullName : " clsId " / " fullName);
				if (ancestorsClass_multiSizes[clsId "*"] == "") {
					#ancestorsClass_isMulti[clsId] = 1;
					ancestorsClass_multiSizes[clsId "*"] = 0;
					size_parentInd = additionalClsIndices[last_clsId "*"];
					#size_paramNames = split(innerParams, paramNames, ", ");
					#indName = paramNames[1 + size_parentInd];
					#additionalClsIndices[clsId "#" size_parentInd] = indName;
					additionalClsIndices[clsId "#" size_parentInd] = "";
					additionalClsIndices[clsId "*"] = size_parentInd + 1;
#print("additionalClsIndices ++ : " clsId " / "  last_clsId " / " additionalClsIndices[clsId "*"] " / " indName " / " innerParams);
				}

				ancestorsClass_multiSizes[clsId "#" ancestorsClass_multiSizes[clsId "*"]] = fullName;
#print("part_isMultiSize clsId/i/fullName: " clsId " / " ancestorsClass_multiSizes[clsId "*"] " / " fullName);
				#additionalClsIndices[clsId "*"] = additionalClsIndices[last_clsId "*"] + 1;

				ancestorsClass_multiSizes[clsId "*"]++;
			} else if (part_isAvailable(nameP)) {
				ancestorsClass_available[clsId] = fullName;
			}
		} else {
#print("isClass == false");
			# store names of additional parameters
			size_parentInd = additionalClsIndices[last_clsId "*"];
#print("a_additionalClsIndices[last_clsId # (size_parentInd-1)]: " additionalClsIndices[last_clsId "#" (size_parentInd-1)]);
			if (additionalClsIndices[last_clsId "#" (size_parentInd-1)] == "") {
				size_paramNames = split(innerParams, paramNames, ", ");
				for (pci=0; pci < size_parentInd; pci++) {
					if (additionalClsIndices[last_clsId "#" pci] == "") {
						indName = paramNames[1 + pci];
#print("a_indName: " indName);
						additionalClsIndices[last_clsId "#" pci] = indName;
					}
				}
			}

			# assign the function to a class
			#isStatic = part_isStatic(fullName);
			isStatic = part_isStatic(nameP);
			if (isStatic) {
				belongsTo = secondLast_clsName;
				ancest = last_ancestorsP;
			} else {
				belongsTo = last_clsName;
				ancest = last_ancestorsP;
			}
			# store one way ...
			funcBelongsTo[fullName] = ancest "-" belongsTo;

			# ... and the other
			sizeI_belongsTo = ancest "-" belongsTo "*";
			if (ownerOfFunc[sizeI_belongsTo] == "") {
				ownerOfFunc[sizeI_belongsTo] = 0;
			}
			ownerOfFunc[ancest "-" belongsTo "#" ownerOfFunc[sizeI_belongsTo]] = fullName;
	#print("ancest - belongsTo # ownerOfFunc[sizeI_belongsTo]: " ancest "-" belongsTo "#" ownerOfFunc[sizeI_belongsTo] " / " fullName);
	#print("clsName: " clsName);
	#print("last_clsName: " last_clsName);
	#print("secondLast_clsName: " secondLast_clsName);
			ownerOfFunc[sizeI_belongsTo]++;
		}
	}

	# store interfaces
	for (clsName in class_ancestors) {
#print("clsName: " clsName);
		size_ancestorParts = split(class_ancestors[clsName], ancestorParts, ",");
#print("size_ancestorParts: " size_ancestorParts);
#print("ancestorParts[1]: " ancestorParts[1]);

		# check if an interface is needed
		if (size_ancestorParts > 1) {
#print("interface: " clsName);
			interfaces[clsName] = 1;

			# assign functions of the first implementation to the interface as reference
			ancCls = ancestorParts[1] "-" clsName;
			anc = ancestorParts[1] "_" clsName;
			size_funcs = ownerOfFunc[ancCls "*"];
			interfaceOwnerOfFunc[clsName "*"] = 0;
			for (f=0; f < size_funcs; f++) {
				fullName = ownerOfFunc[ancCls "#" f];
				interfaceOwnerOfFunc[clsName "#" interfaceOwnerOfFunc[clsName "*"]] = fullName;
				interfaceOwnerOfFunc[clsName "*"]++;
				
				funcBelongsToInterface[fullName] = clsName;
			}
			# assign member classes of the first implementation to the interface as reference
			interface_class[clsName] = ancestors_class[anc];
			size_memCls = split(interface_class[clsName], memCls, ",");
			for (mc=0; mc < size_memCl; mc++) {
				ancestorsInterface_multiSizes[clsName "-" memCls[mc+1] "*"] = ancestorsClass_multiSizes[anc "-" memCls[mc+1] "*"];
			}
			additionalIntIndices[clsName "*"] = additionalClsIndices[ancCls "*"];

			# generate class names for the implementations of the interface
			for (a=0; a < size_ancestorParts; a++) {
				implClsName = ancestorParts[a+1];
				implClsName = extractNormalPart(implClsName);
				#sub(/^_$/, "ROOT", implClsName);
				gsub(/_/, "", implClsName);
				implClsName = implClsName clsName "Impl";
#print("implClsName: " implClsName);

				implClsNames[ancestorParts[a+1] "-" clsName] = implClsName;
			}
		}
	}
}





function printInterfaces() {

	for (clsName in class_ancestors) {
		#size_ancestorParts = split(class_ancestors[clsName], ancestorParts, ",");

		# check if an interface is needed
		if (clsName in interfaces) {
#print("printInterfaces");
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
#print(size_funcs);
	for (f=0; f < size_funcs; f++) {
		fullName_i = interfaceOwnerOfFunc[clsName "#" f];
		printMember(outFile_i, fullName_i, size_addInds, 1);
	}

	# print member class fetchers (single, multi, multi-fetch-single)
	size_memCls = split(interface_class[clsName], memCls, ",");
#print("printInterface size_memCls: " size_memCls);
	for (mc=0; mc < size_memCls; mc++) {
		memberClass = memCls[mc+1];
		fullNameMultiSize_i = ancestorsInterface_isMulti[clsName "-" memberClass];TODO
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
	for (ai=0; ai < size_addInds; ai++) {
		addIndName = additionalClsIndices[clsId_c "#" ai];
		print("\t" "int get" capitalize(addIndName) "() {") >> outFile_c;
		print("\t\t" "return " addIndName ";") >> outFile_c;
		print("\t" "}") >> outFile_c;
		print("") >> outFile_c;
	}


	{
		# print static fetcher method
		print("\t" "static " clsNameExternal_c " getInstance(" ctorParams ") {") >> outFile_c;
		print("") >> outFile_c;
		print("\t\t" clsNameExternal_c " _ret = null;") >> outFile_c;
		fullNameAvailable_c = ancestorsClass_available[clsId_c];
		if (fullNameAvailable_c == "") {
			print("\t\t" "_ret = new " clsNameExternal_c "(" ctorParamsNoTypes ");") >> outFile_c;
		} else {
			print("\t\t" "boolean isAvailable = " myClassVar ".getInnerCallback()." fullNameAvailable_c "(" myClassVar ".getTeamId()" condaddIndPars_c addIndParsNoTypes_c ");") >> outFile_c;
			print("\t\t" "if (isAvailable) {") >> outFile_c;
			print("\t\t\t" "_ret = new " clsNameExternal_c "(" ctorParamsNoTypes ");") >> outFile_c;
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


	# print member functions
	size_funcs = ownerOfFunc[clsId_c "*"];
#print("");
#print(clsId_c);
#print(size_funcs);
	for (f=0; f < size_funcs; f++) {
		fullName_c = ownerOfFunc[clsId_c "#" f];
		printMember(outFile_c, fullName_c, size_addInds, 0);
	}


	# print member class fetchers (single, multi, multi-fetch-single)
	size_memCls = split(ancestors_class[clsFull_c], memCls, ",");
#if (match(clsFull_c, /Clb$/)) { print("Clb ancestors: " ancestors_class[clsFull_c]); }
#print("printClass size_memCls: " clsFull_c " / " size_memCls);
	for (mc=0; mc < size_memCls; mc++) {
		memberClass_c = memCls[mc+1];
#if (match(clsFull_c, /Clb$/)) { print("cls/mem: " clsFull_c " / " memberClass_c); }
		printMemberClassFetchers(outFile_c, clsFull_c, clsId_c, memberClass_c, isInterface_c);
	}


	print("}") >> outFile_c;
	print("") >> outFile_c;
}

function printMemberClassFetchers(outFile_mc, clsFull_mc, clsId_mc, memberClsName_mc, isInterface_mc) {

#if (match(clsFull_mc, /Clb$/)) { print("clsFull_mc: " clsFull_mc); }
		memberClassId_mc = clsFull_mc "-" memberClsName_mc;
		size_multi_mc = ancestorsClass_multiSizes[memberClassId_mc "*"];
		isMulti_mc = (size_multi_mc != "");
		if (isMulti_mc) { # multi element fetcher(s)
			for (mmc=0; mmc < size_multi_mc; mmc++) {
				fullNameMultiSize_mc = ancestorsClass_multiSizes[memberClassId_mc "#" mmc];
#print("multi mem cls: " memberClassId_mc "#" mmc " / " fullNameMultiSize_mc);
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
			print(indent_mf "private " retType " buffer_" fn ";") >> outFile_mf;
			print(indent_mf "private boolean buffer_isInitialized_" fn " = false;") >> outFile_mf;
		}

		print(indent_mf "public " retTypeInterface " " fn "(" params ") {") >> outFile_mf;
		print("") >> outFile_mf;
		indent_mf = indent_mf "\t";
		if (isBuffered_mf) {
			print(indent_mf retType " _ret = buffer_" fn ";") >> outFile_mf;
			print(indent_mf "if (!buffer_isInitialized_" fn ") {") >> outFile_mf;
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
			print(indent_mf "buffer_" fn " = _ret;") >> outFile_mf;
			print(indent_mf "buffer_isInitialized_" fn " = true;") >> outFile_mf;
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
	isVoid_m = retType == "void";
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
		sub(/\[\] .*$/, "", arrayType); # remove post array type
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
		print(indent_m retType " buffer_" memName ";") >> outFile_m;
		print(indent_m "boolean buffer_isInitialized_" memName " = false;") >> outFile_m;
	}

	print(indent_m mod_m retTypeInterface " " memName "(" params ")" firstLineEnd) >> outFile_m;
	if (!isInterface_m) {
		condRet = isVoid_m ? "" : "_ret = ";
		condInnerParamsComma = (innerParams == "") ? "" : ", ";
		indent_m = indent_m "\t";
		if (memName == "handleCommand") {
			print(indent_m "command.write();") >> outFile_m;
		}
		if (isBuffered_m) {
			print(indent_m retType " _ret = buffer_" memName ";") >> outFile_m;
			print(indent_m "if (!buffer_isInitialized_" memName ") {") >> outFile_m;
			indent_m = indent_m "\t";
		} else {
			print(indent_m retType " _ret;") >> outFile_m;
		}
		if (isArraySize) {
			print("") >> outFile_m;
			print(indent_m "int size = " myWrapper "." fullNameArraySize "(" myTeamId condInnerParamsComma innerParams ");") >> outFile_m;
			print(indent_m arrayType "[] tmpArr = new " arrayType "[size];") >> outFile_m;
			print(indent_m myWrapper "." fullNameArrayVals "(" myTeamId condInnerParamsComma innerParams ", tmpArr, size);") >> outFile_m;
			print(indent_m retType " arrList = new " retType "(size);") >> outFile_m;
			print(indent_m "for (int i=0; i < size; i++) {") >> outFile_m;
			if (arrayType == referenceType) {
				print(indent_m "\t" "arrList.add(tmpArr[i]);") >> outFile_m;
			} else {
				print(indent_m "\t" "arrList.add(" referenceType ".getInstance(" myClassVarLocal ", tmpArr[i]));") >> outFile_m;
			}
			print(indent_m "}") >> outFile_m;
			print(indent_m "_ret = arrList;") >> outFile_m;
		} else if (isMapSize) {
			print("") >> outFile_m;
			print(indent_m "int size = " myWrapper "." fullNameMapSize "(" myTeamId condInnerParamsComma innerParams ");") >> outFile_m;
			print(indent_m keyType "[] tmpKeysArr = new " keyType "[size];") >> outFile_m;
			print(indent_m myWrapper "." fullNameMapKeys "(" myTeamId condInnerParamsComma innerParams ", tmpKeysArr);") >> outFile_m;
			print(indent_m valType "[] tmpValsArr = new " valType "[size];") >> outFile_m;
			print(indent_m myWrapper "." fullNameMapVals "(" myTeamId condInnerParamsComma innerParams ", tmpValsArr);") >> outFile_m;
			retMapImplType = retType;
			sub(/Map/, defMapJavaImpl, retMapImplType);
			print(indent_m retType " retMap = new " retMapImplType "(size);") >> outFile_m;
			print(indent_m "for (int i=0; i < size; i++) {") >> outFile_m;
			print(indent_m "\t" "retMap.put(tmpKeysArr[i], tmpValsArr[i]);") >> outFile_m;
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
			print(indent_m "buffer_" memName " = _ret;") >> outFile_m;
			print(indent_m "buffer_isInitialized_" memName " = true;") >> outFile_m;
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






# grab callback functions
#/^\t[^ ]+ Clb_[^ ]+\(int teamId.*\)\;$/ {
/Clb_/ {

	fullName = $1;
	sub(/.* /, "", fullName);

	simpleFullName = extractNormalPart(fullName);

	retType = trim($1);
	sub(fullName, "", retType);

	params = $2;

	doWrapp = !match(simpleFullName, /^Clb_File/);

	if (doWrapp) {
		#size_tmpParts = split($0, tmpParts, / invoke\(/);

		#retType = trim(tmpParts[1]);

		#params = trim(tmpParts[2]);
		#sub(/\)\;/, "", params);
		sub(/^int teamId(\, )?/, "", params);

		innerParams = removeParamTypes(params);

		funcFullName[size_funcs] = fullName;
		funcRetType[fullName] = retType;
		funcParams[fullName] = params;
		funcInnerParams[fullName] = innerParams;

		if (!(simpleFullName in funcSimpleFullName)) {
			funcSimpleFullName[simpleFullName] = fullName;
		} else if (!match(funcSimpleFullName[simpleFullName], fullName)) {
			funcSimpleFullName[simpleFullName] = funcSimpleFullName[simpleFullName] "," fullName;
		}

		size_funcs++;
	}
}


END {
	# finalize things
	storeClassesAndInterfaces();

#for (abc in funcFullName) {
#print("funcFullName[i]: " abc " / " funcFullName[abc]);
#print("funcRetType[i]: " abc " / " funcRetType[funcFullName[abc]]);
#print("funcParams[i]: " abc " / " funcParams[funcFullName[abc]]);
#}
#for (abc in ownerOfFunc) {
#print("ownerOfFunc: " abc " / " ownerOfFunc[abc]);
#}
#print("");
#print("###############################################");
#print("");
#for (abc in interfaceOwnerOfFunc) {
#print("interfaceOwnerOfFunc: " abc " / " interfaceOwnerOfFunc[abc]);
#}

	printInterfaces();
	printClasses();
}
