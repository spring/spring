#!/bin/awk
#
# This awk script creates a C++ class hirarchy to wrapp the C AI Callback.
# In other words, the output of this file wrapps:
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
	FS = "(\\()|(\\)\\;)";
	IGNORECASE = 0;

	# These vars can be assigned externally, see file header.
	# Set the default values if they were not supplied on the command line.
	if (!GENERATED_SOURCE_DIR) {
		GENERATED_SOURCE_DIR = "..";
	}
	mySrc = GENERATED_SOURCE_DIR

	myNameSpace = "springai";
	myClass = "AICallback";
	myClassVar = "clb";
	myWrapClass = "SSkirmishAICallback";
	myWrapVar = "innerCallback";
	myAbstractAIClass = "AbstractAI";
	myAIFactoryClass = "AIFactory";
	#mySourceFile_h = mySrc "/" myClass ".h";
	#mySourceFile_cpp = mySrc "/" myClass ".cpp";

	MAX_IDS = 1024;

	#myBufferedClasses["_UnitDef"] = 1;
	#myBufferedClasses["_WeaponDef"] = 1;
	#myBufferedClasses["_FeatureDef"] = 1;

	size_funcs = 0;
	size_classes = 0;
	size_interfaces = 0;
}



function printHeader(outFile_h, className_h, isOrHasInterface_h, isH_h) {

	implementedInterfacePart = "";
	if (isOrHasInterface_h == 1) {
	} else if (isOrHasInterface_h != 0) {
		implementedInterfacePart = " : public " isOrHasInterface_h;
	}

	printCommentsHeader(outFile_h);
	if (isH_h) {
		print("") >> outFile_h;
		hg_h = createHeaderGuard(className_h);
		print("#ifndef " hg_h) >> outFile_h;
		print("#define " hg_h) >> outFile_h;
		print("") >> outFile_h;
		print("#include \"IncludesHeaders.h\"") >> outFile_h;
		if (isOrHasInterface_h != 0 && isOrHasInterface_h != 1) {
			print("#include \"" isOrHasInterface_h ".h\"") >> outFile_h;
		}
		print("") >> outFile_h;
		print("namespace " myNameSpace " {") >> outFile_h;
		print("") >> outFile_h;
		print("/**") >> outFile_h;
		print(" * Lets C++ Skirmish AIs call back to the Spring engine.") >> outFile_h;
		print(" *") >> outFile_h;
		print(" * @author	AWK wrapper script") >> outFile_h;
		print(" * @version	GENERATED") >> outFile_h;
		print(" */") >> outFile_h;
		print("class " className_h implementedInterfacePart " {") >> outFile_h;
		print("") >> outFile_h;
	} else {
		print("") >> outFile_h;
		myHeader_h = outFile_h;
		sub(/^.*[\/\\]/, "", myHeader_h);
		sub(/\.cpp$/, ".h", myHeader_h);
		print("#include \"" myHeader_h "\"") >> outFile_h;
		print("") >> outFile_h;
		print("#include \"IncludesSources.h\"") >> outFile_h;
		print("") >> outFile_h;
	}
}

function createHeaderGuard(className_hg) {

	return "_CPPWRAPPER_" toupper(className_hg) "_H";
}

function createFileName(clsName_cfn, header_cfn) {

	if (header_cfn) {
		suffix_cfn = ".h";
	} else {
		suffix_cfn = ".cpp";
	}

	return mySrc "/"  clsName_cfn suffix_cfn;
}



function printIncludes() {

	outH_h_inc = createFileName("IncludesHeaders", 1);
	outS_h_inc = createFileName("IncludesSources", 1);
	{
		printCommentsHeader(outH_h_inc);

		print("") >> outH_h_inc;
		print("#include <cstdio> // for NULL") >> outH_h_inc;
		print("#include <vector>") >> outH_h_inc;
		print("#include <map>") >> outH_h_inc;
		print("") >> outH_h_inc;
		print("#include \"ExternalAI/Interface/SAIFloat3.h\"") >> outH_h_inc;
		print("") >> outH_h_inc;
		print("struct SSkirmishAICallback;") >> outH_h_inc;
		print("") >> outH_h_inc;
		print("namespace " myNameSpace " {") >> outH_h_inc;
		print("class " myClass ";") >> outH_h_inc;
		for (clsName in class_ancestors) {
			if (clsName != "Clb") {
				print("class " clsName ";") >> outH_h_inc;
			}
		}
		for (clsName in interfaces) {
			print("class " clsName ";") >> outH_h_inc;
		}
		print("} // namespace " myNameSpace) >> outH_h_inc;
		print("") >> outH_h_inc;
	}
	{
		printCommentsHeader(outS_h_inc);

		print("") >> outS_h_inc;
		print("#include \"ExternalAI/Interface/SSkirmishAICallback.h\"") >> outS_h_inc;
		print("") >> outS_h_inc;
		print("#include \"" myClass ".h\"") >> outS_h_inc;
		for (clsName in class_ancestors) {
			if (clsName != "Clb") {
				print("#include \"" clsName ".h\"") >> outS_h_inc;
			}
		}
		for (clsId in implClsNames) {
			print("#include \"" implClsNames[clsId] ".h\"") >> outS_h_inc;
		}
		for (clsName in interfaces) {
			print("#include \"" clsName ".h\"") >> outS_h_inc;
		}
		print("") >> outS_h_inc;
	}
}


function isSpringAIClass(type_isac) {

	for (clsName_isac in class_ancestors) {
		if (clsName_isac == type_isac) {
			return 1;
		}
	}
	for (clsName_isac in interfaces) {
		if (clsName_isac == type_isac) {
			return 1;
		}
	}
	for (clsId_isac in implClsNames) {
		if (implClsNames[clsId_isac] == type_isac) {
			return 1;
		}
	}

	return 0;
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

	out_h_i = createFileName(clsName_i, 1);
	printHeader(out_h_i, clsName_i, 1, 1);

	size_addInds = additionalIntIndices[clsName_i "*"];

	# print member functions
	size_funcs = interfaceOwnerOfFunc[clsName_i "*"];
	for (f=0; f < size_funcs; f++) {
		fullName_i = interfaceOwnerOfFunc[clsName "#" f];
		printMember(out_h_i, "", clsName_i, fullName_i, size_addInds, 1);
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
				fn = "Get" memberClass "s";
			}
			params = funcParams[fullNameMultiSize_i];
		} else {
			fn = "Get" memberClass;
			params = "";
		}
		# remove additional indices from params
		for (ai=0; ai < size_addInds; ai++) {
			sub(/int [_a-zA-Z0-9]+(, )?/, "", params);
		}
		print("\t" memberClass " " fn "(" params ") = 0;") >> out_h_i;
	}

	print("}; // class " clsName_i ) >> out_h_i;
	print("} // namespace " myNameSpace) >> out_h_i;
	print("") >> out_h_i;
	hg_i = createHeaderGuard(clsName_i);
	print("#endif // " hg_i) >> out_h_i;
	print("") >> out_h_i;

	close(out_h_i);
}





function printClasses() {

	for (clsName in class_ancestors) {
		size_ancestorParts = split(class_ancestors[clsName], ancestorParts, ",");

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
	myWrapper = myClassVar "->GetInnerCallback()";
	myTeamId = myClassVar "->GetTeamId()";
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

	out_h_c = createFileName(clsNameExternal_c, 1);
	out_s_c = createFileName(clsNameExternal_c, 0);
	printHeader(out_h_c, clsNameExternal_c, intName_c, 1);
	printHeader(out_s_c, clsNameExternal_c, intName_c, 0);

	size_addInds = additionalClsIndices[clsId_c "*"];


	# print private vars
	print("private:") >> out_h_c;
	if (isClbRootCls) {
		print("\t" "const " myWrapClass "* " myWrapVar ";") >> out_h_c;
		print("\t" "int teamId;") >> out_h_c;
	} else {
		print("\t" "" myClass "* " myClassVar ";") >> out_h_c;
	}
	# print additionalVars
	for (ai=0; ai < size_addInds; ai++) {
		print("\t" "int " additionalClsIndices[clsId_c "#" ai] ";") >> out_h_c;
	}
	print("") >> out_h_c;


	# print constructor
	#print("private:") >> out_h_c;
	if (isClbRootCls) {
		ctorParams = "const " myWrapClass "* " myWrapVar ", int teamId";
	} else {
		ctorParams = myClass "* " myClassVar;
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

	print("\t" clsNameExternal_c "(" ctorParams ");") >> out_h_c;
	print(myNameSpace "::" clsNameExternal_c "::" clsNameExternal_c "(" ctorParams ") {") >> out_s_c;
	print("") >> out_s_c;
	if (isClbRootCls) {
		print("\t" "this->" myWrapVar " = " myWrapVar ";") >> out_s_c;
		print("\t" "this->teamId = teamId;") >> out_s_c;
	} else {
		print("\t" "this->" myClassVar " = " myClassVar ";") >> out_s_c;
	}
	# init additionalVars
	for (ai=0; ai < size_addInds; ai++) {
		addIndName = additionalClsIndices[clsId_c "#" ai];
		print("\t" "this->" addIndName " = " addIndName ";") >> out_s_c;
	}
	print("}") >> out_s_c;
	print("") >> out_s_c;
	print("") >> out_h_c;

	# print additional vars fetchers
	print("public:") >> out_h_c;
	for (ai=0; ai < size_addInds; ai++) {
		addIndName = additionalClsIndices[clsId_c "#" ai];
		print("\t" "virtual int Get" capitalize(addIndName) "();") >> out_h_c;
		print("int " myNameSpace "::" clsNameExternal_c "::Get" capitalize(addIndName) "() {") >> out_s_c;
		print("\t" "return " addIndName ";") >> out_s_c;
		print("}") >> out_s_c;
		print("") >> out_s_c;
	}
	print("") >> out_s_c;
	print("") >> out_h_c;

	# print static instance fetcher method
	{
		clsIsBuffered_c = isBufferedClass(clsNameExternal_c);
		fullNameAvailable_c = ancestorsClass_available[clsId_c];

		if (clsIsBuffered_c) {
			print("private:") >> out_h_c;
			print("\t" "static std::map<int, " clsNameExternal_c "> _buffer_instances();") >> out_h_c;
			print("") >> out_h_c;
			print("public:") >> out_h_c;
		}
		print("\t" "static " clsNameExternal_c "* GetInstance(" ctorParams ");") >> out_h_c;
		print(myNameSpace "::" clsNameExternal_c "* " myNameSpace "::" clsNameExternal_c "::GetInstance(" ctorParams ") {") >> out_s_c;
		print("") >> out_s_c;
		lastParamName = ctorParamsNoTypes;
		sub(/^.*,[ \t]*/, "", lastParamName);
		if (match(lastParamName, /^[^ \t]+Id$/)) {
			if (clsNameExternal_c == "Unit") {
				# the first valid unit ID is 1
				print("\t" "if (" lastParamName " <= 0) {") >> out_s_c;
			} else {
				# ... for all other IDs, the first valid one is 0
				print("\t" "if (" lastParamName " < 0) {") >> out_s_c;
			}
			print("\t\t" "return NULL;") >> out_s_c;
			print("\t" "}") >> out_s_c;
			print("") >> out_s_c;
		}
		print("\t" clsNameExternal_c "* _ret = NULL;") >> out_s_c;
		if (fullNameAvailable_c == "") {
			print("\t" "_ret = new " clsNameExternal_c "(" ctorParamsNoTypes ");") >> out_s_c;
		} else {
			print("\t" "bool isAvailable = " myClassVar "->GetInnerCallback()->" fullNameAvailable_c "(" myClassVar "->GetTeamId()" condaddIndPars_c addIndParsNoTypes_c ");") >> out_s_c;
			print("\t" "if (isAvailable) {") >> out_s_c;
			print("\t\t" "_ret = new " clsNameExternal_c "(" ctorParamsNoTypes ");") >> out_s_c;
			print("\t" "}") >> out_s_c;
		}
		if (clsIsBuffered_c) {
			if (fullNameAvailable_c == "") {
				print("\t" "{") >> out_s_c;
			} else {
				print("\t" "if (_ret != NULL) {") >> out_s_c;
			}
			print("\t\t" "int indexHash = _ret.hashCode();") >> out_s_c;
			print("\t\t" "if (_buffer_instances.find(indexHash) != _buffer_instances.end()) {") >> out_s_c;
			print("\t\t\t" "_ret = _buffer_instances[indexHash];") >> out_s_c;
			print("\t\t" "} else {") >> out_s_c;
			print("\t\t\t" "_buffer_instances[indexHash] = _ret;") >> out_s_c;
			print("\t\t" "}") >> out_s_c;
			print("\t" "}") >> out_s_c;
		}
		print("\t" "return _ret;") >> out_s_c;
		print("}") >> out_s_c;
		print("") >> out_s_c;
	}

	if (isClbRootCls) {
		# print inner-callback fetcher method
		print("\tconst " myWrapClass "* GetInnerCallback();") >> out_h_c;
		print("const " myWrapClass "* " myNameSpace "::" clsNameExternal_c "::GetInnerCallback() {") >> out_s_c;
		print("\t" "return this->" myWrapVar ";") >> out_s_c;
		print("}") >> out_s_c;
		print("") >> out_s_c;
		# print teamId fetcher method
		print("\t" "int GetTeamId();") >> out_h_c;
		print("int " myNameSpace "::" clsNameExternal_c "::GetTeamId() {") >> out_s_c;
		print("\t" "return this->teamId;") >> out_s_c;
		print("}") >> out_s_c;
		print("") >> out_s_c;
	}

	# print compareTo(other) method
	if (1 == 0) {
		print("\t" "public int compareTo(" clsNameExternal_c " other) {") >> out_s_c;
		print("\t\t" "final int BEFORE = -1;") >> out_s_c;
		print("\t\t" "final int EQUAL  =  0;") >> out_s_c;
		print("\t\t" "final int AFTER  =  1;") >> out_s_c;
		print("") >> out_s_c;
		print("\t\t" "if (this == other) return EQUAL;") >> out_s_c;
		print("") >> out_s_c;

		if (isClbRootCls) {
			print("\t\t" "if (this->teamId < other.teamId) return BEFORE;") >> out_s_c;
			print("\t\t" "if (this->teamId > other.teamId) return AFTER;") >> out_s_c;
			print("\t\t" "return EQUAL;") >> out_s_c;
		} else {
			for (ai=0; ai < size_addInds; ai++) {
				addIndName = additionalClsIndices[clsId_c "#" ai];
				print("\t\t" "if (this->get" capitalize(addIndName) "() < other.get" capitalize(addIndName) "()) return BEFORE;") >> out_s_c;
				print("\t\t" "if (this->get" capitalize(addIndName) "() > other.get" capitalize(addIndName) "()) return AFTER;") >> out_s_c;
			}
			print("\t\t" "return this->" myClassVar "->compareTo(other." myClassVar ");") >> out_s_c;
		}
		print("\t" "}") >> out_s_c;
		print("") >> out_s_c;
	}


	# print equals(other) method
	if (1 == 0 && !isClbRootCls) {
		print("\t" "@Override") >> out_s_c;
		print("\t" "public boolean equals(Object otherObject) {") >> out_s_c;
		print("") >> out_s_c;
		print("\t\t" "if (this == otherObject) return true;") >> out_s_c;
		print("\t\t" "if (!(otherObject instanceof " clsNameExternal_c ")) return false;") >> out_s_c;
		print("\t\t" clsNameExternal_c " other = (" clsNameExternal_c ") otherObject;") >> out_s_c;
		print("") >> out_s_c;

		if (isClbRootCls) {
			print("\t\t" "if (this->teamId != other.teamId) return false;") >> out_s_c;
			print("\t\t" "return true;") >> out_s_c;
		} else {
			for (ai=0; ai < size_addInds; ai++) {
				addIndName = additionalClsIndices[clsId_c "#" ai];
				print("\t\t" "if (this->get" capitalize(addIndName) "() != other.get" capitalize(addIndName) "()) return false;") >> out_s_c;
			}
			print("\t\t" "return this->" myClassVar "->equals(other." myClassVar ");") >> out_s_c;
		}
		print("\t" "}") >> out_s_c;
		print("") >> out_s_c;
	}


	# print hashCode() method
	if (1 == 0 && !isClbRootCls) {
		print("\t" "@Override") >> out_s_c;
		print("\t" "public int hashCode() {") >> out_s_c;
		print("") >> out_s_c;
		print("\t\t" "int _res = 23;") >> out_s_c;
		print("") >> out_s_c;

		if (isClbRootCls) {
			print("\t\t" "_res += this->teamId * 10E8;") >> out_s_c;
		} else {
			# NOTE: This could go wrong if we have more then 7 additional indices
			# see 10E" (7-ai) below
			for (ai=0; ai < size_addInds; ai++) {
				addIndName = additionalClsIndices[clsId_c "#" ai];
				print("\t\t" "_res += this->get" capitalize(addIndName) "() * 10E" (7-ai) ";") >> out_s_c;
			}
			print("\t\t" "_res += this->" myClassVar "->hashCode();") >> out_s_c;
		}
		print("") >> out_s_c;
		print("\t\t" "return _res;") >> out_s_c;
		print("\t" "}") >> out_s_c;
		print("") >> out_s_c;
	}

	# print member functions
	size_funcs = ownerOfFunc[clsId_c "*"];
	for (f=0; f < size_funcs; f++) {
		fullName_c = ownerOfFunc[clsId_c "#" f];
		printMember(out_h_c, out_s_c, clsNameExternal_c, fullName_c, size_addInds, 0);
	}


	# print member class fetchers (single, multi, multi-fetch-single)
	size_memCls = split(ancestors_class[clsFull_c], memCls, ",");
	for (mc=0; mc < size_memCls; mc++) {
		memberClass_c = memCls[mc+1];
		printMemberClassFetchers(out_h_c, out_s_c, clsNameExternal_c, clsFull_c, clsId_c, memberClass_c, isInterface_c);
	}


	print("}; // class " clsNameExternal_c ) >> out_h_c;
	print("} // namespace " myNameSpace) >> out_h_c;
	print("") >> out_h_c;
	hg_c = createHeaderGuard(clsNameExternal_c);
	print("#endif // " hg_c) >> out_h_c;
	print("") >> out_h_c;

	close(out_s_c);
	close(out_h_c);
}

function printMemberClassFetchers(out_h_mc, out_s_mc, clsNameExternal_mc, clsFull_mc, clsId_mc, memberClsName_mc, isInterface_mc) {

#if (match(clsFull_mc, /Clb$/)) { print("clsFull_mc: " clsFull_mc); }
		memberClassId_mc = clsFull_mc "-" memberClsName_mc;
		size_multi_mc = ancestorsClass_multiSizes[memberClassId_mc "*"];
		isMulti_mc = (size_multi_mc != "");
		if (isMulti_mc) { # multi element fetcher(s)
			for (mmc=0; mmc < size_multi_mc; mmc++) {
				fullNameMultiSize_mc = ancestorsClass_multiSizes[memberClassId_mc "#" mmc];
#print("multi mem cls: " memberClassId_mc "#" mmc " / " fullNameMultiSize_mc);
				printMemberClassFetcher(out_h_mc, out_s_mc, clsNameExternal_mc, clsFull_mc, clsId_mc, memberClsName_mc, isInterface_mc, fullNameMultiSize_mc);
			}
		} else { # single element fetcher
			printMemberClassFetcher(out_h_mc, out_s_mc, clsNameExternal_mc, clsFull_mc, clsId_mc, memberClsName_mc, isInterface_mc, 0);
		}
}
# fullNameMultiSize_mf is 0 if it is no multi element
function printMemberClassFetcher(out_h_mf, out_s_mf, clsNameExternal_mf, clsFull_mf, clsId_mf, memberClsName_mf, isInterface_mf, fullNameMultiSize_mf) {

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
			fn = "Get" fn;

			params = funcParams[fullNameMultiSize_mf];
			innerParams = funcInnerParams[fullNameMultiSize_mf];

			fullNameMultiVals_mf = fullNameMultiSize_mf;
			sub(/0MULTI1SIZE/, "0MULTI1VALS", fullNameMultiVals_mf);
			hasMultiVals_mf = (funcRetType[fullNameMultiVals_mf] != "");
		} else { # single element fetcher
			fn = "Get" memberClsName_mf;
			params = "";
		}

		# remove additional indices from params
		for (ai=0; ai < size_addInds; ai++) {
			sub(/int [_a-zA-Z0-9]+(, )?/, "", params);
		}

		retType = memberClsName_mf;
		if (isMulti_mf) {
			retTypeInterface = "std::vector<" retType "*>";
			retTypeInterfaceSrc = "std::vector<" myNameSpace "::" retType "*>";
			retType = "std::vector<" retType "*>";
		} else {
			retTypeInterface = retType "*";
			retTypeInterfaceSrc = myNameSpace "::" retType "*";
			retType = retType "*";
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
			print("private:") >> out_h_mf;
			print(indent_mf "" retType "* _buffer_" fn ";") >> out_h_mf;
			# TODO: FIXME: this is not good -> make non static, but still has to be initialized to false!
			print(indent_mf "static bool _buffer_isInitialized_" fn " = false;") >> out_s_mf;
		}

		printFunctionComment_Common(out_h_mf, funcDocComment, fullNameMultiSize_mf, indent_mf);

		print("public:") >> out_h_mf;
		print(indent_mf "" retTypeInterface " " fn "(" params ");") >> out_h_mf;
		print(retTypeInterfaceSrc " " myNameSpace "::" clsNameExternal_mf "::" fn "(" params ") {") >> out_s_mf;
		print("") >> out_s_mf;
		indent_mf = indent_mf "\t";
		if (isBuffered_mf) {
			print(indent_mf retType " _ret = _buffer_" fn ";") >> out_s_mf;
			print(indent_mf "if (!_buffer_isInitialized_" fn ") {") >> out_s_mf;
			indent_mf = indent_mf "\t";
		} else {
			print(indent_mf retType " _ret;") >> out_s_mf;
		}
		if (isMulti_mf) {
			print(indent_mf "int size = " myWrapper "->" fullNameMultiSize_mf "(" myTeamId condInnerParamsComma innerParams ");") >> out_s_mf;
			if (hasMultiVals_mf) {
				print(indent_mf "int* tmpArr = new int[size];") >> out_s_mf;
				print(indent_mf myWrapper "->" fullNameMultiVals_mf "(" myTeamId condInnerParamsComma innerParams ", tmpArr, size);") >> out_s_mf;
				print(indent_mf retType " arrList;") >> out_s_mf;
				print(indent_mf "for (int i=0; i < size; i++) {") >> out_s_mf;
				print(indent_mf "\t" "arrList.push_back(" memberClassImpl_mf "::GetInstance(" myClassVarLocal condIndexComma_mf indexParams_mf ", tmpArr[i]));") >> out_s_mf;
				print(indent_mf "}") >> out_s_mf;
				print(indent_mf "_ret = arrList;") >> out_s_mf;
			} else {
				print(indent_mf retType " arrList;") >> out_s_mf;
				print(indent_mf "for (int i=0; i < size; i++) {") >> out_s_mf;
				print(indent_mf "\t" "arrList.push_back(" memberClassImpl_mf "::GetInstance(" myClassVarLocal condIndexComma_mf indexParams_mf ", i));") >> out_s_mf;
				print(indent_mf "}") >> out_s_mf;
				print(indent_mf "_ret = arrList;") >> out_s_mf;
			}
		} else {
			print(indent_mf "_ret = " memberClassImpl_mf "::GetInstance(" myClassVarLocal condIndexComma_mf indexParams_mf ");") >> out_s_mf;
		}
		if (isBuffered_mf) {
			print(indent_mf "_buffer_" fn " = _ret;") >> out_s_mf;
			print(indent_mf "_buffer_isInitialized_" fn " = true;") >> out_s_mf;
			sub(/\t/, "", indent_mf);
			print(indent_mf "}") >> out_s_mf;
			print("") >> out_s_mf;
		}
		print(indent_mf "return _ret;") >> out_s_mf;
		sub(/\t/, "", indent_mf);
		print("\t" "}") >> out_s_mf;
}





function printMember(out_h_m, out_s_m, clsNameExternal_m, fullName_m, additionalIndices_m, isInterface_m) {
#print("out_h_m: " out_h_m);
	if (part_isMultiValues(fullName_m) || part_isArrayValues(fullName_m) || part_isMapKeys(fullName_m) || part_isMapValues(fullName_m)) {
		return;
	}

	indent_m = "\t";
	retType = funcRetType[fullName_m];
	retTypeInterface = "";
	retTypeInterfaceSrc = "";
	params = funcParams[fullName_m];
	innerParams = funcInnerParams[fullName_m];
	memName = extractNormalPart(fullName_m);
	sub(/^.*_/, "", memName);
	isVoid_m = (retType == "void");
	isBuffered_m = !isVoid_m && isBufferedFunc(fullName_m);

#	if (memName == "handleCommand") {
#		sub(/int commandTopic\, Pointer commandData/, "AICommand command", params);
#		sub(/commandTopic\, commandData/, "command.getTopic(), command.getPointer()", innerParams);
#	}

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
		usingBrackets = sub(/ [^ \t]+\[\] .*$/, "", arrayType); # remove post array type ([])
		sub(/[ \t][^ \t]+$/, "", arrayType); # remove param name
		if (!usingBrackets && match(arrayType, /SAIFloat3\*/)) {
			sub(/\*$/, "", arrayType); # remove array type (*) if it is SAIFloat3
		}
		referenceType = arrayType;
		if (match(fullNameArraySize, /0ARRAY1SIZE1/)) {
			# we want to reference objects, of a certain class as arrayType
			referenceType = fullNameArraySize;
			sub(/^.*0ARRAY1SIZE1/, "", referenceType); # remove pre ref array type
			sub(/[0123].*$/, "", referenceType); # remove post ref array type
		}
		arrType_java = referenceType;
		if (!isSpringAIClass(arrType_java)) {
			retTypeInterface = "std::vector<" arrType_java ">";
			retTypeInterfaceSrc = "std::vector<" arrType_java ">";
			retType = "std::vector<" arrType_java ">";
		} else {
			retTypeInterface = "std::vector<" arrType_java "*>";
			retTypeInterfaceSrc = "std::vector<" myNameSpace "::" arrType_java "*>";
			retType = "std::vector<" arrType_java "*>";
		}
		#sub(/(\, )?[^ ]+ [_a-zA-Z0-9]+$/, "", params); # remove array
#print("params 0: " params);
#print("arrayType: " arrayType);
		sub(/(\, )?[^,]+ [_a-zA-Z0-9]+\[\]$/, "", params); # remove array
#print("params 1: " params);
	}

	isMapSize = part_isMapSize(fullName_m);
	if (isMapSize) {
		fullNameMapSize = fullName_m;

		fullNameMapKeys = fullNameMapSize;
		sub(/0MAP1SIZE/, "0MAP1KEYS", fullNameMapKeys);
#print("fullNameMapKeys: " fullNameMapKeys) >> outFile_m;
		keyType = funcParams[fullNameMapKeys];
		sub(/\[\].*$/, "", keyType); # remove everything after array param
		sub(/^.*, /, "", keyType); # remove everything before array param
		keyType = extractParamType(keyType)

		fullNameMapVals = fullNameMapSize;
		sub(/0MAP1SIZE/, "0MAP1VALS", fullNameMapVals);
		valType = funcParams[fullNameMapVals];
		sub(/\[\].*$/, "", valType); # remove everything after array type
		sub(/^.*, /, "", valType); # remove everything before array type
		valType = extractParamType(valType)

		sub(/\, int [_a-zA-Z0-9]+$/, "", params); # remove max
		retType = "std::map<" keyType ", " valType ">"; # getArrayType
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
		retType = fetchClass_m "*";
		retTypeInterfaceSrc = myNameSpace "::" retType;

		indName_m = additionalClsIndices[clsId_c "#" (additionalClsIndices[clsId_c "*"]-1)];
		#instanceInnerParams = innerParams;
		#sub("(, )?" indName_m, "", instanceInnerParams); # remove index parameter
		instanceInnerParams = "";
	}

	refObjsFullName_m = fullName_m;
	hasReferenceObject_m = part_hasReferenceObject(refObjsFullName_m);
	while (hasReferenceObject_m) {
		refObj_m = refObjsFullName_m;
		sub(/^.*0REF/, "", refObj_m); # remove everything before ref type
		sub(/0.*$/, "", refObj_m); # remove everything after ref type

		refCls_m = refObj_m;
		sub(/^[^1]*1/, "", refCls_m); # remove everything before ref cls
		sub(/[123].*$/, "", refCls_m); # remove everything after ref cls

		refParamName_m = refObj_m;
		sub(/^[^2]*2/, "", refParamName_m); # remove everything before ref param name
		sub(/[123].*$/, "", refParamName_m); # remove everything after ref param name

		#refParamNameClass_m[refParamName_m] = refCls_m; # eg: refParamNameClass_m["resourceId"] = "Resource"
		sub("int " refParamName_m, refCls_m " c_" refParamName_m, params); # remove everything before ref param name
		sub(refParamName_m, "c_" refParamName_m ".Get" capitalize(refParamName_m) "()", innerParams); # remove everything before ref param name

		sub("0REF" refObj_m, "", refObjsFullName_m); # remove everything after array type
		hasReferenceObject_m = part_hasReferenceObject(refObjsFullName_m);
	}

	# remove additional indices from params
	for (ai=0; ai < additionalIndices_m; ai++) {
		sub(/int [_a-zA-Z0-9]+(, )?/, "", params);
	}

	mod_m = "";

	retType = trim(retType);
	if (retTypeInterface == "") {
		retTypeInterface = retType;
	}
	if (retTypeInterfaceSrc == "") {
		retTypeInterfaceSrc = retTypeInterface;
	}

	# convert eg. "const char* const" to "const char*"
	sub(/\* const$/, "*", retType);

	print("") >> out_h_m;
	if (!isInterface_m && isBuffered_m) {
		print("private:") >> out_h_m;
		print(indent_m retType " _buffer_" memName ";") >> out_h_m;
		print(indent_m "bool _buffer_isInitialized_" memName " = false;") >> out_h_m;
	}


	memNameExternal_m = memName;
	if (capitalize(memName) != clsNameExternal_m) {
		memNameExternal_m = capitalize(memName);
	}

	if (!isInterface_m) {
		print("public:") >> out_h_m;
	}
	printFunctionComment_Common(out_h_m, funcDocComment, fullName_m, indent_m);
	print(indent_m mod_m retTypeInterface " " memNameExternal_m "(" params ");") >> out_h_m;
	if (!isInterface_m) {
		print(retTypeInterfaceSrc " " myNameSpace "::" clsNameExternal_m "::" memNameExternal_m "(" params ") {") >> out_s_m;
		condRet = isVoid_m ? "" : "_ret = ";
		condInnerParamsComma = (innerParams == "") ? "" : ", ";
		indent_m = indent_m "\t";
		if (isBuffered_m) {
			print(indent_m retType " _ret = _buffer_" memName ";") >> out_s_m;
			print(indent_m "if (!_buffer_isInitialized_" memName ") {") >> out_s_m;
			indent_m = indent_m "\t";
		} else if (!isVoid_m) {
			print(indent_m retType " _ret;") >> out_s_m;
		}
		if (isArraySize) {
			print("") >> out_s_m;
			print(indent_m "int size = " myWrapper "->" fullNameArraySize "(" myTeamId condInnerParamsComma innerParams ");") >> out_s_m;
			print(indent_m arrayType "* tmpArr = new " arrayType "[size];") >> out_s_m;
			print(indent_m myWrapper "->" fullNameArrayVals "(" myTeamId condInnerParamsComma innerParams ", tmpArr, size);") >> out_s_m;
			print(indent_m retType " arrList;") >> out_s_m;
			print(indent_m "for (int i=0; i < size; i++) {") >> out_s_m;
			if (arrayType == referenceType) {
				print(indent_m "\t" "arrList.push_back(tmpArr[i]);") >> out_s_m;
			} else {
				print(indent_m "\t" "arrList.push_back(" referenceType "::GetInstance(" myClassVarLocal ", tmpArr[i]));") >> out_s_m;
			}
			print(indent_m "}") >> out_s_m;
			print(indent_m "delete [] tmpArr;") >> out_s_m;
			print(indent_m "_ret = arrList;") >> out_s_m;
		} else if (isMapSize) {
			print("") >> out_s_m;
			print(indent_m "int size = " myWrapper "->" fullNameMapSize "(" myTeamId condInnerParamsComma innerParams ");") >> out_s_m;
			print(indent_m keyType "* tmpKeysArr = new " keyType "[size];") >> out_s_m;
			print(indent_m myWrapper "->" fullNameMapKeys "(" myTeamId condInnerParamsComma innerParams ", tmpKeysArr);") >> out_s_m;
			print(indent_m valType "* tmpValsArr = new " valType "[size];") >> out_s_m;
			print(indent_m myWrapper "->" fullNameMapVals "(" myTeamId condInnerParamsComma innerParams ", tmpValsArr);") >> out_s_m;
			retMapImplType = retType;
			sub(/Map/, defMapJavaImpl, retMapImplType);
			print(indent_m retType " retMap;") >> out_s_m;
			print(indent_m "for (int i=0; i < size; i++) {") >> out_s_m;
			print(indent_m "\t" "retMap[tmpKeysArr[i]] = tmpValsArr[i];") >> out_s_m;
			print(indent_m "}") >> out_s_m;
			print(indent_m "delete [] tmpKeysArr;") >> out_s_m;
			print(indent_m "delete [] tmpValsArr;") >> out_s_m;
			print(indent_m "_ret = retMap;") >> out_s_m;
		} else if (isSingleFetch) {
			condInstanceInnerParamsComma = (instanceInnerParams == "") ? "" : ", ";
			print("") >> out_s_m;
			print(indent_m innerRetType " innerRet = " myWrapper "->" fullName_m "(" myTeamId condInnerParamsComma innerParams ");") >> out_s_m;
			pureRetType = retType;
			sub(/\*$/, "", pureRetType);
			print(indent_m "_ret = " pureRetType "::GetInstance(" myClassVarLocal ", innerRet" condInstanceInnerParamsComma instanceInnerParams ");") >> out_s_m;
		} else {
			print(indent_m condRet myWrapper "->" fullName_m "(" myTeamId condInnerParamsComma innerParams ");") >> out_s_m;
		}
		if (isBuffered_m) {
			print(indent_m "_buffer_" memName " = _ret;") >> out_s_m;
			print(indent_m "_buffer_isInitialized_" memName " = true;") >> out_s_m;
			sub(/\t/, "", indent_m);
			print(indent_m "}") >> out_s_m;
			print("") >> out_s_m;
		}
		if (!isVoid_m) {
			print(indent_m "return _ret;") >> out_s_m;
		}
		sub(/\t/, "", indent_m);
		print(indent_m "}") >> out_s_m;
	}
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


# grab callback functions pointer info
# 2nd, 3rd, ... line of a function pointer definition
{
	if (isMultiLineFunc) { # function is defined on one single line
		funcIntermLine = $0;
		# remove possible comment at end of line: // fu bar
		sub(/[ \t]*\/\/.*/, "", funcIntermLine);
		funcIntermLine = trim(funcIntermLine);
		funcSoFar = funcSoFar " " funcIntermLine;
		if (match(funcSoFar, /\;$/)) {
			# function ends in this line
			wrappFunctionPointerDef(funcSoFar);
			isMultiLineFunc = 0;
		}
	}
}
# 1st line of a function pointer definition
/^[^\/]*CALLING_CONV.*$/ {

	funcStartLine = $0;
	# remove possible comment at end of line: // fu bar
	sub(/\/\/.*$/, "", funcStartLine);
	funcStartLine = trim(funcStartLine);
	if (match(funcStartLine, /\;$/)) {
		# function ends in this line
		wrappFunctionPointerDef(funcStartLine);
	} else {
		funcSoFar = funcStartLine;
		isMultiLineFunc = 1;
	}
}

function wrappFunctionPointerDef(funcDef) {

	size_funcParts = split(funcDef, funcParts, "(\\()|(\\)\\;)");
	# because the empty part after ");" would count as part aswell
	size_funcParts--;

	fullName = funcParts[2];
	sub(/.*[ \t]+\*/, "", fullName);
	sub(/\)$/, "", fullName);

	retType = trim(funcParts[1]);

	params = funcParts[3];

	wrappFunction(retType, fullName, params);
}


END {
	# finalize things
	storeClassesAndInterfaces();

	printIncludes();
	printInterfaces();
	printClasses();
}
