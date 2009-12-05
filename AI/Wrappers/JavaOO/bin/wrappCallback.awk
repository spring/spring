#!/bin/awk
#
# This awk script creates Java classes in OO style to wrapp the C style
# JNI based AI Callback wrapper.
# In other words, the output of this file wrapps:
# com/springrts/ai/AICallback.java
# which wrapps:
# rts/ExternalAI/Interface/SSkirmishAICallback.h
# and
# rts/ExternalAI/Interface/AISCommands.h
#
# This script uses functions from the following files:
# * common.awk
# * commonDoc.awk
# * commonOOCallback.awk
# Variables that can be set on the command-line (with -v):
# * GENERATED_SOURCE_DIR           : the generated sources root dir
# * JAVA_GENERATED_SOURCE_DIR      : the generated java sources root dir
# * INTERFACE_SOURCE_DIR           : the Java AI Interfaces static source files root dir
# * INTERFACE_GENERATED_SOURCE_DIR : the Java AI Interfaces generated source files root dir
#
# usage:
# 	awk -f thisScript.awk -f common.awk -f commonDoc.awk -f commonOOCallback.awk
# 	awk -f thisScript.awk -f common.awk -f commonDoc.awk -f commonOOCallback.awk \
#       -v 'GENERATED_SOURCE_DIR=/tmp/build/AI/Interfaces/Java/src-generated'
#

BEGIN {
	# initialize things

	# define the field splitter(-regex)
	FS = "(\\()|(\\)\\;)";
	IGNORECASE = 0;

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
	if (!INTERFACE_SOURCE_DIR) {
		INTERFACE_SOURCE_DIR = "../../../Interfaces/Java/src/main/java";
	}
	if (!INTERFACE_GENERATED_SOURCE_DIR) {
		INTERFACE_GENERATED_SOURCE_DIR = "../../../Interfaces/Java/src-generated/main/java";
	}

	myMainPkgA     = "com.springrts.ai";
	myParentPkgA   = myMainPkgA ".oo";
	myPkgA         = myParentPkgA ".clb";
	myPkgD         = convertJavaNameFormAToD(myPkgA);
	myClassVar     = "ooClb";
	myWrapClass    = "AICallback";
	myWrapVar      = "innerCallback";
	defMapJavaImpl = "HashMap";

	myBufferedClasses["UnitDef"]    = 1;
	myBufferedClasses["WeaponDef"]  = 1;
	myBufferedClasses["FeatureDef"] = 1;

	retParamName  = "__retVal";

	size_funcs = 0;
	size_classes = 0;
	size_interfaces = 0;
}

function createJavaFileName(clsName_c) {
	return JAVA_GENERATED_SOURCE_DIR "/" myPkgD "/" clsName_c ".java";
}


function printHeader(outFile_h, javaPkg_h, javaClassName_h, isInterface_h,
		implementsInterface_h, isJniBound_h, isAbstract_h, implementsClass_h) {

	if (isInterface_h) {
		classOrInterface_h = "interface";
	} else if (isAbstract_h) {
		classOrInterface_h = "abstract class";
	} else {
		classOrInterface_h = "class";
	}

	extensionsPart_h = "";
	if (isInterface_h) {
		extensionsPart_h = " extends Comparable<" javaClassName_h ">";
	} else if (isAbstract_h) {
		extensionsPart_h = " implements " implementsInterface_h "";
	} else {
		extensionsPart_h = " extends " implementsClass_h " implements " implementsInterface_h;
	}

	printCommentsHeader(outFile_h);
	print("") >> outFile_h;
	print("package " javaPkg_h ";") >> outFile_h;
	print("") >> outFile_h;
	print("") >> outFile_h;
	print("import " myParentPkgA ".AICallbackException;") >> outFile_h;
	print("import " myParentPkgA ".AIFloat3;") >> outFile_h;
	if (isJniBound_h) {
		print("import " myMainPkgA ".AICallback;") >> outFile_h;
		print("import " myMainPkgA ".Util;") >> outFile_h;
	}
	print("") >> outFile_h;
	print("/**") >> outFile_h;
	print(" * @author	AWK wrapper script") >> outFile_h;
	print(" * @version	GENERATED") >> outFile_h;
	print(" */") >> outFile_h;
	print("public " classOrInterface_h " " javaClassName_h extensionsPart_h " {") >> outFile_h;
	print("") >> outFile_h;
}


function printTripleFunc(fRet_tr, fName_tr, fParams_tr, thrownExceptions_tr, outFile_int_tr, outFile_stb_tr, outFile_jni_tr, printIntAndStb_tr, noOverride_tr) {

	_funcHdr_tr = "public " fRet_tr " " fName_tr "(" fParams_tr ")";
	if (thrownExceptions_tr != "") {
		_funcHdr_tr = _funcHdr_tr " throws " thrownExceptions_tr;
	}

	if (printIntAndStb_tr) {
		print("\t" _funcHdr_tr ";") >> outFile_int_tr;
		print("") >> outFile_int_tr;

		print("\t" "@Override") >> outFile_stb_c;
		print("\t" _funcHdr_tr " {") >> outFile_stb_c;
		if (fRet_tr == "void") {
			# return nothing
		} else if (fRet_tr == "String") {
			print("\t\t" "return \"\";") >> outFile_stb_c;
		} else if (fRet_tr == "AIFloat3") {
			#print("\t\t" "return new AIFloat3(0.0f, 0.0f, 0.0f);") >> outFile_stb_c;
			print("\t\t" "return null;") >> outFile_stb_c;
		} else if (fRet_tr == "java.awt.Color") {
			#print("\t\t" "return java.awt.Color.BLACK;") >> outFile_stb_c;
			print("\t\t" "return null;") >> outFile_stb_c;
		} else if (startsWithCapital(fRet_tr)) {
			# must be a class
			print("\t\t" "return null;") >> outFile_stb_c;
		} else if (match(fRet_tr, /^java.util.List/)) {
			print("\t\t" "return null;") >> outFile_stb_c;
		} else if (match(fRet_tr, /^java.util.Map/)) {
			print("\t\t" "return null;") >> outFile_stb_c;
		} else if (fRet_tr == "boolean") {
			print("\t\t" "return false;") >> outFile_stb_c;
		} else {
			print("\t\t" "return 0;") >> outFile_stb_c;
		}
		print("\t" "}") >> outFile_stb_c;
		print("") >> outFile_stb_c;
	}

	if (!noOverride_tr) {
		print("\t" "@Override") >> outFile_jni_tr;
	}
	print("\t" _funcHdr_tr " {") >> outFile_jni_tr;
	print("") >> outFile_jni_tr;
}


function printClasses() {

	c_size_cs = cls_id_name["*"];
	for (c=0; c < c_size_cs; c++) {
		cls_cs      = cls_id_name[c];
		anc_size_cs = cls_name_implIds[cls_cs ",*"];
#if (c == 0) { print("now printing class: " cls_cs " ancs: " anc_size_cs); }

		printIntAndStb_cs = 1;
		for (a=0; a < anc_size_cs; a++) {
			implId_cs = cls_name_implIds[cls_cs "," a];
			printClass(implId_cs, cls_cs, printIntAndStb_cs);
			# only print interface and stub when printing the first impl-class
			printIntAndStb_cs = 0;
		}
	}
}


function printClass(implId_c, clsName_c, printIntAndStb_c) {

clsNameExternal_c = clsName_c;

clsId_c = ancestors_c "-" clsName_c;

	implCls_c = implId_c;
	sub(/^.*,/, "", implCls_c);

	clsName_int_c = clsName_c;
	clsName_abs_c = "Abstract" clsName_int_c;
	clsName_stb_c = "Stub" clsName_int_c;
	_fullClsName = cls_implId_fullClsName[implId_c];
	if (_fullClsName != clsName_c) {
		lastAncName_c = implId_c;
		sub(/,[^,]*$/, "", lastAncName_c); # remove class name
		sub(/^.*,/,    "", lastAncName_c); # remove pre last ancestor name
		noInterfaceIndices_c = lowerize(lastAncName_c) "Id";
	} else {
		noInterfaceIndices_c = 0;
	}
	clsName_jni_c = "Wrapp" _fullClsName;

#print("print class    : " clsName_jni_c);
if (printIntAndStb_c) {
	#print("print interface: " clsName_int_c);
	#print("print class    : " clsName_stb_c);
}

	if (printIntAndStb_c) {
		outFile_int_c = createJavaFileName(clsName_int_c);
		outFile_abs_c = createJavaFileName(clsName_abs_c);
		outFile_stb_c = createJavaFileName(clsName_stb_c);
#print("Printing interface: "  clsName_int_c);
#print("Printing stub class: " clsName_stb_c);
	}
#print("Printing wrap class: " clsName_jni_c);
	outFile_jni_c = createJavaFileName(clsName_jni_c);

	if (printIntAndStb_c) {
		printHeader(outFile_int_c, myPkgA, clsName_int_c, 1, 0,             0, 0, 0);
		printHeader(outFile_abs_c, myPkgA, clsName_abs_c, 0, clsName_int_c, 0, 1, 0);
		printHeader(outFile_stb_c, myPkgA, clsName_stb_c, 0, clsName_int_c, 0, 0, clsName_abs_c);
	}
	printHeader(    outFile_jni_c, myPkgA, clsName_jni_c, 0, clsName_int_c, 1, 0, clsName_abs_c);

#return;
outFile_c = outFile_jni_c;

	# prepare additional indices names
	addInds_size_c = split(cls_implId_indicesArgs[implId_c], addInds_c, ",");
	for (ai=1; ai <= addInds_size_c; ai++) {
		sub(/int /, "", addInds_c[ai]);
		addInds_c[ai] = trim(addInds_c[ai]);
	}
#if (addInds_size_c != 0) {
#	print("addInds_size_c last: " addInds_size_c " " addInds_c[addInds_size_c]);
#}

	myInnerClb = myClassVar ".getInnerCallback()";


	# print private vars
	print("\t" "private " myWrapClass " " myWrapVar " = null;") >> outFile_c;
	# print additionalVars
	for (ai=1; ai <= addInds_size_c; ai++) {
		print("\t" "private int " addInds_c[ai] " = -1;") >> outFile_c;
	}
	print("") >> outFile_c;


	# print constructor
	ctorParams   = myWrapClass " " myWrapVar;
	addIndPars_c = "";
	for (ai=1; ai <= addInds_size_c; ai++) {
		addIndPars_c = addIndPars_c ", int " addInds_c[ai];
	}
	ctorParams          = ctorParams addIndPars_c;
	ctorParamsNoTypes   = removeParamTypes(ctorParams);
	sub(/^, /, "", addIndPars_c);
	addIndParsNoTypes_c = removeParamTypes(addIndPars_c);
	condAddIndPars_c    = (addIndPars_c == "") ? "" : ", ";
	print("\t" "public " clsName_jni_c "(" ctorParams ") {") >> outFile_c;
	print("") >> outFile_c;
	print("\t\t" "this." myWrapVar " = " myWrapVar ";") >> outFile_c;
	# init additionalVars
	for (ai=1; ai <= addInds_size_c; ai++) {
		addIndName = addInds_c[ai];
		print("\t\t" "this." addIndName " = " addIndName ";") >> outFile_c;
	}
	print("\t" "}") >> outFile_c;
	print("") >> outFile_c;


	# print additional vars fetchers
	for (ai=1; ai <= addInds_size_c; ai++) {
		addIndName = addInds_c[ai];
		_fRet    = "int";
		_fName   = "get" capitalize(addIndName);
		_fParams = "";
		_fExceps = "";

		printIntAndStb_tmp_c = printIntAndStb_c;
		_noOverride = 0;
		if ((noInterfaceIndices_c != 0) && (addIndName == noInterfaceIndices_c)) {
			printIntAndStb_tmp_c = 0;
			_noOverride = 1;
		}
		printTripleFunc(_fRet, _fName, _fParams, _fExceps, outFile_int_c, outFile_stb_c, outFile_jni_c, printIntAndStb_tmp_c, _noOverride);

		print("\t\t" "return " addIndName ";") >> outFile_jni_c;
		print("\t" "}") >> outFile_jni_c;
		print("") >> outFile_jni_c;
	}

	# print static instance fetcher method
	{
		clsIsBuffered_c = isBufferedClass(clsNameExternal_c);
		fullNameAvailable_c = ancestorsClass_available[clsId_c];

		if (clsIsBuffered_c) {
			print("\t" "private static java.util.Map<Integer, " clsNameExternal_c "> _buffer_instances = new java.util.HashMap<Integer, " clsNameExternal_c ">();") >> outFile_c;
			print("") >> outFile_c;
		}
		print("\t" "public static " clsNameExternal_c " getInstance(" ctorParams ") {") >> outFile_c;
		print("") >> outFile_c;
		lastParamName = ctorParamsNoTypes;
		sub(/^.*,[ \t]*/, "", lastParamName);
		if (match(lastParamName, /^[^ \t]+Id$/)) {
			if (clsNameExternal_c == "Unit") {
				# the first valid unit ID is 1
				print("\t\t" "if (" lastParamName " <= 0) {") >> outFile_c;
			} else {
				# ... for all other IDs, the first valid one is 0
				print("\t\t" "if (" lastParamName " < 0) {") >> outFile_c;
			}
			print("\t\t\t" "return null;") >> outFile_c;
			print("\t\t" "}") >> outFile_c;
			print("") >> outFile_c;
		}
		print("\t\t" clsNameExternal_c " _ret = null;") >> outFile_c;
		if (fullNameAvailable_c == "") {
			print("\t\t" "_ret = new " clsName_jni_c "(" ctorParamsNoTypes ");") >> outFile_c;
		} else {
			print("\t\t" "boolean isAvailable = " myWrapVar ".getInnerCallback()." fullNameAvailable_c "(" addIndParsNoTypes_c ");") >> outFile_c;
			print("\t\t" "if (isAvailable) {") >> outFile_c;
			print("\t\t\t" "_ret = new " clsName_jni_c "(" ctorParamsNoTypes ");") >> outFile_c;
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


	if (printIntAndStb_c) {
		# print compareTo(other) method
		{
			print("\t" "@Override") >> outFile_abs_c;
			print("\t" "public int compareTo(" clsNameExternal_c " other) {") >> outFile_abs_c;
			print("\t\t" "final int BEFORE = -1;") >> outFile_abs_c;
			print("\t\t" "final int EQUAL  =  0;") >> outFile_abs_c;
			print("\t\t" "final int AFTER  =  1;") >> outFile_abs_c;
			print("") >> outFile_abs_c;
			print("\t\t" "if (this == other) return EQUAL;") >> outFile_abs_c;
			print("") >> outFile_abs_c;

			if (isClbRootCls) {
				print("\t\t" "if (this.skirmishAIId < other.skirmishAIId) return BEFORE;") >> outFile_abs_c;
				print("\t\t" "if (this.skirmishAIId > other.skirmishAIId) return AFTER;") >> outFile_abs_c;
				print("\t\t" "return EQUAL;") >> outFile_abs_c;
			} else {
				for (ai=1; ai <= addInds_size_c; ai++) {
					addIndName = addInds_c[ai];
					if ((noInterfaceIndices_c == 0) || (addIndName != noInterfaceIndices_c)) {
						print("\t\t" "if (this.get" capitalize(addIndName) "() < other.get" capitalize(addIndName) "()) return BEFORE;") >> outFile_abs_c;
						print("\t\t" "if (this.get" capitalize(addIndName) "() > other.get" capitalize(addIndName) "()) return AFTER;") >> outFile_abs_c;
					}
				}
				print("\t\t" "return 0;") >> outFile_abs_c;
			}
			print("\t" "}") >> outFile_abs_c;
			print("") >> outFile_abs_c;
		}


		# print equals(other) method
		if (!isClbRootCls) {
			print("\t" "@Override") >> outFile_abs_c;
			print("\t" "public boolean equals(Object otherObject) {") >> outFile_abs_c;
			print("") >> outFile_abs_c;
			print("\t\t" "if (this == otherObject) return true;") >> outFile_abs_c;
			print("\t\t" "if (!(otherObject instanceof " clsNameExternal_c ")) return false;") >> outFile_abs_c;
			print("\t\t" clsNameExternal_c " other = (" clsNameExternal_c ") otherObject;") >> outFile_abs_c;
			print("") >> outFile_abs_c;

			if (isClbRootCls) {
				print("\t\t" "if (this.skirmishAIId != other.skirmishAIId) return false;") >> outFile_abs_c;
				print("\t\t" "return true;") >> outFile_abs_c;
			} else {
				for (ai=1; ai <= addInds_size_c; ai++) {
					addIndName = addInds_c[ai];
					if ((noInterfaceIndices_c == 0) || (addIndName != noInterfaceIndices_c)) {
						print("\t\t" "if (this.get" capitalize(addIndName) "() != other.get" capitalize(addIndName) "()) return false;") >> outFile_abs_c;
					}
				}
				print("\t\t" "return true;") >> outFile_abs_c;
			}
			print("\t" "}") >> outFile_abs_c;
			print("") >> outFile_abs_c;
		}


		# print hashCode() method
		if (!isClbRootCls) {
			print("\t" "@Override") >> outFile_abs_c;
			print("\t" "public int hashCode() {") >> outFile_abs_c;
			print("") >> outFile_abs_c;

			if (isClbRootCls) {
				print("\t\t" "int _res = 0;") >> outFile_abs_c;
				print("") >> outFile_abs_c;
				print("\t\t" "_res += this.skirmishAIId * 10E8;") >> outFile_abs_c;
			} else {
				print("\t\t" "int _res = 23;") >> outFile_abs_c;
				print("") >> outFile_abs_c;
				# NOTE: This could go wrong if we have more then 7 additional indices
				# see 10E" (7-ai) below
				# the conversion to int is nessesarry,
				# as otherwise it would be a double,
				# which would be higher then max int,
				# and most hashes would end up being max int,
				# when converted from double to int
				for (ai=1; ai <= addInds_size_c; ai++) {
					addIndName = addInds_c[ai];
					if ((noInterfaceIndices_c == 0) || (addIndName != noInterfaceIndices_c)) {
						print("\t\t" "_res += this.get" capitalize(addIndName) "() * (int) (10E" (7-ai) ");") >> outFile_abs_c;
					}
				}
			}
			print("") >> outFile_abs_c;
			print("\t\t" "return _res;") >> outFile_abs_c;
			print("\t" "}") >> outFile_abs_c;
			print("") >> outFile_abs_c;
		}


		# print toString() method
		{
			print("\t" "@Override") >> outFile_abs_c;
			print("\t" "public String toString() {") >> outFile_abs_c;
			print("") >> outFile_abs_c;
			print("\t\t" "String _res = this.getClass().toString();") >> outFile_abs_c;
			print("") >> outFile_abs_c;

			#if (isClbRootCls) { # NO FOLD
			#	print("\t\t" "_res = _res + \"(skirmishAIId=\" + this.skirmishAIId + \", \";") >> outFile_abs_c;
			#} else { # NO FOLD
			#	print("\t\t" "_res = _res + \"(clbHash=\" + this." myWrapVar ".hashCode() + \", \";") >> outFile_abs_c;
			#	print("\t\t" "_res = _res + \"skirmishAIId=\" + this." myWrapVar ".SkirmishAI_getSkirmishAIId() + \", \";") >> outFile_abs_c;
				for (ai=1; ai <= addInds_size_c; ai++) {
					addIndName = addInds_c[ai];
					if ((noInterfaceIndices_c == 0) || (addIndName != noInterfaceIndices_c)) {
						print("\t\t" "_res = _res + \"" addIndName "=\" + this.get" capitalize(addIndName) "() + \", \";") >> outFile_abs_c;
					}
				}
			#} # NO FOLD
			print("\t\t" "_res = _res + \")\";") >> outFile_abs_c;
			print("") >> outFile_abs_c;
			print("\t\t" "return _res;") >> outFile_abs_c;
			print("\t" "}") >> outFile_abs_c;
			print("") >> outFile_abs_c;
		}
	}

	# make these available in called functions
	implId_c_         = implId_c;
	clsName_c_        = clsName_c;
	printIntAndStb_c_ = printIntAndStb_c;

	# print member functions
	members_size = cls_name_members[clsName_int_c ",*"];
#print("mems " clsName_int_c ": " members_size);
	for (m=0; m < members_size; m++) {
		memName_c  = cls_name_members[clsName_int_c "," m];
		fullName_c = implId_c "," memName_c;
		gsub(/,/, "_", fullName_c);
		if (doWrapp(fullName_c)) {
			printMember(fullName_c, memName_c, addInds_size_c);
		}
	}


	# print member class fetchers (single, multi, multi-fetch-single)
	size_memCls = split(ancestors_class[clsFull_c], memCls, ",");
	for (mc=0; mc < size_memCls; mc++) {
		memberClass_c = memCls[mc+1];
		#printMemberClassFetchers(clsFull_c, clsId_c, memberClass_c, isInterface_c);
	}


	# finnish up
	if (printIntAndStb_c) {
		print("}") >> outFile_int_c;
		print("") >> outFile_int_c;
		close(outFile_int_c);

		print("}") >> outFile_abs_c;
		print("") >> outFile_abs_c;
		close(outFile_abs_c);

		print("}") >> outFile_stb_c;
		print("") >> outFile_stb_c;
		close(outFile_stb_c);
	}
	print("}") >> outFile_jni_c;
	print("") >> outFile_jni_c;
	close(outFile_jni_c);
}

function printMemberClassFetchers(outFile_mc, clsFull_mc, clsId_mc, memberClsName_mc, isInterface_mc) {

		memberClassId_mc = clsFull_mc "-" memberClsName_mc;
		size_multi_mc = ancestorsClass_multiSizes[memberClassId_mc "*"];
		isMulti_mc = (size_multi_mc != "");
		if (isMulti_mc) {
			# multi element fetcher(s)
			for (mmc=0; mmc < size_multi_mc; mmc++) {
				fullNameMultiSize_mc = ancestorsClass_multiSizes[memberClassId_mc "#" mmc];
				printMemberClassFetcher(outFile_mc, clsFull_mc, clsId_mc, memberClsName_mc, isInterface_mc, fullNameMultiSize_mc);
			}
		} else {
			# single element fetcher
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
		if (isMulti_mf) {
			# multi element fetcher
			if (match(fullNameMultiSize_mf, /^.*0MULTI1[^0]*3/)) {
				# wants a different function name then the default one
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
		} else {
			# single element fetcher
			fn = "get" memberClsName_mf;
			params = "";
		}

		# remove additional indices from params
		for (ai=0; ai < addInds_size_c; ai++) {
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
		for (ai=0; ai < addInds_size_c; ai++) {
			addIndName = additionalClsIndices[clsId_mf "#" ai];
			indexParams_mf = indexParams_mf  ", " addIndName;
		}
		sub(/^\, /, "", indexParams_mf); # remove comma at the end
		condIndexComma_mf = (indexParams_mf == "") ? "" : ", ";

		indent_mf = "\t";

		print("") >> outFile_mf;

		isBuffered_mf = isBufferedFunc(clsFull_mf "_" memberClsName_mf) && (params == "");
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
			print(indent_mf "int size = " myWrapVar "." fullNameMultiSize_mf "(" mySkirmishAIId condInnerParamsComma innerParams ");") >> outFile_mf;
			if (hasMultiVals_mf) {
				print(indent_mf "int[] tmpArr = new int[size];") >> outFile_mf;
				print(indent_mf myWrapVar "." fullNameMultiVals_mf "(" mySkirmishAIId condInnerParamsComma innerParams ", tmpArr, size);") >> outFile_mf;
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



function isRetParamName(paramName_rp) {
	return (match(paramName_rp, /_out(_|$)/) || match(paramName_rp, /(^|_)?ret_/));
}

# UNUSED
function cleanRetParamName(paramName_rp) {

	paramNameClean_rp = paramName_rp;

	sub(/_out(_|$)/,  "", paramNameClean_rp);
	sub(/(^|_)?ret_/, "", paramNameClean_rp);

	return paramNameClean_rp;
}

function printMember(fullName_m, memName_m, additionalIndices_m) {
#print("Printing member: " fullName_m);
	# use some vars from the printClass function (which called us)
	implId_m         = implId_c_;
	clsName_m        = clsName_c_;
	printIntAndStb_m = printIntAndStb_c_;
	implCls_m        = implCls_c;
	clsName_int_m    = clsName_int_c;
	clsName_stb_m    = clsName_stb_c;
	clsName_jni_m    = clsName_jni_c;
	outFile_int_m    = outFile_int_c;
	outFile_stb_m    = outFile_stb_c;
	outFile_jni_m    = outFile_jni_c;
	addInds_size_m   = addInds_size_c;
	for (ai=1; ai <= addInds_size_m; ai++) {
		addInds_m[ai] = addInds_c[ai];
	}

	indent_m            = "\t";
	memId_m             = clsName_m "," memName_m;
	retType             = cls_memberId_retType[memId_m]; # this may be changed
	retType_int         = retType;                       # this is a const var
	params              = cls_memberId_params[memId_m];
	isFetcher           = cls_memberId_isFetcher[memId_m];
	metaComment         = cls_memberId_metaComment[memId_m];
#print("retType: " retType);
	memName             = fullName_m;
	sub(/^.*_/, "", memName);
	functionName_m      = fullName_m;
	sub(/^[^_]+_/, "", functionName_m);

	isVoid_int_m        = (retType_int == "void");

	retVar_int_m        = "_ret_int";   # this is a const var
	retVar_out_m        = retVar_int_m; # this may be changed
	declaredVarsCode    = "";
	conversionCode_pre  = "";
	conversionCode_post = "";
	thrownExceptions    = "";
	ommitMainCall       = 0;
	
	if (!isVoid_int_m) {
		declaredVarsCode = "\t\t" retType_int " " retVar_int_m ";" "\n" declaredVarsCode;
	}


	# Rewrite meta comment
	if (match(metaComment, /FETCHER:MULTI:IDs:/)) {
		# convert this: FETCHER:MULTI:IDs:Group:groupIds
		# to this:      ARRAY:groupIds->Group
#print("");
#print("metaComment: " metaComment);
		_mc_pre  = metaComment;
		sub(/FETCHER:MULTI:IDs:.*$/, "", _mc_pre);
		_mc_fet  = metaComment;
		sub(/^.*FETCHER:MULTI:IDs:/, "", _mc_fet);
		sub(/[ \t].*$/, "", _mc_fet);
		_mc_post = metaComment;
		sub(/^.*FETCHER:MULTI:IDs:[^ \t]*/, "", _mc_post);

		_refObj = _mc_fet;
		sub(/:.*$/, "", _refObj);
		_refPaNa = _mc_fet;
		sub(/^.*:/, "", _refPaNa);

		_mc_newArr = "ARRAY:" _refPaNa "->" _refObj;
		metaComment = _mc_pre _mc_newArr _mc_post;
#print("metaComment: " metaComment);
	}
	if (match(metaComment, /FETCHER:MULTI:NUM:/)) {
		# convert this: FETCHER:MULTI:NUM:Resource
		# to this:      ARRAY:RETURN_SIZE->Resource
		sub(/FETCHER:MULTI:NUM:/, "ARRAY:RETURN_SIZE->", metaComment);
	}
	if (match(metaComment, /REF:MULTI:/)) {
		# convert this: REF:MULTI:unitDefIds->UnitDef
		# to this:      ARRAY:unitDefIds->UnitDef
		sub(/REF:MULTI:/, "ARRAY:", metaComment);
	}

outFile_m = outFile_jni_m;

	# remove additional indices from the outter params
	for (ai=1; ai <= addInds_size_m; ai++) {
		_removed = sub(/[^,]+(, )?/, "", params);
		if (!_removed && !part_isStatic(memName_m, metaComment)) {
			addIndName = addInds_m[ai];
			print("ERROR: failed removing additional indices " addIndName " from method " memName_m " in class " clsName_int_m);
		}
	}

	innerParams         = removeParamTypes(params);

	# add additional indices fetcher calls to inner params
	addInds_real_size_m = addInds_size_m;
	if (part_isStatic(memName_m, metaComment)) {
		addInds_real_size_m--;
	}
	for (ai=1; ai <= addInds_real_size_m; ai++) {
		addIndName = addInds_m[ai];
		_condComma = "";
		if (innerParams != "") {
			_condComma = ", ";
		}
		innerParams = "this.get" capitalize(addIndName) "()" _condComma innerParams;
	}


	# convert param types
	paramNames_size = split(innerParams, paramNames, ", ");
	for (prm = 1; prm <= paramNames_size; prm++) {
		paNa = paramNames[prm];
		if (!isRetParamName(paNa)) {
			if (match(paNa, /_posF3/)) {
				# convert float[3] to AIFloat3
				paNaNew = paNa;
				sub(/_posF3/, "", paNaNew);
				sub("float\\[\\] " paNa, "AIFloat3 " paNaNew, params);
				conversionCode_pre = conversionCode_pre "\t\t" "float[] " paNa " = " paNaNew ".toFloatArray();" "\n";
			} else if (match(paNa, /_colorS3/)) {
				# convert short[3] to java.awt.Color
				paNaNew = paNa;
				sub(/_colorS3/, "", paNaNew);
				sub("short\\[\\] " paNa, "java.awt.Color " paNaNew, params);
				conversionCode_pre = conversionCode_pre "\t\t" "short[] " paNa " = Util.toShort3Array(" paNaNew ");" "\n";
			}
		}
	}

	# convert an error return int value to an Exception
	#metaComment "error-return:0=OK"
	if (part_isErrorReturn(metaComment) && retType == "int") {
		errorRetValueOk_m = part_getErrorReturnValueOk(metaComment);

		conversionCode_post = conversionCode_post "\t\t" "if (" retVar_out_m " != " errorRetValueOk_m ") {" "\n";
		conversionCode_post = conversionCode_post "\t\t\t" "throw new AICallbackException(\"" memName_m "\", " retVar_out_m ");" "\n";
		conversionCode_post = conversionCode_post "\t\t" "}" "\n";
		thrownExceptions = thrownExceptions ", AICallbackException" thrownExceptions;

		retType = "void";
	}

	# convert out params to return values
	paramTypeNames_size = split(params, paramTypeNames, ", ");
	hasRetParam = 0;
	for (prm = 1; prm <= paramTypeNames_size; prm++) {
		paNa = extractParamName(paramTypeNames[prm]);
		if (isRetParamName(paNa)) {
			if (retType == "void") {
				paTy = extractParamType(paramTypeNames[prm]);
				hasRetParam = 1;
				if (match(paNa, /_posF3/)) {
					# convert float[3] to AIFloat3
					retParamType = "AIFloat3";
					retVar_out_m = "_ret";
					conversionCode_pre  = conversionCode_pre  "\t\t" "float[] " paNa " = new float[3];" "\n";
					conversionCode_post = conversionCode_post "\t\t" retVar_out_m " = new AIFloat3(" paNa "[0], " paNa "[1]," paNa "[2]);" "\n";
					declaredVarsCode = "\t\t" retParamType " " retVar_out_m ";" "\n" declaredVarsCode;
					sub("(, )?float\\[\\] " paNa, "", params);
					retType = retParamType;
				} else if (match(paNa, /_colorS3/)) {
					retParamType = "java.awt.Color";
					retVar_out_m = "_ret";
					conversionCode_pre  = conversionCode_pre  "\t\t" "short[] " paNa " = new short[3];" "\n";
					conversionCode_post = conversionCode_post "\t\t" retVar_out_m " = Util.toColor(" paNa ");" "\n";
					declaredVarsCode = "\t\t" retParamType " " retVar_out_m ";" "\n" declaredVarsCode;
					sub("(, )?short\\[\\] " paNa, "", params);
					retType = retParamType;
				} else {
					print("FAILED converting return param: " paramTypeNames[prm] " / " fullName_m);
					exit(1);
				}
#print(paNa);
			} else {
				print("FAILED converting return param: return type should be \"void\", but is \"" retType "\"");
				exit(1);
			}
		}
	}

	# REF:
	refObjs_size_m = split(metaComment, refObjs_m, "REF:");
	for (ro=2; ro <= refObjs_size_m; ro++) {
		_ref = refObjs_m[ro];
		sub(/[ \t].*$/, "", _ref); # remove parts after this REF part
#print("hasReferenceObject: REF: " _ref);
		_isMulti  = match(_ref, /MULTI:/);
		_isReturn = match(_ref, /RETURN->/);

		if (!_isMulti && !_isReturn) {
			# convert single param reference
			_refRel = _ref;
			sub(/^.*:/, "", _refRel);
			_paNa = _refRel;            # example: resourceId
			sub(/->.*$/, "", _paNa);
			_refObj = _refRel;         # example: Resource
			sub(/^.*->/, "", _refObj);
			_paNaNew = _paNa;
			if (!sub(/Id$/, "", _paNaNew)) {
				_paNaNew = "oo_" _paNaNew;
			}
#print("");
#print("REF: SINGLE: #int " _paNa "#" _refObj " " _paNaNew "");
#print("REF: SINGLE: " _ref);
#print("");

			if (_refObj != "Team" && _refObj != "FigureGroup" && _refObj != "Path") {
				_paNa_found = sub("int " _paNa, _refObj " " _paNaNew, params);
				# it may not be found if it is an output parameter
				if (_paNa_found) {
					conversionCode_pre = conversionCode_pre "\t\t"  "int " _paNa " = " _paNaNew ".get" _refObj "Id();" "\n";
				}
			} else {
				print("note: ignoring meta comment: REF:" _ref);
			}
		} else if (!_isMulti && _isReturn) {
#print("isReturn: REF: " _ref);
			_refObj = _ref;         # example: Resource
			sub(/^.*->/, "", _refObj);
			_implId = implId_m "," _refObj;
			if (_implId in cls_implId_fullClsName) {
				_fullClsName = cls_implId_fullClsName[_implId];
			} else if (cls_name_implIds[_refObj ",*"] == 1) {
				_fullClsName = cls_name_implIds[_refObj ",0"];
				_fullClsName = cls_implId_fullClsName[_fullClsName];
			} else {
				print("ERROR: failed finding the full class name for: " _refObj);
				exit(1);
			}
#print("_fullClsName: " _implId " " _fullClsName);

			_retVar_out_new = retVar_out_m "_out";
			_wrappGetInst_params = myWrapVar;
			if (retType != "void") {
				_wrappGetInst_params = _wrappGetInst_params ", " retVar_out_m;
			} else {
				ommitMainCall = 1;
			}
			conversionCode_post = conversionCode_post "\t\t" _retVar_out_new " = Wrapp" _fullClsName ".getInstance(" _wrappGetInst_params ");" "\n";
			declaredVarsCode = "\t\t" _refObj " " _retVar_out_new ";" "\n" declaredVarsCode;
			retVar_out_m = _retVar_out_new;
			retType = _refObj;
		} else {
print("WARNING: unsupported: REF:" _ref);
		}
	}


	isMap = part_isMap(fullName_m, metaComment);
	if (isMap) {
#print("isMap::fullName_m: " fullName_m);

		_isFetching = 1;
		_isRetSize  = 0;
		_isObj      = 0;
		_mapVar_size      = "_size";
		_mapVar_keys      = "keys";
		_mapVar_values    = "values";
		_mapType_key      = "String";
		_mapType_value    = "String";
		_mapType_oo_key   = "String";
		_mapType_oo_value = "String";
		_mapVar_oo        = "_map";
		_mapType_int      = "java.util.Map<"     _mapType_oo_key ", " _mapType_oo_value ">";
		_mapType_impl     = "java.util.HashMap<" _mapType_oo_key ", " _mapType_oo_value ">";

		sub("(, )?" _mapType_key   "\\[\\] " _mapVar_keys,   "", params);
		sub("(, )?" _mapType_value "\\[\\] " _mapVar_values, "", params);
		sub(/, $/                                      , "", params);

		declaredVarsCode = "\t\t" "int " _mapVar_size ";" "\n" declaredVarsCode;
		if (_isFetching) {
			declaredVarsCode = "\t\t" _mapType_int " " _mapVar_oo ";" "\n" declaredVarsCode;
		}
		if (!_isRetSize) {
			declaredVarsCode = "\t\t" _mapType_key   "[] " _mapVar_keys ";" "\n" declaredVarsCode;
			declaredVarsCode = "\t\t" _mapType_value "[] " _mapVar_values ";" "\n" declaredVarsCode;
			if (_isFetching) {
				conversionCode_pre = conversionCode_pre "\t\t" _mapVar_keys   " = null;" "\n";
				conversionCode_pre = conversionCode_pre "\t\t" _mapVar_values " = null;" "\n";
				conversionCode_pre = conversionCode_pre "\t\t" _mapVar_size   " = " myWrapVar "." functionName_m "(" innerParams ");" "\n";
			} else {
				#conversionCode_pre = conversionCode_pre "\t\t" _arraySizeVar " = " _arrayListVar ".size();" "\n";
				#conversionCode_pre = conversionCode_pre "\t\t" "int _size = " _arraySizeVar ";" "\n";
			}
		}

		if (_isRetSize) {
			#conversionCode_post = conversionCode_post "\t\t" _arraySizeVar " = " retVar_out_m ";" "\n";
			#_arraySizeMaxPaNa = _arraySizeVar;
		} else {
			conversionCode_pre = conversionCode_pre "\t\t" _mapVar_keys   " = new " _mapType_key   "[" _mapVar_size "];" "\n";
			conversionCode_pre = conversionCode_pre "\t\t" _mapVar_values " = new " _mapType_value "[" _mapVar_size "];" "\n";
		}

		if (_isFetching) {
			# convert to a HashMap
			conversionCode_post = conversionCode_post "\t\t" _mapVar_oo " = new " _mapType_impl "();" "\n";
			conversionCode_post = conversionCode_post "\t\t" "for (int i=0; i < " _mapVar_size "; i++) {" "\n";
			if (_isObj) {
				if (_isRetSize) {
					conversionCode_post = conversionCode_post "\t\t\t" _mapVar_oo ".put(" _mapVar_keys "[i], " _mapVar_values "[i]);" "\n";
				} else {
					#conversionCode_post = conversionCode_post "\t\t\t" _mapVar_oo ".put(" myPkgA ".Wrapp" _refObj ".getInstance(" myWrapVar _addWrappVars ", " _arrayPaNa "[i]));" "\n";
				}
			} else if (_isNative) {
				#conversionCode_post = conversionCode_post "\t\t\t" _arrayListVar ".add(" _arrayPaNa "[i]);" "\n";
			}
			conversionCode_post = conversionCode_post "\t\t" "}" "\n";

			retParamType = _mapType_int;
			retVar_out_m = _mapVar_oo;
			retType = retParamType;
		} else {
			# convert from a HashMap
		}
	}


	isArray = part_isArray(fullName_m, metaComment);
#print("metaComment: " metaComment);
	if (isArray) {
#print("");
#print("isArray::fullName_m: " fullName_m);
#print("isArray::params: " params);
		_refObj = "";
		_arrayPaNa = metaComment;
		_addWrappVars = "";
		sub(/^.*ARRAY:/, "", _arrayPaNa);
		sub(/[ \t].*$/,  "", _arrayPaNa);
		if (match(_arrayPaNa, /->/)) {
			_refObj = _arrayPaNa;
			sub(/->.*$/, "", _arrayPaNa);
			sub(/^.*->/, "", _refObj);
			_refObjInt = _refObj;

			if (match(_refObj, /-/)) {
				sub(/-.*$/, "", _refObj);
				sub(/^.*-/, "", _refObjInt);
			}
			_implId = implId_m "," _refObj;
			if (_implId in cls_implId_fullClsName) {
				_fullClsName = cls_implId_fullClsName[_implId];
			} else if ((myRootClass "," _refObj) in cls_implId_fullClsName) {
				_implId = myRootClass "," _refObj;
				_fullClsName = cls_implId_fullClsName[_implId];
			} else {
				print("ERROR: failed to find the full class name for " _refObj " in " fullName_m);
				exit(1);
			}
#print("Special Array: #0:" fullName_m "#1:" _arrayPaNa "#2:" _refObj "#3:" _refObjInt "#4:" clsName_m "#5:" implCls_m "#6:" implId_m "#7:" _fullClsName "#");
			_refObj = _fullClsName;
			_addWrappVars = cls_implId_indicesArgs[_implId];
			sub(/(,)?[^,]*$/, "", _addWrappVars); # remove last index
			_addWrappVars = trim(removeParamTypes(_addWrappVars));
			if (_addWrappVars != "") {
				_addWrappVars = ", " _addWrappVars;
			}
		}

		_isF3     = match(_arrayPaNa, /_AposF3/);
		_isObj    = (_refObj != "");
		_isNative = (!_refObj && !_isObjc);

		_isRetSize = 0;
		if (_isObj) {
			_isRetSize = (_arrayPaNa == "RETURN_SIZE");
		}

		_arrayType = params;
		sub("\\[\\][ \t]" _arrayPaNa ".*$", "", _arrayType);
		sub("^.*[ \t]", "", _arrayType);
#if (match(params, / $/)) { print("params with space at end 1: " params); }
		_arraySizeMaxPaNa = _arrayPaNa "_sizeMax";
		_arraySizeVar     = _arrayPaNa "_size";
#print("isArray: #" fullName_m "#" params "#" _arrayType "#" _arrayPaNa "#");

		_arrayListVar    = _arrayPaNa "_list";
		if (_isF3) {
			_arrListGenType = "AIFloat3";
		} else if (_isObj) {
			_arrListGenType = _refObjInt;
		} else if (_isNative) {
			_arrListGenType  = convertJavaBuiltinTypeToClass(_arrayType);
		}
		_arrListType     = "java.util.List<" _arrListGenType ">";    
		_arrListImplType = "java.util.ArrayList<" _arrListGenType ">";
#if (fullName_m == "OOAICallback_UnitDef_getBuildOptions") {
#	print("OOAICallback_UnitDef_getBuildOptions: #" _arrListType "#" _arrListImplType "#" _isObj "#" _isObj "#");
#}

		_isFetching = sub("(, )?int " _arraySizeMaxPaNa, "", params);
		if (_isRetSize) {
			_isFetching = 1;
		} else {
			if (!_isFetching && !_isRetSize) {
				_isNonFetcher = sub("(, )?int " _arraySizeVar, "", params);
				if (!_isNonFetcher) {
					print("ERROR: neither propper fetcher nor supplier ARRAY syntax in function: " fullName_m);
					exit(1);
				}
			}
			if (_isFetching) {
				sub(_arrayType "\\[\\] " _arrayPaNa, "", params);
			} else {
				sub(_arrayType "\\[\\] " _arrayPaNa, _arrListType " " _arrayListVar, params);
			}
			sub(/^, /, "", params);
			sub(/, $/, "", params);
		}

		declaredVarsCode = "\t\t" "int " _arraySizeVar ";" "\n" declaredVarsCode;
		if (_isFetching) {
			declaredVarsCode = "\t\t" _arrListType " " _arrayListVar ";" "\n" declaredVarsCode;
		}
		if (!_isRetSize) {
			declaredVarsCode = "\t\t" _arrayType "[] " _arrayPaNa ";" "\n" declaredVarsCode;
			if (_isFetching) {
				declaredVarsCode = "\t\t" "int " _arraySizeMaxPaNa ";" "\n" declaredVarsCode;
				conversionCode_pre = conversionCode_pre "\t\t" _arraySizeMaxPaNa " = Integer.MAX_VALUE;" "\n";
				conversionCode_pre = conversionCode_pre "\t\t" _arrayPaNa " = null;" "\n";
				conversionCode_pre = conversionCode_pre "\t\t" _arraySizeVar " = " myWrapVar "." functionName_m "(" innerParams ");" "\n";
				conversionCode_pre = conversionCode_pre "\t\t" _arraySizeMaxPaNa " = " _arraySizeVar ";" "\n";
				if (_isF3) {
					conversionCode_pre = conversionCode_pre "\t\t" "if (" _arraySizeVar " % 3 != 0) {" "\n";
					conversionCode_pre = conversionCode_pre "\t\t\t" "throw new RuntimeException(\"returned AIFloat3 array has incorrect size (\" + " _arraySizeVar "+ \"), should be a multiple of 3.\");" "\n";
					conversionCode_pre = conversionCode_pre "\t\t" "}" "\n";
					conversionCode_pre = conversionCode_pre "\t\t" _arraySizeVar " /= 3;" "\n";
				}
			} else {
				conversionCode_pre = conversionCode_pre "\t\t" _arraySizeVar " = " _arrayListVar ".size();" "\n";
				conversionCode_pre = conversionCode_pre "\t\t" "int _size = " _arraySizeVar ";" "\n";
				if (_isF3) {
					conversionCode_pre = conversionCode_pre "\t\t" _arraySizeVar " *= 3;" "\n";
				}
			}
		}

		if (_isRetSize) {
			conversionCode_post = conversionCode_post "\t\t" _arraySizeVar " = " retVar_out_m ";" "\n";
			_arraySizeMaxPaNa = _arraySizeVar;
		} else {
			conversionCode_pre = conversionCode_pre "\t\t" _arrayPaNa " = new " _arrayType "[" _arraySizeVar "];" "\n";
		}

		if (_isFetching) {
			# convert to an ArrayList
			conversionCode_post = conversionCode_post "\t\t" _arrayListVar " = new " _arrListImplType "(" _arraySizeVar ");" "\n";
			conversionCode_post = conversionCode_post "\t\t" "for (int i=0; i < " _arraySizeMaxPaNa "; i++) {" "\n";
			if (_isF3) {
				conversionCode_post = conversionCode_post "\t\t\t" _arrayListVar ".add(new AIFloat3(" _arrayPaNa "[i], " _arrayPaNa "[++i], " _arrayPaNa "[++i]));" "\n";
			} else if (_isObj) {
				if (_isRetSize) {
					conversionCode_post = conversionCode_post "\t\t\t" _arrayListVar ".add(" myPkgA ".Wrapp" _refObj ".getInstance(" myWrapVar _addWrappVars ", i));" "\n";
				} else {
					conversionCode_post = conversionCode_post "\t\t\t" _arrayListVar ".add(" myPkgA ".Wrapp" _refObj ".getInstance(" myWrapVar _addWrappVars ", " _arrayPaNa "[i]));" "\n";
				}
			} else if (_isNative) {
				conversionCode_post = conversionCode_post "\t\t\t" _arrayListVar ".add(" _arrayPaNa "[i]);" "\n";
			}
			conversionCode_post = conversionCode_post "\t\t" "}" "\n";

			retParamType = _arrListType;
			retVar_out_m = _arrayListVar;
			retType = retParamType;
		} else {
			# convert from an ArrayList
			conversionCode_pre = conversionCode_pre "\t\t" "for (int i=0; i < _size; i++) {" "\n";
			if (_isF3) {
				conversionCode_pre = conversionCode_pre "\t\t\t" "int arrInd = i*3;" "\n";
				conversionCode_pre = conversionCode_pre "\t\t\t" "AIFloat3 aif3 = " _arrayListVar ".get(i);" "\n";
				conversionCode_pre = conversionCode_pre "\t\t\t" _arrayPaNa "[arrInd]   = aif3.x;" "\n";
				conversionCode_pre = conversionCode_pre "\t\t\t" _arrayPaNa "[arrInd+1] = aif3.y;" "\n";
				conversionCode_pre = conversionCode_pre "\t\t\t" _arrayPaNa "[arrInd+2] = aif3.z;" "\n";
			} else if (_isObj) {
				conversionCode_pre = conversionCode_pre "\t\t\t" _arrayPaNa "[i] = " _arrayListVar ".get(i).get" _refObj "Id();" "\n";
			} else if (_isNative) {
				conversionCode_pre = conversionCode_pre "\t\t\t" _arrayPaNa "[i] = " _arrayListVar ".get(i);" "\n";
			}
			conversionCode_pre = conversionCode_pre "\t\t" "}" "\n";
		}

		#convCode = "";

		#if (_isFetching) {
		#	conversionCode_pre = conversionCode_pre convCode;
		#} else {
		#	conversionCode_pre = conversionCode_pre convCode;
		#}

		#retParamType = _arrayType "[]";
		#retVar_out_m = _arrayPaNa;
		#retType = retParamType;
	}

	# remove additional indices from params
	#for (ai=0; ai < additionalIndices_m; ai++) {
	#	sub(/^int [_a-zA-Z0-9]+(, )?/, "", params);
	#}

	firstLineEnd = ";";
	mod_m = "";
	if (!isInterface_m) {
		firstLineEnd = " {";
		mod_m = "public ";
	}

	#retTypeInterface = trim(retType);
	#if (retTypeInterface == "") {
	#	retTypeInterface = retType;
	#}
#print("retType: " retType);
#print("retTypeInterface: " retTypeInterface);

	sub(/^, /, "", thrownExceptions);

	print("") >> outFile_m;

	isBuffered_m = !isVoid_m && isBufferedFunc(fullName_m) && (params == "");
	if (!isInterface_m && isBuffered_m) {
		print(indent_m retType " _buffer_" memName ";") >> outFile_m;
		print(indent_m "boolean _buffer_isInitialized_" memName " = false;") >> outFile_m;
	}

	# print method doc comment
	fullName_doc_m = fullName_m;
	sub(/^[^_]*_/, "", fullName_doc_m); # remove OOAICallback_
	if (printIntAndStb_m) {
		printFunctionComment_Common(outFile_int_m, funcDocComment, fullName_doc_m, indent_m);
		printFunctionComment_Common(outFile_stb_m, funcDocComment, fullName_doc_m, indent_m);
	}
	printFunctionComment_Common(outFile_jni_m, funcDocComment, fullName_doc_m, indent_m);

	#print(indent_m mod_m retTypeInterface " " memName "(" params ")" firstLineEnd) >> outFile_m;
	printTripleFunc(retType, memName, params, thrownExceptions, outFile_int_m, outFile_stb_m, outFile_jni_m, printIntAndStb_m, 0);

	isVoid_m            = (retType == "void");

	if (!isInterface_m) {
		condRet_int_m = isVoid_int_m ? "" : retVar_int_m " = ";
		indent_m = indent_m "\t";

		if (isBuffered_m) {
			#_retVar_out_new = retVar_out_m "_buff";
			#print(indent_m retType " " _retVar_out_new " = _buffer_" memName ";") >> outFile_m;
			print(indent_m "if (!_buffer_isInitialized_" memName ") {") >> outFile_m;
			indent_m = indent_m "\t";
		}
		if (declaredVarsCode != "") {
			print(declaredVarsCode) >> outFile_m;
		}
		if (conversionCode_pre != "") {
			print(conversionCode_pre) >> outFile_m;
		}
		if (isArray && 1 == 0) {
			print("") >> outFile_m;
			print(indent_m "int size = " myWrapVar "." fullNameArraySize "("innerParams ");") >> outFile_m;
			print(indent_m retType " arrList = new " retType "(size);") >> outFile_m;
			print(indent_m "if (size > 0) {") >> outFile_m;
			indent_m = indent_m "\t";
			print(indent_m arrayType "[] tmpArr = new " arrayType "[size];") >> outFile_m;
			print(indent_m myWrapVar "." fullNameArrayVals "(" innerParams ", tmpArr, size);") >> outFile_m;
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
		} else if (isMap && 1 == 0) {
			print("") >> outFile_m;
			print(indent_m "int size = " myWrapVar "." fullNameMapSize "(" innerParams ");") >> outFile_m;
			retMapImplType = retType;
			sub(/Map/, defMapJavaImpl, retMapImplType);
			print(indent_m retType " retMap = new " retMapImplType "(size);") >> outFile_m;
			print(indent_m "if (size > 0) {") >> outFile_m;
			indent_m = indent_m "\t";
			print(indent_m keyType "[] tmpKeysArr = new " keyType "[size];") >> outFile_m;
			print(indent_m myWrapVar "." fullNameMapKeys "(" innerParams ", tmpKeysArr);") >> outFile_m;
			print(indent_m valType "[] tmpValsArr = new " valType "[size];") >> outFile_m;
			print(indent_m myWrapVar "." fullNameMapVals "(" innerParams ", tmpValsArr);") >> outFile_m;
			print(indent_m "for (int i=0; i < size; i++) {") >> outFile_m;
			print(indent_m "\t" "retMap.put(tmpKeysArr[i], tmpValsArr[i]);") >> outFile_m;
			print(indent_m "}") >> outFile_m;
			sub(/\t/, "", indent_m);
			print(indent_m "}") >> outFile_m;
			print(indent_m "_ret = retMap;") >> outFile_m;
		} else if (isSingleFetch && 1 == 0) {
			condInstanceInnerParamsComma = (instanceInnerParams == "") ? "" : ", ";
			print("") >> outFile_m;
			print(indent_m innerRetType " innerRet = " myWrapVar "." functionName_m "(" innerParams ");") >> outFile_m;
			print(indent_m "_ret = " retType ".getInstance(" myClassVarLocal ", innerRet" condInstanceInnerParamsComma instanceInnerParams ");") >> outFile_m;
		} else {
			if (!ommitMainCall) {
				print(indent_m condRet_int_m myWrapVar "." functionName_m "(" innerParams ");") >> outFile_m;
			}
		}
		if (conversionCode_post != "") {
			print(conversionCode_post) >> outFile_m;
		}
		if (isBuffered_m) {
			print(indent_m "_buffer_" memName " = " retVar_out_m ";") >> outFile_m;
			print(indent_m "_buffer_isInitialized_" memName " = true;") >> outFile_m;
			sub(/\t/, "", indent_m);
			print(indent_m "}") >> outFile_m;
			print("") >> outFile_m;
			retVar_out_m = "_buffer_" memName;
		}
		if (!isVoid_m) {
			print(indent_m "return " retVar_out_m ";") >> outFile_m;
		}
		sub(/\t/, "", indent_m);
		print(indent_m "}") >> outFile_m;
	}
}


# Used by the common OO AWK script
function doWrapp(funcFullName_dw, params_dw, metaComment_dw) {

	doWrapp_dw = 1;

	#doWrapp_dw = doWrapp_dw && !match(params_dw, /String\[\]/);
	doWrapp_dw = doWrapp_dw && !match(funcFullName_dw, /Lua_callRules/);

	return doWrapp_dw;
}

function wrappFunctionDef(funcDef, commentEolTot) {

	size_funcParts = split(funcDef, funcParts, "(\\()|(\\)\\;)");
	# because the empty part after ");" would count as part as well
	size_funcParts--;

	fullName = funcParts[1];
	fullName = trim(fullName);
	sub(/.*[ \t]+/, "", fullName);

	retType = funcParts[1];
	sub(/[ \t]*public/, "", retType);
	sub(fullName, "", retType);
	retType = trim(retType);

	params = funcParts[2];

	wrappFunctionPlusMeta(retType, fullName, params, commentEolTot);
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
		# separate possible comment at end of line: // fu bar
		commentEol = funcIntermLine;
		if (sub(/.*\/\//, "", commentEol)) {
			commentEolTot = commentEolTot commentEol;
		}
		sub(/[ \t]*\/\/.*$/, "", funcIntermLine);
		funcIntermLine = trim(funcIntermLine);
		funcSoFar = funcSoFar " " funcIntermLine;
		if (match(funcSoFar, /\;$/)) {
			# function ends in this line
			wrappFunctionDef(funcSoFar, commentEolTot);
			isMultiLineFunc = 0;
		}
	}
}
# 1st line of a function definition
/\tpublic .*);/ {

	funcStartLine = $0;
	# separate possible comment at end of line: // foo bar
	commentEolTot = "";
	commentEol = funcStartLine;
	if (sub(/.*\/\//, "", commentEol)) {
		commentEolTot = commentEolTot commentEol;
	}
	# remove possible comment at end of line: // foo bar
	sub(/\/\/.*$/, "", funcStartLine);
	funcStartLine = trim(funcStartLine);
	if (match(funcStartLine, /\;$/)) {
		# function ends in this line
		wrappFunctionDef(funcStartLine, commentEolTot);
	} else {
		funcSoFar = funcStartLine;
		isMultiLineFunc = 1;
	}
}



END {
	# finalize things
	store_everything();
	printClasses();
}
