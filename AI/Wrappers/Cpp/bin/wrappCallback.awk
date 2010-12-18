#!/usr/bin/awk -f
#
# This awk script creates a C++ class hirarchy to wrapp the C AI Callback.
# In other words, the output of this file wrapps:
# rts/ExternalAI/Interface/SSkirmishAICallback.h
#
# This script uses functions from the following files:
# * common.awk
# * commonDoc.awk
# * commonOOCallback.awk
# Variables that can be set on the command-line (with -v):
# * GENERATED_SOURCE_DIR: will contain the generated sources
#
# usage:
# 	awk -f thisScript.awk -f common.awk -f commonDoc.awk -f commonOOCallback.awk
# 	awk -f thisScript.awk -f common.awk -f commonDoc.awk -f commonOOCallback.awk -v 'GENERATED_SOURCE_DIR=/tmp/build/AI/Wrappers/Cpp/src-generated'
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

	myNameSpace = "springai";
	myClass = "AICallback";
	myClassVar = "clb";
	myWrapClass = "SSkirmishAICallback";
	myWrapVar = "innerCallback";
	myBridgePrefix = "bridged__";
	myAbstractAIClass = "AbstractAI";
	myAIFactoryClass = "AIFactory";

	myFixedClasses["AIException"] = 1;
	myFixedClasses["AIEvent"] = 1;
	myFixedClasses["AIFloat3"] = 1;
	myFixedClasses["AIColor"] = 1;
	myFixedClasses["AIException"] = 1;
	myFixedClasses["CallbackAIException"] = 1;
	myFixedClasses["EventAIException"] = 1;

	MAX_IDS = 1024;

	#myBufferedClasses["_UnitDef"] = 1;
	#myBufferedClasses["_WeaponDef"] = 1;
	#myBufferedClasses["_FeatureDef"] = 1;
}


# Used by the common OO AWK script
function doWrappOO(funcFullName_dw, params_dw, metaComment_dw) {

	doWrapp_dw = 1;

	#doWrapp_dw = doWrapp_dw && !match(funcFullName_dw, /Lua_callRules/);

	return doWrapp_dw;
}


function printHeader(outFile_h, namespace_h, className_h,
		isAbstract_h, implementsClass_h, isH_h) {

	clsModifiers = "";
	implementedInterfacePart = "";
	#if (isAbstract_h != 0) {
	#	clsModifiers = "abstract ";
	#} else
	if (implementsClass_h != 0) {
		implementedInterfacePart = " : public " implementsClass_h;
	}

	printCommentsHeader(outFile_h);
	if (isH_h) {
		print("") >> outFile_h;
		hg_h = createHeaderGuard(className_h);
		print("#ifndef " hg_h) >> outFile_h;
		print("#define " hg_h) >> outFile_h;
		print("") >> outFile_h;
		print("#include <climits> // for INT_MAX (required by unit-command wrapping functions)") >> outFile_h;
		print("") >> outFile_h;
		print("#include \"IncludesHeaders.h\"") >> outFile_h;
		if (implementsClass_h != 0) {
			print("#include \"" implementsClass_h ".h\"") >> outFile_h;
		}
		print("") >> outFile_h;
		if (className_h == "Wrapp" myRootClass) {
			print("struct " myWrapClass ";") >> outFile_h;
			print("") >> outFile_h;
		}
		print("namespace " myNameSpace " {") >> outFile_h;
		print("") >> outFile_h;
		print("/**") >> outFile_h;
		print(" * Lets C++ Skirmish AIs call back to the Spring engine.") >> outFile_h;
		print(" *") >> outFile_h;
		print(" * @author	AWK wrapper script") >> outFile_h;
		print(" * @version	GENERATED") >> outFile_h;
		print(" */") >> outFile_h;
		print(clsModifiers "class " className_h implementedInterfacePart " {") >> outFile_h;
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
		if (className_h == "Wrapp" myRootClass) {
			print("struct " myWrapClass ";") >> outFile_h;
			print("") >> outFile_h;
		}
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

	return GENERATED_SOURCE_DIR "/"  clsName_cfn suffix_cfn;
}

function createHeaderFileName(sourceFile_chfn) {

	headerFile_chfn = sourceFile_chfn;

	sub(/\.cpp$/, ".h", headerFile_chfn);

	return headerFile_chfn;
}
function createSourceFileName(headerFile_chfn) {

	sourceFile_chfn = headerFile_chfn;

	sub(/\.h$/, ".cpp", sourceFile_chfn);

	return sourceFile_chfn;
}



function printIncludesHeaders() {

	outH_h_inc = createFileName("IncludesHeaders", 1);
	outS_h_inc = createFileName("IncludesSources", 1);
	{
		printCommentsHeader(outH_h_inc);

		print("") >> outH_h_inc;
		print("#include <cstdio> // for NULL") >> outH_h_inc;
		print("#include <climits> // for INT_MAX") >> outH_h_inc;
		print("#include <stdexcept> // for runtime_error") >> outH_h_inc;
		print("#include <string>") >> outH_h_inc;
		print("#include <vector>") >> outH_h_inc;
		print("#include <map>") >> outH_h_inc;
		print("") >> outH_h_inc;
		for (_fc in myFixedClasses) {
			# Treat them all as value-types
			print("#include \"" _fc ".h\"") >> outH_h_inc;
		}
		print("") >> outH_h_inc;
		print("namespace " myNameSpace " {") >> outH_h_inc;
		#print("class " myClass ";") >> outH_h_inc;
		for (clsName in class_ancestors) {
			if (clsName != "Clb") {
				#print("class " clsName ";") >> outH_h_inc;
			}
		}
		#for (clsName in interfaces) {
		#	print("class " clsName ";") >> outH_h_inc;
		#}
		#for (_fc in myFixedClasses) {
		#	print("class " _fc ";") >> outH_h_inc;
		#}
		c_size_pih = cls_id_name["*"];
		for (c=0; c < c_size_pih; c++) {
			cls_pih      = cls_id_name[c];
			print("class " cls_pih ";") >> outH_h_inc;
		}
		print("} // namespace " myNameSpace) >> outH_h_inc;
		print("") >> outH_h_inc;
	}
	{
		printCommentsHeader(outS_h_inc);

		print("") >> outS_h_inc;
		#print("#include \"" myClass ".h\"") >> outS_h_inc;
		for (clsName in class_ancestors) {
			if (clsName != "Clb") {
				#print("#include \"" clsName ".h\"") >> outS_h_inc;
			}
		}
		for (clsId in implClsNames) {
			#print("#include \"" implClsNames[clsId] ".h\"") >> outS_h_inc;
		}
		for (clsName in interfaces) {
			#print("#include \"" clsName ".h\"") >> outS_h_inc;
		}
		for (_fc in myFixedClasses) {
			print("#include \"" _fc ".h\"") >> outS_h_inc;
		}
		c_size_pih = cls_id_name["*"];
		for (c=0; c < c_size_pih; c++) {
			cls_pih      = cls_id_name[c];
			print("#include \"" cls_pih ".h\"") >> outS_h_inc;
		}
		for (implId in cls_implId_fullClsName) {
			cls_pih      = cls_implId_fullClsName[implId];
			print("#include \"Wrapp" cls_pih ".h\"") >> outS_h_inc;
		}
		print("") >> outS_h_inc;
		print("#include \"CombinedCallbackBridge.h\"") >> outS_h_inc;
		print("") >> outS_h_inc;
	}
}



function getNullTypeValue(fRet_ntv) {

	if (fRet_ntv == "void") {
		return "";
	} else if (fRet_ntv == "std::string") {
		return "\"\"";
	} else if (fRet_ntv == myNameSpace "::AIFloat3") {
		return myNameSpace "::AIFloat3::NULL_VALUE";
	} else if (fRet_ntv == myNameSpace "::AIColor") {
		return myNameSpace "::AIColor::NULL_VALUE";
	} else if (startsWithCapital(fRet_ntv)) {
		# must be a class
		return "NULL";
	} else if (match(fRet_ntv, /^std::vector</)) {
		return fRet_ntv "()";
	} else if (match(fRet_ntv, /^std::map</)) {
		return fRet_ntv "()";
	} else if (fRet_ntv == "bool") {
		return "false";
	} else {
		return "0";
	}
}
function printTripleFunc(fRet_tr, fName_tr, fParams_tr, thrownExceptions_tr, outFile_int_h_tr, outFile_stb_h_tr, outFile_wrp_h_tr, printIntAndStb_tr, noOverride_tr, const_tr, clsName_stb_tr, clsName_wrp_tr) {

	_constStr_tr = "";
	if (const_tr) {
		_constStr_tr = " const";
	}

	if (fName_tr == "Log") {
		# as the class name is "Log" too, and we can not have a function
		# with the same name as the class/constructor, we have to rename.
		fName_tr = "DoLog";
	}

	fRet_noPointer_tr = fRet_tr;
	sub(/\*$/, "", fRet_noPointer_tr);
	if (fRet_noPointer_tr in cls_name_id) {
		fRet_tr = myNameSpace "::" fRet_tr;
	}

	_funcHdr_tr = fName_tr "(" fParams_tr ")" _constStr_tr;
	_funcHdr_h_tr = "virtual " fRet_tr " " _funcHdr_tr;
	if (thrownExceptions_tr != "") {
		#_funcHdr_tr = _funcHdr_tr " throws " thrownExceptions_tr;
	}

	if (printIntAndStb_tr) {
		_funcHdr_int_h_tr = _funcHdr_h_tr;
		# add default values for unit-command wrapping functions
		sub(/short options, int timeOut/, "short options = 0, int timeOut = INT_MAX", _funcHdr_int_h_tr);
		print("public:") >> outFile_int_h_tr;
		print("\t" _funcHdr_int_h_tr " = 0;") >> outFile_int_h_tr;
		print("") >> outFile_int_h_tr;

		outFile_stb_cpp_tr = createSourceFileName(outFile_stb_h_tr);
		isSimpleGetter_tr = ((fRet_tr != "void") && (fParams_tr == "") && match(fName_tr, /^(Get|Is)/));
		if ((fName_tr == "IsEnabled") && match(outFile_int_h_tr, /Cheats\.h$/)) {
			# an exception, because this has a setter anyway
			isSimpleGetter_tr = 0;
		}
		nullTypeValue_tr = getNullTypeValue(fRet_tr);
		clsPrefix_stb_tr = myNameSpace "::" clsName_stb_tr "::";
		if (isSimpleGetter_tr) {
			# create an additional private member and setter
			propName_tr = fName_tr;
			sub(/^Get/, "", propName_tr);
			propName_tr = lowerize(propName_tr);
			fSetterName_tr = fName_tr;
			sub(/^(Get|Is)/, "Set", fSetterName_tr);
			print("private:") >> outFile_stb_h_tr;
			print("\t" fRet_tr " " propName_tr ";/* = "  nullTypeValue_tr "*/; // TODO: FIXME: put this into a constructor") >> outFile_stb_h_tr;
			print("public:") >> outFile_stb_h_tr;

			print("\t" "virtual " "void "                  fSetterName_tr "(" fRet_tr " " propName_tr ")" ";")  >> outFile_stb_h_tr;
			print("\t"            "void " clsPrefix_stb_tr fSetterName_tr "(" fRet_tr " " propName_tr ")" " {") >> outFile_stb_cpp_tr;
			print("\t\t" "this->" propName_tr " = " propName_tr ";") >> outFile_stb_cpp_tr;
			print("\t" "}") >> outFile_stb_cpp_tr;
			print("\t" "// @Override") >> outFile_stb_h_tr;
			print("\t"                              _funcHdr_h_tr ";")  >> outFile_stb_h_tr;
			print("\t" fRet_tr " " clsPrefix_stb_tr _funcHdr_tr " {") >> outFile_stb_cpp_tr;
			print("\t\t" "return " propName_tr ";") >> outFile_stb_cpp_tr;
		} else {
			print("\t" "// @Override") >> outFile_stb_h_tr;
			print("\t"                              _funcHdr_h_tr ";")  >> outFile_stb_h_tr;
			print("\t" fRet_tr " " clsPrefix_stb_tr _funcHdr_tr " {") >> outFile_stb_cpp_tr;
			# simply return a NULL like value
			if (fRet_tr == "void") {
				# return nothing
			} else {
				print("\t\t" "return " nullTypeValue_tr ";") >> outFile_stb_cpp_tr;
			}
		}
		print("\t" "}") >> outFile_stb_cpp_tr;
		print("") >> outFile_stb_cpp_tr;
	}

	outFile_wrp_cpp_tr = createSourceFileName(outFile_wrp_h_tr);
	print("public:") >> outFile_wrp_h_tr;
	if (!noOverride_tr) {
		print("\t" "// @Override") >> outFile_wrp_h_tr;
	}
	clsPrefix_wrp_tr = myNameSpace "::" clsName_wrp_tr "::";
	print("\t"                              _funcHdr_h_tr ";")  >> outFile_wrp_h_tr;
	print("\t" fRet_tr " " clsPrefix_wrp_tr _funcHdr_tr " {") >> outFile_wrp_cpp_tr;
	print("") >> outFile_wrp_cpp_tr;
}


function printClasses() {

	# look for AVAILABLE indicators
	for (_memberId in cls_memberId_metaComment) {
		if (match(cls_memberId_metaComment[_memberId], /AVAILABLE:/)) {
			_availCls = cls_memberId_metaComment[_memberId];
			sub(/^.*AVAILABLE:/, "", _availCls);
			sub(/[ \t].*$/,      "", _availCls);
			clsAvailInd_memberId_cls[_memberId] = _availCls;
		}
	}

	c_size_cs = cls_id_name["*"];
#print("c_size_cs: " c_size_cs);
	for (c=0; c < c_size_cs; c++) {
		cls_cs      = cls_id_name[c];
		anc_size_cs = cls_name_implIds[cls_cs ",*"];

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
	clsName_wrp_c = "Wrapp" _fullClsName;
	isClbRootCls_c = (clsName_int_c == myRootClass);

	if (printIntAndStb_c) {
		outFile_int_h_c   = createFileName(clsName_int_c, 1);
		outFile_abs_h_c   = createFileName(clsName_abs_c, 1);
		outFile_abs_cpp_c = createFileName(clsName_abs_c, 0);
		outFile_stb_h_c   = createFileName(clsName_stb_c, 1);
		outFile_stb_cpp_c = createFileName(clsName_stb_c, 0);
	}
	outFile_wrp_h_c       = createFileName(clsName_wrp_c, 1);
	outFile_wrp_cpp_c     = createFileName(clsName_wrp_c, 0);

	if (printIntAndStb_c) {
		printHeader(outFile_int_h_c,   myPkgA, clsName_int_c, 1, 0,             1);
		printHeader(outFile_abs_h_c,   myPkgA, clsName_abs_c, 1, clsName_int_c, 1);
		printHeader(outFile_abs_cpp_c, myPkgA, clsName_abs_c, 1, clsName_int_c, 0);
		printHeader(outFile_stb_h_c,   myPkgA, clsName_stb_c, 0, clsName_int_c, 1);
		printHeader(outFile_stb_cpp_c, myPkgA, clsName_stb_c, 0, clsName_int_c, 0);
	}
	printHeader(    outFile_wrp_h_c,   myPkgA, clsName_wrp_c, 0, clsName_int_c, 1);
	printHeader(    outFile_wrp_cpp_c, myPkgA, clsName_wrp_c, 0, clsName_int_c, 0);

	# prepare additional indices names
	addInds_size_c = split(cls_implId_indicesArgs[implId_c], addInds_c, ",");
	for (ai=1; ai <= addInds_size_c; ai++) {
		sub(/int /, "", addInds_c[ai]);
		addInds_c[ai] = trim(addInds_c[ai]);
	}


	# print private vars
	print("private:") >> outFile_wrp_h_c;
	if (isClbRootCls_c) {
		print("\t" "const struct " myWrapClass "* " myWrapVar ";") >> outFile_wrp_h_c;
	}
	# print additionalVars
	for (ai=1; ai <= addInds_size_c; ai++) {
		print("\t" "int " addInds_c[ai] ";") >> outFile_wrp_h_c;
	}
	print("") >> outFile_wrp_h_c;


	# assemble constructor params
	ctorParams   = "";
	if (isClbRootCls_c) {
		ctorParams   = ctorParams ", const struct " myWrapClass "* " myWrapVar;
	}
	addIndPars_c = "";
	for (ai=1; ai <= addInds_size_c; ai++) {
		addIndPars_c = addIndPars_c ", int " addInds_c[ai];
	}
	ctorParams   = ctorParams addIndPars_c;
	sub(/^, /, "", ctorParams);

	ctorParamsNoTypes   = removeParamTypes(ctorParams);
	sub(/^, /, "", addIndPars_c);
	addIndParsNoTypes_c = removeParamTypes(addIndPars_c);
	condAddIndPars_c    = (addIndPars_c == "") ? "" : ", ";

	# print constructor
	print("\t"                                     clsName_wrp_c "(" ctorParams ")" ";")  >> outFile_wrp_h_c;
	print("\t" myNameSpace "::" clsName_wrp_c "::" clsName_wrp_c "(" ctorParams ")" " {") >> outFile_wrp_cpp_c;
	print("") >> outFile_wrp_cpp_c;
	# init additionalVars
	for (ai=1; ai <= addInds_size_c; ai++) {
		addIndName = addInds_c[ai];
		print("\t\t" "this->" addIndName " = " addIndName ";") >> outFile_wrp_cpp_c;
	}
	if (isClbRootCls_c) {
		print("\t\t" "funcPntBrdg_addCallback(skirmishAIId, " myWrapVar ");") >> outFile_wrp_cpp_c;
	}
	print("\t" "}") >> outFile_wrp_cpp_c;
	print("") >> outFile_wrp_cpp_c;

	# print destructor
	print("\t" "virtual "                          "~" clsName_wrp_c "()" ";")  >> outFile_wrp_h_c;
	print("\t" myNameSpace "::" clsName_wrp_c "::" "~" clsName_wrp_c "()" " {") >> outFile_wrp_cpp_c;
	print("") >> outFile_wrp_cpp_c;
	if (isClbRootCls_c) {
		print("\t\t" "funcPntBrdg_removeCallback(skirmishAIId);") >> outFile_wrp_cpp_c;
	}
	print("\t" "}") >> outFile_wrp_cpp_c;
	print("") >> outFile_wrp_cpp_c;
	if (printIntAndStb_c) {
		print("public:") >> outFile_int_h_c;
		print("\t" "virtual "                          "~" clsName_int_c "()" "{}")  >> outFile_int_h_c;
		print("protected:") >> outFile_abs_h_c;
		print("\t" "virtual "                          "~" clsName_abs_c "()" ";")   >> outFile_abs_h_c;
		print("\t" myNameSpace "::" clsName_abs_c "::" "~" clsName_abs_c "()" " {}") >> outFile_abs_cpp_c;
		print("protected:") >> outFile_stb_h_c;
		print("\t" "virtual "                          "~" clsName_stb_c "()" ";")   >> outFile_stb_h_c;
		print("\t" myNameSpace "::" clsName_stb_c "::" "~" clsName_stb_c "()" " {}") >> outFile_stb_cpp_c;
	}


	# print additional vars fetchers
	print("public:") >> outFile_wrp_h_c;
	for (ai=1; ai <= addInds_size_c; ai++) {
		addIndName = addInds_c[ai];
		_fRet    = "int";
		_fName   = "Get" capitalize(addIndName);
		_fParams = "";
		_fExceps = "";

		printIntAndStb_tmp_c = printIntAndStb_c;
		_noOverride = 0;
		if ((noInterfaceIndices_c != 0) && (addIndName == noInterfaceIndices_c)) {
			printIntAndStb_tmp_c = 0;
			_noOverride = 1;
		}
		printTripleFunc(_fRet, _fName, _fParams, _fExceps, outFile_int_h_c, outFile_stb_h_c, outFile_wrp_h_c, printIntAndStb_tmp_c, _noOverride, 1, clsName_stb_c, clsName_wrp_c);

		print("\t\t" "return " addIndName ";") >> outFile_wrp_cpp_c;
		print("\t" "}") >> outFile_wrp_cpp_c;
		print("") >> outFile_wrp_cpp_c;
	}

	# print static instance fetcher method
	{
		clsIsBuffered_c    = isBufferedClass(clsName_c);
		_isAvailableMethod = "";
		for (_memId in clsAvailInd_memberId_cls) {
			if (clsAvailInd_memberId_cls[_memId] == clsName_c) {
				_isAvailableMethod = _memId;
				gsub(/,/, "_", _isAvailableMethod);
			}
		}

		if (clsIsBuffered_c) {
			print("private:") >> outFile_wrp_h_c;
			print("\t" "static std::map<int," clsName_c "*> _buffer_instances;") >> outFile_wrp_h_c;
			print("") >> outFile_wrp_h_c;
		}
		print("public:") >> outFile_wrp_h_c;
		print("\t" "static "                        clsName_c "* "                                  "GetInstance(" ctorParams ")" ";")  >> outFile_wrp_h_c;
		print("\t" myNameSpace"::"clsName_wrp_c"::" clsName_c "* " myNameSpace"::"clsName_wrp_c"::" "GetInstance(" ctorParams ")" " {") >> outFile_wrp_cpp_c;
		print("") >> outFile_wrp_cpp_c;
		lastParamName = ctorParamsNoTypes;
		sub(/^.*,[ \t]*/, "", lastParamName);
		if (match(lastParamName, /^[^ \t]+Id$/)) {
			if (clsName_c == "Unit") {
				# the first valid unit ID is 1
				print("\t\t" "if (" lastParamName " <= 0) {") >> outFile_wrp_cpp_c;
			} else {
				# ... for all other IDs, the first valid one is 0
				print("\t\t" "if (" lastParamName " < 0) {") >> outFile_wrp_cpp_c;
			}
			print("\t\t\t" "return NULL;") >> outFile_wrp_cpp_c;
			print("\t\t" "}") >> outFile_wrp_cpp_c;
			print("") >> outFile_wrp_cpp_c;
		}
		print("\t\t" myNameSpace"::"clsName_c "* _ret = NULL;") >> outFile_wrp_cpp_c;
		if (_isAvailableMethod == "") {
			print("\t\t" "_ret = new " myNameSpace"::"clsName_wrp_c "(" ctorParamsNoTypes ");") >> outFile_wrp_cpp_c;
		} else {
			print("\t\t" "bool isAvailable = " myBridgePrefix _isAvailableMethod "(" addIndParsNoTypes_c ");") >> outFile_wrp_cpp_c;
			print("\t\t" "if (isAvailable) {") >> outFile_wrp_cpp_c;
			print("\t\t\t" "_ret = new " myNameSpace"::"clsName_wrp_c "(" ctorParamsNoTypes ");") >> outFile_wrp_cpp_c;
			print("\t\t" "}") >> outFile_wrp_cpp_c;
		}
		if (clsIsBuffered_c) {
			if (_isAvailableMethod == "") {
				print("\t\t" "{") >> outFile_wrp_cpp_c;
			} else {
				print("\t\t" "if (_ret != NULL) {") >> outFile_wrp_cpp_c;
			}
			print("\t\t\t" "const int indexHash = _ret.HashCode();") >> outFile_wrp_cpp_c;
			print("\t\t\t" "if (_buffer_instances.containsKey(indexHash)) {") >> outFile_wrp_cpp_c;
			print("\t\t\t\t" "_ret = _buffer_instances.get(indexHash);") >> outFile_wrp_cpp_c;
			print("\t\t\t" "} else {") >> outFile_wrp_cpp_c;
			print("\t\t\t\t" "_buffer_instances.put(indexHash, _ret);") >> outFile_wrp_cpp_c;
			print("\t\t\t" "}") >> outFile_wrp_cpp_c;
			print("\t\t" "}") >> outFile_wrp_cpp_c;
		}
		print("\t\t" "return _ret;") >> outFile_wrp_cpp_c;
		print("\t" "}") >> outFile_wrp_cpp_c;
		print("") >> outFile_wrp_cpp_c;
	}


	if (printIntAndStb_c) {
		# print CompareTo(other) method
		{
			cai_c = 0;
			for (ai=1; ai <= addInds_size_c; ai++) {
				addIndName = addInds_c[ai];
				if ((noInterfaceIndices_c == 0) || (addIndName != noInterfaceIndices_c)) {
					compareAddInds_c[cai_c] = addIndName;
					cai_c++;
				}
			}

			print("\t" "// @Override") >> outFile_abs_h_c;
			print("public:") >> outFile_abs_h_c;
			print("\t" "virtual " "int "                                  "CompareTo(const " clsName_c "& other)" ";") >> outFile_abs_h_c;
			print("\t"            "int " myNameSpace"::"clsName_abs_c"::" "CompareTo(const " clsName_c "& other)" " {") >> outFile_abs_cpp_c;
			print("\t\t" "static const int EQUAL  =  0;") >> outFile_abs_cpp_c;
			if (cai_c > 0) {
				print("\t\t" "static const int BEFORE = -1;") >> outFile_abs_cpp_c;
				print("\t\t" "static const int AFTER  =  1;") >> outFile_abs_cpp_c;
			}
			print("") >> outFile_abs_cpp_c;
			print("\t\t" "if ((" clsName_c "*)this == &other) return EQUAL;") >> outFile_abs_cpp_c;
			print("") >> outFile_abs_cpp_c;

			#if (isClbRootCls_c) {
			#	print("\t\t" "if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;") >> outFile_abs_cpp_c;
			#	print("\t\t" "if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;") >> outFile_abs_cpp_c;
			#	print("\t\t" "return EQUAL;") >> outFile_abs_cpp_c;
			#} else {
				for (ai=0; ai < cai_c; ai++) {
					addIndName = capitalize(compareAddInds_c[ai]);
					print("\t\t" "if (this->Get" addIndName "() < other.Get" addIndName "()) return BEFORE;") >> outFile_abs_cpp_c;
					print("\t\t" "if (this->Get" addIndName "() > other.Get" addIndName "()) return AFTER;")  >> outFile_abs_cpp_c;
				}
				print("\t\t" "return EQUAL;") >> outFile_abs_cpp_c;
			#}
			print("\t" "}") >> outFile_abs_cpp_c;
			print("") >> outFile_abs_cpp_c;
		}


		# print Equals(other) method
		#if (!isClbRootCls_c)
		{
			print("\t" "// @Override") >> outFile_abs_h_c;
			print("public:") >> outFile_abs_h_c;
			print("\t" "virtual " "bool "                                  "Equals(const " clsName_c "& other)" ";")  >> outFile_abs_h_c;
			print(                "bool " myNameSpace"::"clsName_abs_c"::" "Equals(const " clsName_c "& other)" " {") >> outFile_abs_cpp_c;
			print("") >> outFile_abs_cpp_c;

			#if (isClbRootCls_c) {
			#	print("\t" "if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;") >> outFile_abs_cpp_c;
			#	print("\t" "return true;") >> outFile_abs_cpp_c;
			#} else {
				for (ai=1; ai <= addInds_size_c; ai++) {
					addIndName = addInds_c[ai];
					if ((noInterfaceIndices_c == 0) || (addIndName != noInterfaceIndices_c)) {
						print("\t" "if (this->Get" capitalize(addIndName) "() != other.Get" capitalize(addIndName) "()) return false;") >> outFile_abs_cpp_c;
					}
				}
				print("\t" "return true;") >> outFile_abs_cpp_c;
			#}
			print("}") >> outFile_abs_cpp_c;
			print("") >> outFile_abs_cpp_c;
		}


		# print HashCode() method
		#if (!isClbRootCls_c)
		{
			print("\t" "// @Override") >> outFile_abs_h_c;
			print("public:") >> outFile_abs_h_c;
			print("\t" "virtual " "int "                                  "HashCode()" ";")  >> outFile_abs_h_c;
			print(                "int " myNameSpace"::"clsName_abs_c"::" "HashCode()" " {") >> outFile_abs_cpp_c;
			print("") >> outFile_abs_cpp_c;

			#if (isClbRootCls_c) {
			#	print("\t" "int _res = 0;") >> outFile_abs_cpp_c;
			#	print("") >> outFile_abs_cpp_c;
			#	print("\t" "_res += this->GetSkirmishAIId() * 10E8;") >> outFile_abs_cpp_c;
			#} else {
				print("\t" "int _res = 23;") >> outFile_abs_cpp_c;
				print("") >> outFile_abs_cpp_c;
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
						print("\t" "_res += this->Get" capitalize(addIndName) "() * (int) (10E" (7-ai) ");") >> outFile_abs_cpp_c;
					}
				}
			#}
			print("") >> outFile_abs_cpp_c;
			print("\t" "return _res;") >> outFile_abs_cpp_c;
			print("}") >> outFile_abs_cpp_c;
			print("") >> outFile_abs_cpp_c;
		}


		# print ToString() method
		{
			print("\t" "// @Override") >> outFile_abs_h_c;
			print("public:") >> outFile_abs_h_c;
			print("\t" "virtual " "std::string "                                  "ToString()" ";") >> outFile_abs_h_c;
			print(                "std::string " myNameSpace"::"clsName_abs_c"::" "ToString()" " {") >> outFile_abs_cpp_c;
			print("") >> outFile_abs_cpp_c;
			print("\t" "std::string _res = \"" clsName_c "\";") >> outFile_abs_cpp_c;
			print("") >> outFile_abs_cpp_c;

			#if (isClbRootCls_c) { # NO FOLD
			#	print("\t\t" "_res = _res + \"(skirmishAIId=\" + this->skirmishAIId + \", \";") >> outFile_abs_cpp_c;
			#} else { # NO FOLD
			#	print("\t\t" "_res = _res + \"(clbHash=\" + " myBridgePrefix "hashCode() + \", \";") >> outFile_abs_cpp_c;
			#	print("\t\t" "_res = _res + \"skirmishAIId=\" + " myBridgePrefix "SkirmishAI_getSkirmishAIId() + \", \";") >> outFile_abs_cpp_c;
				if (addInds_size_c > 0) {
					print("\t" "char _buff[64];") >> outFile_abs_cpp_c;
				}
				for (ai=1; ai <= addInds_size_c; ai++) {
					addIndName = addInds_c[ai];
					if ((noInterfaceIndices_c == 0) || (addIndName != noInterfaceIndices_c)) {
						print("\t" "sprintf(_buff, \"" addIndName "=%i, \", this->Get" capitalize(addIndName) "());") >> outFile_abs_cpp_c;
						print("\t" "_res = _res + _buff;") >> outFile_abs_cpp_c;
					}
				}
			#} # NO FOLD
			print("\t" "_res = _res + \")\";") >> outFile_abs_cpp_c;
			print("") >> outFile_abs_cpp_c;
			print("\t" "return _res;") >> outFile_abs_cpp_c;
			print("}") >> outFile_abs_cpp_c;
			print("") >> outFile_abs_cpp_c;
		}
	}

	# make these available in called functions
	implId_c_         = implId_c;
	clsName_c_        = clsName_c;
	printIntAndStb_c_ = printIntAndStb_c;

	# print member functions
	members_size = cls_name_members[clsName_int_c ",*"];
	for (m=0; m < members_size; m++) {
		memName_c  = cls_name_members[clsName_int_c "," m];
		fullName_c = implId_c "," memName_c;
		gsub(/,/, "_", fullName_c);
		if (doWrappMember(fullName_c)) {
			printMember(fullName_c, memName_c, addInds_size_c);
		} else {
			print("JavaOO: NOTE: intentionally not wrapped: " fullName_c);
		}
	}


	# finnish up
	if (printIntAndStb_c) {
		print("}; // class " clsName_int_c) >> outFile_int_h_c;
		print("") >> outFile_int_h_c;
		print("}  // namespace " myNameSpace) >> outFile_int_h_c;
		print("") >> outFile_int_h_c;
		print("#endif // " createHeaderGuard(clsName_int_c)) >> outFile_int_h_c;
		print("") >> outFile_int_h_c;
		close(outFile_int_h_c);

		print("}; // class " clsName_abs_c) >> outFile_abs_h_c;
		print("") >> outFile_abs_h_c;
		print("}  // namespace " myNameSpace) >> outFile_abs_h_c;
		print("") >> outFile_abs_h_c;
		print("#endif // " createHeaderGuard(clsName_abs_c)) >> outFile_abs_h_c;
		print("") >> outFile_abs_h_c;
		close(outFile_abs_h_c);
		close(outFile_abs_cpp_c);

		print("}; // class " clsName_stb_c) >> outFile_stb_h_c;
		print("") >> outFile_stb_h_c;
		print("}  // namespace " myNameSpace) >> outFile_stb_h_c;
		print("") >> outFile_stb_h_c;
		print("#endif // " createHeaderGuard(clsName_stb_c)) >> outFile_stb_h_c;
		print("") >> outFile_stb_h_c;
		close(outFile_stb_h_c);
		close(outFile_stb_cpp_c);
	}
	print("}; // class " clsName_wrp_c) >> outFile_wrp_h_c;
	print("") >> outFile_wrp_h_c;
	print("}  // namespace " myNameSpace) >> outFile_wrp_h_c;
	print("") >> outFile_wrp_h_c;
	print("#endif // " createHeaderGuard(clsName_wrp_c)) >> outFile_wrp_h_c;
	print("") >> outFile_wrp_h_c;
	close(outFile_wrp_h_c);
	close(outFile_wrp_cpp_c);
}


function isRetParamName(paramName_rp) {
	return (match(paramName_rp, /_out(_|$)/) || match(paramName_rp, /(^|_)?ret_/));
}


function printMember(fullName_m, memName_m, additionalIndices_m) {

	# use some vars from the printClass function (which called us)
	implId_m          = implId_c_;
	clsName_m         = clsName_c_;
	printIntAndStb_m  = printIntAndStb_c_;
	implCls_m         = implCls_c;
	clsName_int_m     = clsName_int_c;
	clsName_stb_m     = clsName_stb_c;
	clsName_wrp_m     = clsName_wrp_c;
	isClbRootCls_m    = isClbRootCls_c;
	outFile_int_h_m   = outFile_int_h_c;
	outFile_stb_h_m   = outFile_stb_h_c;
	outFile_wrp_h_m   = outFile_wrp_h_c;
	outFile_stb_cpp_m = outFile_stb_cpp_c;
	outFile_wrp_cpp_m = outFile_wrp_cpp_c;
	addInds_size_m    = addInds_size_c;
	for (ai=1; ai <= addInds_size_m; ai++) {
		addInds_m[ai] = addInds_c[ai];
	}

	indent_m            = "\t";
	memId_m             = clsName_m "," memName_m;
	retType             = cls_memberId_retType[memId_m]; # this may be changed
	retType_int         = retType;     
	params              = cls_memberId_params[memId_m];                  # this is a const var
	#if (isClbRootCls_m) {
	#	params = "int skirmishAIId, " params;
	#	sub(/, $/, "", params);
	#}
	isFetcher           = cls_memberId_isFetcher[memId_m];
	metaComment         = cls_memberId_metaComment[memId_m];
	memName             = fullName_m;
	sub(/^.*_/, "", memName);
	memName = capitalize(memName);
#print("memName: " memName);
	functionName_m      = fullName_m;
	sub(/^[^_]+_/, "", functionName_m);

	if (memId_m in clsAvailInd_memberId_cls) {
		return;
	}

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

	# remove additional indices from the outter params
	for (ai=1; ai <= addInds_size_m; ai++) {
		_removed = sub(/[^,]+(, )?/, "", params);
		if (!_removed && !part_isStatic(memName_m, metaComment)) {
			addIndName = addInds_m[ai];
			print("ERROR: failed removing additional indices " addIndName " from method " memName_m " in class " clsName_int_m);
			exit(1);
		}
	}

	innerParams         = removeParamTypes(params);

	# add additional indices fetcher calls to inner params
	addInnerParams = "";
	addInds_real_size_m = addInds_size_m;
	if (part_isStatic(memName_m, metaComment)) {
		addInds_real_size_m--;
	}
	for (ai=1; ai <= addInds_real_size_m; ai++) {
		addIndName = addInds_m[ai];
		_condComma = "";
		if (addInnerParams != "") {
			_condComma = ", ";
		}
		addInnerParams = addInnerParams _condComma "this->Get" capitalize(addIndName) "()";
	}
	_condComma = "";
	if ((addInnerParams != "") && (innerParams != "")) {
		_condComma = ", ";
	}
	innerParams = addInnerParams _condComma innerParams;


	# convert param types
	paramNames_size = split(innerParams, paramNames, ", ");
	for (prm = 1; prm <= paramNames_size; prm++) {
		paNa = paramNames[prm];
		if (!isRetParamName(paNa)) {
			if (match(paNa, /_posF3/)) {
				# convert float[3] to AIFloat3
				paNaNew = paNa;
				sub(/_posF3/, "", paNaNew);
				sub("float\\* " paNa, "const " myNameSpace "::AIFloat3\\& " paNaNew, params);
				conversionCode_pre = conversionCode_pre "\t\t" "float " paNa "[3];" "\n";
				conversionCode_pre = conversionCode_pre "\t\t" paNaNew ".LoadInto(" paNa ");" "\n";
			} else if (match(paNa, /_colorS3/)) {
				# convert short[3] to springai::AIColor
				paNaNew = paNa;
				sub(/_colorS3/, "", paNaNew);
				sub("short\\* " paNa, "const " myNameSpace "::AIColor\\& " paNaNew, params);
				conversionCode_pre = conversionCode_pre "\t\t" "short " paNa "[3];" "\n";
				conversionCode_pre = conversionCode_pre "\t\t" paNaNew ".LoadInto3(" paNa ");" "\n";
			}
		}
	}

	# convert an error return int value to an Exception
	# "error-return:0=OK"
	if (part_isErrorReturn(metaComment) && retType == "int") {
		errorRetValueOk_m = part_getErrorReturnValueOk(metaComment);

		conversionCode_post = conversionCode_post "\t" "if (" retVar_out_m " != " errorRetValueOk_m ") {" "\n";
		conversionCode_post = conversionCode_post "\t\t" "throw CallbackAIException(\"" memName_m "\", " retVar_out_m ");" "\n";
		conversionCode_post = conversionCode_post "\t" "}" "\n";
		thrownExceptions = thrownExceptions ", CallbackAIException" thrownExceptions;

		retType = "void";
	}

	# convert out params to return values
	paramTypeNames_size = split(params, paramTypeNames, ", ");
	hasRetParam = 0;
	for (prm = 1; prm <= paramTypeNames_size; prm++) {
		paNa = extractParamName(paramTypeNames[prm]);
		paTy = extractParamType(paramTypeNames[prm]);
		if (isRetParamName(paNa)) {
			if (retType == "void") {
				paTy = extractParamType(paramTypeNames[prm]);
				hasRetParam = 1;
				if (match(paNa, /_posF3/)) {
					# convert float[3] to AIFloat3
					retParamType = myNameSpace "::AIFloat3";
					retVar_out_m = "_ret";
					conversionCode_pre  = conversionCode_pre  "\t\t" "float " paNa "[3];" "\n";
					#conversionCode_post = conversionCode_post "\t\t" retVar_out_m " = new " myNameSpace "::AIFloat3(" paNa "[0], " paNa "[1]," paNa "[2]);" "\n";
					#declaredVarsCode = "\t\t" retParamType " " retVar_out_m ";" "\n" declaredVarsCode;
					conversionCode_post = conversionCode_post "\t\t" retParamType " " retVar_out_m "(" paNa "[0], " paNa "[1], " paNa "[2]);" "\n";
					sub("(, )?float\\* " paNa, "", params);
					retType = retParamType;
				} else if (match(paNa, /_colorS3/)) {
					retParamType = myNameSpace "::AIColor";
					retVar_out_m = "_ret";
					conversionCode_pre  = conversionCode_pre  "\t\t" "short " paNa "[3];" "\n";
					#conversionCode_post = conversionCode_post "\t\t" retVar_out_m " = new " myNameSpace "::AIColor(" paNa "[0], " paNa "[1]," paNa "[2]);" "\n";
					#declaredVarsCode = "\t\t" retParamType " " retVar_out_m ";" "\n" declaredVarsCode;
					conversionCode_post = conversionCode_post "\t\t" retParamType " " retVar_out_m "((unsigned char) " paNa "[0], (unsigned char) " paNa "[1], (unsigned char) " paNa "[2]);" "\n";
					sub("(, )?short\\* " paNa, "", params);
					retType = retParamType;
				} else if (match(paTy, /const char\*/)) {
					retParamType = "std::string";
					retVar_out_m = "_ret";
					conversionCode_pre  = conversionCode_pre  "\t\t" "char " paNa "[10240];" "\n";
					conversionCode_post = conversionCode_post "\t\t" retParamType " " retVar_out_m "(" paNa ");" "\n";
					sub("(, )?const char\\* " paNa, "", params);
					retType = retParamType;
				} else {
					print("FAILED converting return param: " paramTypeNames[prm] " / " fullName_m);
					exit(1);
				}
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
		_isMulti  = match(_ref, /MULTI:/);
		_isReturn = match(_ref, /RETURN->/);

		if (!_isMulti && !_isReturn) {
			# convert single param reference
			_refRel = _ref;
			sub(/^.*:/, "", _refRel);
			_paNa = _refRel;           # example: resourceId
			sub(/->.*$/, "", _paNa);
			sub(/^.*->/, "", _refRel);
			_refObj = _refRel"*";         # example: Resource*
			_paNaNew = _paNa;
			if (!sub(/Id$/, "", _paNaNew)) {
				_paNaNew = "oo_" _paNaNew;
			}

			if (_refObj != "Team*" && _refObj != "FigureGroup*" && _refObj != "Path*") {
				_paNa_found = sub("int " _paNa, _refObj " " _paNaNew, params);
				# it may not be found if it is an output parameter
				if (_paNa_found) {
					conversionCode_pre = conversionCode_pre "\t\t"  "int " _paNa " = " _paNaNew "->Get" _refRel "Id();" "\n";
				}
			} else {
				print("note: ignoring meta comment: REF:" _ref);
			}
		} else if (!_isMulti && _isReturn) {
			_refObj = _ref;         # example: Resource*
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

			_retVar_out_new = retVar_out_m "_out";
			_wrappGetInst_params = "";
			if (clsName_m == myRootClass || (clsName_m == "WeaponMount" && _fullClsName == "WeaponDef")) {
				_wrappGetInst_params = "skirmishAIId"; # FIXME: HACK; do not know why needed :/ (should use parents list of params instead of self,or vice-versa?)
			}
			_hasRetInd = 0;
			_refObj = _refObj"*";
			if (retType != "void") {
				_hasRetInd = 1;
			}
			for (ai=1; ai <= (addInds_size_m-_hasRetInd); ai++) {
				# Very hacky! too unmotivated for propper fix, sorry.
				# propper fix would involve getting the parent of the wrapped
				# class and using its additional indices
				if (functionName_m != "UnitDef_WeaponMount_getWeaponDef") {
					_wrappGetInst_params = _wrappGetInst_params ", " addInds_m[ai];
				}
			}
			if (retType != "void") {
				_wrappGetInst_params = _wrappGetInst_params ", " retVar_out_m;
			} else {
				ommitMainCall = 1;
			}

			if (clsName_m == myRootClass) {
				sub(/^skirmishAIId, skirmishAIId/, "skirmishAIId", _wrappGetInst_params); # FIXME: HACK; do not know why needed :/ (should use parents list of params instead of self,or vice-versa?)
				if (_fullClsName == "Engine") {
					sub(/^skirmishAIId(, )?/, "", _wrappGetInst_params); # FIXME: HACK; do not know why needed :/ (should use parents list of params instead of self,or vice-versa?)
				}
			}
			sub(/^, /, "", _wrappGetInst_params);
			conversionCode_post = conversionCode_post "\t\t" _retVar_out_new " = " myNameSpace "::Wrapp" _fullClsName "::GetInstance(" _wrappGetInst_params ");" "\n";
			declaredVarsCode = "\t\t" _refObj " " _retVar_out_new ";" "\n" declaredVarsCode;
			retVar_out_m = _retVar_out_new;
			retType = _refObj;
		} else {
			print("WARNING: unsupported: REF:" _ref);
		}
	}


	isMap = part_isMap(fullName_m, metaComment);
	if (isMap) {

		_isFetching = 1;
		_isRetSize  = 0;
		_isObj      = 0;
		_mapVar_size      = "_size";
		_mapVar_keys      = "keys";
		_mapVar_values    = "values";
		_mapType_key      = "const char*";
		_mapType_value    = "const char*";
		_mapType_oo_key   = "std::string";
		_mapType_oo_value = "std::string";
		_mapVar_oo        = "_map";
		_mapType_int      = "std::map<"     _mapType_oo_key "," _mapType_oo_value ">";
		_mapType_impl     = "std::map<" _mapType_oo_key "," _mapType_oo_value ">"; # TODO: should be unused, but needs check

		_mapType_key_regexEscaped = regexEscape(_mapType_key);
		_mapType_value_regexEscaped = regexEscape(_mapType_value);
		sub("(, )?" _mapType_key_regexEscaped "\\* " _mapVar_keys,   "", params);
		sub("(, )?" _mapType_value_regexEscaped "\\* " _mapVar_values, "", params);
		sub(/, $/                                      , "", params);

		declaredVarsCode = "\t\t" "int " _mapVar_size ";" "\n" declaredVarsCode;
		if (_isFetching) {
			declaredVarsCode = "\t\t" _mapType_int " " _mapVar_oo ";" "\n" declaredVarsCode;
		}
		if (!_isRetSize) {
			#declaredVarsCode = "\t\t" _mapType_key   "* " _mapVar_keys ";" "\n" declaredVarsCode;
			#declaredVarsCode = "\t\t" _mapType_value "* " _mapVar_values ";" "\n" declaredVarsCode;
			if (_isFetching) {
				#conversionCode_pre = conversionCode_pre "\t\t" _mapVar_keys   " = NULL;" "\n";
				#conversionCode_pre = conversionCode_pre "\t\t" _mapVar_values " = NULL;" "\n";
				innerParams_sizeFetch = innerParams;
				sub(_mapVar_keys, "NULL", innerParams_sizeFetch);
				sub(_mapVar_values, "NULL", innerParams_sizeFetch);
				conversionCode_pre = conversionCode_pre "\t\t" _mapVar_size   " = " myBridgePrefix functionName_m "(" innerParams_sizeFetch ");" "\n";
			} else {
				#conversionCode_pre = conversionCode_pre "\t\t" _arraySizeVar " = " _arrayListVar ".size();" "\n";
				#conversionCode_pre = conversionCode_pre "\t\t" "int _size = " _arraySizeVar ";" "\n";
			}
		}

		if (_isRetSize) {
			#conversionCode_post = conversionCode_post "\t\t" _arraySizeVar " = " retVar_out_m ";" "\n";
			#_arraySizeMaxPaNa = _arraySizeVar;
		} else {
			#conversionCode_pre = conversionCode_pre "\t\t" _mapVar_keys   " = new " _mapType_key   "[" _mapVar_size "];" "\n";
			#conversionCode_pre = conversionCode_pre "\t\t" _mapVar_values " = new " _mapType_value "[" _mapVar_size "];" "\n";
			#conversionCode_post = conversionCode_post "\t\t" "delete[] " _mapVar_values ";" "\n";
			#conversionCode_post = conversionCode_post "\t\t" "delete[] " _mapVar_keys ";" "\n";
			conversionCode_pre = conversionCode_pre "\t\t" _mapType_key   " " _mapVar_keys   "[" _mapVar_size "];" "\n";
			conversionCode_pre = conversionCode_pre "\t\t" _mapType_value " " _mapVar_values "[" _mapVar_size "];" "\n";
		}

		if (_isFetching) {
			# convert to a HashMap
			#conversionCode_post = conversionCode_post "\t\t" _mapVar_oo ".reserve(" _mapVar_size ");" "\n";
			conversionCode_post = conversionCode_post "\t\t" "for (int i=0; i < " _mapVar_size "; i++) {" "\n";
			if (_isObj) {
				if (_isRetSize) {
					conversionCode_post = conversionCode_post "\t\t\t" _mapVar_oo "[" _mapVar_keys "[i]] = " _mapVar_values "[i];" "\n";
				} else {
					#conversionCode_post = conversionCode_post "\t\t\t" _mapVar_oo "[" myNameSpace "::Wrapp" _refObj "::GetInstance(" myBridgePrefix _addWrappVars "] = " _arrayPaNa "[i]);" "\n";
					conversionCode_post = conversionCode_post "\t\t\t" _mapVar_oo "[" _mapVar_keys "[i]] = " _mapVar_values "[i];" "\n";
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
	if (isArray) {
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
			_refObj = _fullClsName;
			_addWrappVars = cls_implId_indicesArgs[_implId];
			sub(/(,)?[^,]*$/, "", _addWrappVars); # remove last index
			_addWrappVars = trim(removeParamTypes(_addWrappVars));
		}
		sub(/^, /, "", _addWrappVars);

		_isF3     = match(_arrayPaNa, /_AposF3/);
		_isObj    = (_refObj != "");
		_isNative = (!_refObj && !_isObjc);

		_isRetSize = 0;
		if (_isObj) {
			_isRetSize = (_arrayPaNa == "RETURN_SIZE");
		}

		_arrayType = params;
		sub("(\\[\\])|(\\*)[ \t]*" _arrayPaNa ".*$", "", _arrayType);
		sub("^.*,[ \t]", "", _arrayType);
		_arraySizeMaxPaNa = _arrayPaNa "_sizeMax";
		_arraySizeVar     = _arrayPaNa "_size";
		_arraySizeRaw     = _arrayPaNa "_raw_size";

		_arrayListVar    = _arrayPaNa "_list";
		if (_isF3) {
			_arrListGenType = myNameSpace "::" "AIFloat3";
		} else if (_isObj) {
			_arrListGenType = myNameSpace "::" _refObjInt "*";
		} else if (_isNative) {
			#_arrListGenType  = convertJavaBuiltinTypeToClass(_arrayType);
			_arrListGenType  = _arrayType;
		} else {
			print("Error: no generic array type defined");
			exit(1);
		}
		_arrListType     = "std::vector<" _arrListGenType ">";    
		_arrListImplType = "std::vector<" _arrListGenType ">";  # TODO: should be unused, but needs check

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
			_arrayType_regexEscaped = regexEscape(_arrayType);
			if (_isFetching) {
				sub(_arrayType_regexEscaped "\\* " _arrayPaNa, "", params);
			} else {
				sub(_arrayType_regexEscaped "\\* " _arrayPaNa, _arrListType " " _arrayListVar, params);
			}
			sub(/^, /, "", params);
			sub(/, $/, "", params);
		}

		declaredVarsCode = "\t\t" "int " _arraySizeVar ";" "\n" declaredVarsCode;
		if (_isFetching) {
			declaredVarsCode = "\t\t" _arrListType " " _arrayListVar ";" "\n" declaredVarsCode;
		}
		if (!_isRetSize) {
			declaredVarsCode = "\t\t" _arrayType "* " _arrayPaNa ";" "\n" declaredVarsCode;
			declaredVarsCode = "\t\t" "int " _arraySizeRaw ";" "\n" declaredVarsCode;
			if (_isFetching) {
				declaredVarsCode = "\t\t" "int " _arraySizeMaxPaNa ";" "\n" declaredVarsCode;
				conversionCode_pre = conversionCode_pre "\t\t" _arraySizeMaxPaNa " = INT_MAX;" "\n";
				conversionCode_pre = conversionCode_pre "\t\t" _arrayPaNa " = NULL;" "\n";
				conversionCode_pre = conversionCode_pre "\t\t" _arraySizeVar " = " myBridgePrefix functionName_m "(" innerParams ");" "\n";
				conversionCode_pre = conversionCode_pre "\t\t" _arraySizeMaxPaNa " = " _arraySizeVar ";" "\n";
				conversionCode_pre = conversionCode_pre "\t\t" _arraySizeRaw " = " _arraySizeVar ";" "\n";
				if (_isF3) {
					conversionCode_pre = conversionCode_pre "\t\t" "if (" _arraySizeVar " % 3 != 0) {" "\n";
					conversionCode_pre = conversionCode_pre "\t\t\t" "throw std::runtime_error(\"returned AIFloat3 array has incorrect size, should be a multiple of 3.\");" "\n";
					conversionCode_pre = conversionCode_pre "\t\t" "}" "\n";
					conversionCode_pre = conversionCode_pre "\t\t" _arraySizeVar " /= 3;" "\n";
				}
			} else {
				conversionCode_pre = conversionCode_pre "\t\t" _arraySizeVar " = " _arrayListVar ".size();" "\n";
				conversionCode_pre = conversionCode_pre "\t\t" "int _size = " _arraySizeVar ";" "\n";
				if (_isF3) {
					conversionCode_pre = conversionCode_pre "\t\t" _arraySizeVar " *= 3;" "\n";
				}
				conversionCode_pre = conversionCode_pre "\t\t" _arraySizeRaw " = " _arraySizeVar ";" "\n";
			}
		}

		if (_isRetSize) {
			conversionCode_post = conversionCode_post "\t\t" _arraySizeVar " = " retVar_out_m ";" "\n";
			_arraySizeMaxPaNa = _arraySizeVar;
		} else {
			conversionCode_pre  = conversionCode_pre  "\t\t" _arrayPaNa " = new " _arrayType "[" _arraySizeRaw "];" "\n";
		}

		if (_isFetching) {
			# convert to an ArrayList
			conversionCode_post = conversionCode_post "\t\t" _arrayListVar ".reserve(" _arraySizeVar ");" "\n";
			conversionCode_post = conversionCode_post "\t\t" "for (int i=0; i < " _arraySizeMaxPaNa "; ++i) {" "\n";
			if (_isF3) {
				conversionCode_post = conversionCode_post "\t\t\t" _arrayListVar ".push_back(AIFloat3(" _arrayPaNa "[i], " _arrayPaNa "[i+1], " _arrayPaNa "[i+2]));" "\n";
				conversionCode_post = conversionCode_post "\t\t\t" "i += 2;" "\n";
			} else if (_isObj) {
				if (_isRetSize) {
					conversionCode_post = conversionCode_post "\t\t\t" _arrayListVar ".push_back(" myNameSpace "::Wrapp" _refObj "::GetInstance(" _addWrappVars ", i));" "\n";
				} else {
					conversionCode_post = conversionCode_post "\t\t\t" _arrayListVar ".push_back(" myNameSpace "::Wrapp" _refObj "::GetInstance(" _addWrappVars ", " _arrayPaNa "[i]));" "\n";
				}
			} else if (_isNative) {
				conversionCode_post = conversionCode_post "\t\t\t" _arrayListVar ".push_back(" _arrayPaNa "[i]);" "\n";
			}
			conversionCode_post = conversionCode_post "\t\t" "}" "\n";

			retParamType = _arrListType;
			retVar_out_m = _arrayListVar;
			retType = retParamType;
		} else {
			# convert from an ArrayList
			conversionCode_pre = conversionCode_pre "\t\t" "for (int i=0; i < _size; ++i) {" "\n";
			if (_isF3) {
				conversionCode_pre = conversionCode_pre "\t\t\t" "int arrInd = i*3;" "\n";
				conversionCode_pre = conversionCode_pre "\t\t\t" "AIFloat3 aif3 = " _arrayListVar "[i];" "\n";
				conversionCode_pre = conversionCode_pre "\t\t\t" _arrayPaNa "[arrInd]   = aif3.x;" "\n";
				conversionCode_pre = conversionCode_pre "\t\t\t" _arrayPaNa "[arrInd+1] = aif3.y;" "\n";
				conversionCode_pre = conversionCode_pre "\t\t\t" _arrayPaNa "[arrInd+2] = aif3.z;" "\n";
			} else if (_isObj) {
				conversionCode_pre = conversionCode_pre "\t\t\t" _arrayPaNa "[i] = " _arrayListVar "[i]->Get" _refObj "Id();" "\n";
			} else if (_isNative) {
				conversionCode_pre = conversionCode_pre "\t\t\t" _arrayPaNa "[i] = " _arrayListVar "[i];" "\n";
			}
			conversionCode_pre = conversionCode_pre "\t\t" "}" "\n";
		}

		if (!_isRetSize) {
			conversionCode_post = conversionCode_post "\t\t" "delete[] " _arrayPaNa ";" "\n";
		}
	}

	firstLineEnd = ";";
	mod_m = "";
	if (!isInterface_m) {
		firstLineEnd = " {";
		mod_m = "public ";
	}

	sub(/^, /, "", thrownExceptions);

	print("") >> outFile_wrp_h_m;
	print("") >> outFile_wrp_cpp_m;

	isBuffered_m = !isVoid_m && isBufferedFunc(fullName_m) && (params == "");
	if (!isInterface_m && isBuffered_m) {
		print("private:") >> outFile_wrp_h_m;
		print(indent_m retType " _buffer_" memName ";") >> outFile_wrp_h_m;
		print(indent_m "bool _buffer_isInitialized_" memName " = false;") >> outFile_wrp_h_m;
	}

	# print method doc comment
	fullName_doc_m = fullName_m;
	sub(/^[^_]*_/, "", fullName_doc_m); # remove OOAICallback_
	if (printIntAndStb_m) {
		printFunctionComment_Common(outFile_int_h_m, funcDocComment, fullName_doc_m, indent_m);
		printFunctionComment_Common(outFile_stb_h_m, funcDocComment, fullName_doc_m, indent_m);
	}
	printFunctionComment_Common(outFile_wrp_h_m, funcDocComment, fullName_doc_m, indent_m);

	printTripleFunc(retType, memName, params, thrownExceptions, outFile_int_h_m, outFile_stb_h_m, outFile_wrp_h_m, printIntAndStb_m, 0, 0, clsName_stb_m, clsName_wrp_m);

	isVoid_m = (retType == "void");

	if (!isInterface_m) {
		condRet_int_m = isVoid_int_m ? "" : retVar_int_m " = ";
		indent_m = indent_m "\t";

		if (isBuffered_m) {
			print(indent_m "if (!_buffer_isInitialized_" memName ") {") >> outFile_wrp_cpp_m;
			indent_m = indent_m "\t";
		}
		if (declaredVarsCode != "") {
			print(declaredVarsCode) >> outFile_wrp_cpp_m;
		}
		if (conversionCode_pre != "") {
			print(conversionCode_pre) >> outFile_wrp_cpp_m;
		}
		if (!ommitMainCall) {
			print(indent_m condRet_int_m myBridgePrefix functionName_m "(" innerParams ");") >> outFile_wrp_cpp_m;
		}
		if (conversionCode_post != "") {
			print(conversionCode_post) >> outFile_wrp_cpp_m;
		}
		if (isBuffered_m) {
			print(indent_m "_buffer_" memName " = " retVar_out_m ";") >> outFile_wrp_cpp_m;
			print(indent_m "_buffer_isInitialized_" memName " = true;") >> outFile_wrp_cpp_m;
			sub(/\t/, "", indent_m);
			print(indent_m "}") >> outFile_wrp_cpp_m;
			print("") >> outFile_wrp_cpp_m;
			retVar_out_m = "_buffer_" memName;
		}
		if (!isVoid_m) {
			print(indent_m "return " retVar_out_m ";") >> outFile_wrp_cpp_m;
		}
		sub(/\t/, "", indent_m);
		print(indent_m "}") >> outFile_wrp_cpp_m;
	}
}


function doWrappMember(fullName_dwm) {

	doWrapp_dwm = 1;

	return doWrapp_dwm;
}


#EXPORT(float) bridged__UnitDef_getUpkeep(int _skirmishAIId, int unitDefId, int resourceId); // REF:resourceId->Resource
function wrappFunctionDef(funcDef, commentEolTot) {

	size_funcParts = split(funcDef, funcParts, "(\\()|(\\)[ \t]+bridged__)|(\\)\\;)");
	# because the empty part after ");" would count as part as well
	size_funcParts--;

	fullName = funcParts[3];
	#fullName = trim(fullName);
	#sub(/.*[ \t]+/, "", fullName);

	retType = funcParts[2];
	#sub(/[ \t]*public/, "", retType);
	#sub(fullName, "", retType);
	#retType = trim(retType);

	params = funcParts[4];
#print("wrappFunctionDef: " retType " " fullName "(" params "); // " commentEolTot);
#return;

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


# grab callback function definition
/^EXPORT/ {

	funcLine = $0;
	# separate possible comment at end of line: // foo bar
	commentEol = funcLine;
	if (!sub(/.*\/\//, "", commentEol)) {
		commentEol = "";
	}
	# remove possible comment at end of line: // foo bar
	sub(/\/\/.*$/, "", funcLine);
	funcLine = trim(funcLine);
	if (match(funcLine, /\;$/)) {
		wrappFunctionDef(funcLine, commentEol);
	} else {
		print("Error: Function not declared in a single line.");
		exit 1;
	}
}



#UNUSED
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

	cls_implId_indicesArgs[myRootClass] = "int skirmishAIId";
	cls_name_indicesArgs[myRootClass]   = "int skirmishAIId";
	store_everything();

	printClasses();
	printIncludesHeaders();
}
