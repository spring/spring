#!/usr/bin/awk -f
#
# This awk script contains common functions that may be used by other scripts.
# Contains functions suitable for creating Object Oriented wrappers of
# the callback in: rts/ExternalAI/Interface/SSkirmishAICallback.h
# use like this:
# 	awk -f yourScript.awk -f common.awk [additional-params]
# this should work with all flavours of AWK (eg. gawk, mawk, nawk, ...)
#

BEGIN {
	# initialize things

	myRootClass = "OOAICallback";

	# Storage containers
	# ==================
	#
	# These are filled up when calling storeClasses()
	#
	# cls_id_name["*"] = 3
	# cls_id_name[0] = "Unit"
	# cls_id_name[1] = "Group"
	# cls_id_name[2] = "Line"
	#
	# cls_name_id["Unit"]  = 0
	# cls_name_id["Group"] = 1
	# cls_name_id["Line"]  = 2
	#
	# cls_implName_name["CurrentCommand"] = "Command"
	# cls_implName_name["SupportedCommand"] = "SCommandDescription"
	#
	# cls_name_implIds["Command,*"] = 2
	# cls_name_implIds["Command,0"] = "OOAICallback,Unit,SupportedCommand"
	# cls_name_implIds["Command,1"] = "OOAICallback,Group,SupportedCommand"
	# cls_name_implIds["Line,*"] = 1
	# cls_name_implIds["Line,0"] = "OOAICallback,Map,Line"
	#
	# cls_implId_indicesArgs["OOAICallback,Unit,SupportedCommand"]  = "int unitId, int supportedCommandId"
	# cls_implId_indicesArgs["OOAICallback,Group,SupportedCommand"] = "int groupId, int supportedCommandId"
	# cls_implId_indicesArgs["OOAICallback,Map,Line"]               = "int lineId"
	# cls_implId_indicesArgs["OOAICallback,Unit"]                   = "int unitId"
	# cls_implId_indicesArgs["OOAICallback,Unit,MoveData"]          = "int unitId"
	# cls_implId_indicesArgs["OOAICallback,Engine"]                 = ""
	#
	# cls_name_indicesArgs["Unit"] = "int unitId"
	#
	# cls_implId_fullClsName["OOAICallback,Unit,SupportedCommand"]  = "UnitSupportedCommand";
	# cls_implId_fullClsName["OOAICallback,Group,SupportedCommand"] = "GroupSupportedCommand";
	# cls_implId_fullClsName["OOAICallback,Map,Line"]               = "Line";
	#
	#
	# cls_name_members["Unit,*"] = 4
	# cls_name_members["Unit,0"] = "getPos"
	# cls_name_members["Unit,1"] = "isActive"
	# cls_name_members["Unit,2"] = "getPower"
	# cls_name_members["Unit,3"] = "moveTo"
	# cls_name_members["OOAICallback,*"] = 4
	# cls_name_members["OOAICallback,0"] = "getEnemyUnits"
	# cls_name_members["OOAICallback,1"] = "getFriendlyUnits"
	# cls_name_members["OOAICallback,2"] = "getFriendlyUnitsIn"
	# cls_name_members["OOAICallback,3"] = "getFeatures"
	#
	# cls_memberId_retType["Unit,getPower"] = "float"
	# cls_memberId_retType["Unit,moveTo"]   = "void"
	# cls_memberId_retType["OOAICallback,getFriendlyUnits"]   = "int"
	# cls_memberId_retType["OOAICallback,getFriendlyUnitsIn"] = "int"
	# cls_memberId_retType["OOAICallback,getFeatures"]        = "int"
	#
	# cls_memberId_params["Unit,getPower"] = ""
	# cls_memberId_params["Unit,moveTo"]   = "float[] pos_posF3"
	# cls_memberId_params["OOAICallback,getFriendlyUnits"]   = ""
	# cls_memberId_params["OOAICallback,getFriendlyUnitsIn"] = "float[] pos_posF3, float radius"
	# cls_memberId_params["OOAICallback,getFeatures"]        = ""
	#
	#
	# cls_memberId_isFetcher["OOAICallback,getEnemyUnits"]      = 1
	# cls_memberId_isFetcher["OOAICallback,getFriendlyUnits"]   = 1
	# cls_memberId_isFetcher["OOAICallback,getFriendlyUnitsIn"] = 1
	# cls_memberId_isFetcher["OOAICallback,getFeatures"]        = 1
	# cls_memberId_isFetcher["Unit,getPower"]                   = 0
	#
	# cls_memberId_metaComment["OOAICallback,getFeatures"] = "FETCHER:MULTI:NUM:..."
	# cls_memberId_metaComment["Unit,getPower"]            = ""
	#
	#
	#
	#

	size_funcs = 0;
}

function isBufferedFunc(funcFullName_b) {
	return matchesAnyKey(funcFullName_b, myBufferedClasses);
}

function isBufferedClass(clsName_bc) {
	return matchesAnyKey(clsName_bc, myBufferedClasses);
}

function part_isClass(namePart_p, metaInfo_p) {
	return startsWithCapital(namePart_p);
}
function part_isStatic(namePart_p, metaInfo_p) {
	return match(metaInfo_p, /STATIC/);
}
function part_isArray(namePart_p, metaInfo_p) {
	return match(metaInfo_p, /ARRAY:/);
}
function part_isMap(namePart_p, metaInfo_p) {
	return match(metaInfo_p, /(^|[ \t])MAP([ \t]|$)/);
}

function part_isMultiSize(namePart_p, metaInfo_p) {
	return match(metaInfo_p, /FETCHER:MULTI:IDs:/);
}
function part_isMultiFetch(namePart_p, metaInfo_p) {
	return match(metaInfo_p, /FETCHER:MULTI:NUM:/);
}
function part_isSingleFetch(namePart_p, metaInfo_p) {
	return match(metaInfo_p, /FETCHER:SINGLE:/);
}
function part_isFetcher(metaInfo_p) {
	return match(metaInfo_p, /FETCHER:/);
}
function part_getFetcherPart(metaInfo_p) {

	fetcherPart_p = metaInfo_p;

	# remove pre class part
	sub(/.*FETCHER:/, "", fetcherPart_p);
	sub(/^MULTI:/,    "", fetcherPart_p);
	sub(/^SINGLE:/,   "", fetcherPart_p);
	sub(/^IDs:/,      "", fetcherPart_p);
	sub(/^NUM:/,      "", fetcherPart_p);

	# remove post class part
	sub(/: .*/,     "", fetcherPart_p);

	return fetcherPart_p;
}
function part_getFetchedClassPart(metaInfo_p) {

	fetchedClsPart_p = part_getFetcherPart(metaInfo_p);

	# remove post class part
	sub(/:.*/,     "", fetchedClsPart_p);

	return fetchedClsPart_p;
}
function part_getFetchedClass(metaInfo_p) {

	fetchedCls_p = part_getFetchedClassPart(metaInfo_p);

	# remove impl class
	sub(/^.*-/, "", fetchedCls_p);

	return fetchedCls_p;
}
function part_getFetchedImplClass(metaInfo_p) {

	fetchedCls_p = part_getFetchedClassPart(metaInfo_p);

	# remove (interface-) class
	sub(/-.*$/, "", fetchedCls_p);

	return fetchedCls_p;
}
function part_getFetcherIndArg(metaInfo_p) {

	fetcherIndArgPart_p = part_getFetcherPart(metaInfo_p);

	# remove post indices arg
	if (match(/:/, fetcherIndArgPart_p)) {
		sub(/.*:/, "", fetcherIndArgPart_p);
	} else {
		fetcherIndArgPart_p = "";
	}

	return fetcherIndArgPart_p;
}

function part_isErrorReturn(metaInfo_p) {
	return match(metaInfo_p, /error-return:/);
}
function part_getErrorReturnValueOk(metaInfo_p) {

	errRetValOk_p = metaInfo_p;

	sub(/^.*error-return:/, "", errRetValOk_p);
	sub(/[ \t].*$/,         "", errRetValOk_p);
	sub(/=OK.*$/, "", errRetValOk_p);

	return errRetValOk_p;
}

function part_isAvailable(namePart_p, metaInfo_p) {
	return match(metaInfo_p, /AVAILABLE:/);
}
function part_hasReferenceObject(namePart_p, metaInfo_p) {
	return match(metaInfo_p, /REF:/);
}

function part_getClassName(ancestorsPlusName_p, metaInfo_p) {

	clsName_p = ancestorsPlusName_clsName[ancestorsPlusName_p];

	if (clsName_p == "") {
		clsName_p = ancestorsPlusName_p;
		sub(/^.*_/, "", clsName_p);

		ancestorsPlusName_clsName[ancestorsPlusName_p] = clsName_p;
	}

	return clsName_p;
}
function part_getInterfaceName(ancestorsPlusName_p, metaInfo_p) {

	intName_p = ancestorsPlusName_intName[ancestorsPlusName_p];

	if (intName_p == "") {
		isIntNameDifferent = match(metaInfo_p, /FETCHER:MULTI:[^: ]*:[^: -]*-[^: -]*/);
if (isIntNameDifferent) { print("isIntNameDifferent: " ancestorsPlusName_p " " metaInfo_p); }
		if (isIntNameDifferent) {
			intName_p = metaInfo_p;
			sub(/FETCHER:MULTI:[^: ]*:[^: -]*-/, "", intName_p);
		} else {
			intName_p = part_getClassName(ancestorsPlusName_p, metaInfo_p);
		}

		ancestorsPlusName_intName[ancestorsPlusName_p] = intName_p;
	}

	return intName_p;
}

function part_getIndicesArgs(clsName_p, implClsName_p, params_p, metaComment_p, parentImplClsId_p) {

	indicesArg_p = "";

#if (clsName_p == "OrderPreview") { print("\nOrderPreview params_p: " params_p); }
#if (clsName_p == "MoveData") { print("\nMoveData params_p: " params_p); }
#if (clsName_p == "Unit") { print("Unit params_p: " params_p); }
#if (clsName_p == "Group") { print("Group params_p: " params_p); }
	if (part_isFetcher(metaComment_p)) {
		indicesArg_p = part_getFetcherIndArg(metaComment_p);
		if (indicesArg_p != "") {
			indicesArg_p = "int " indicesArg_p;
		}
	} else {
		tmp_indArg_p = clsName_p;
		tmp_indArg_p = "int " lowerize(tmp_indArg_p) "Id";
#if (params_p != "") { print("params_p: " params_p); }
		if (match(params_p, tmp_indArg_p)) {
			indicesArg_p = tmp_indArg_p;
#if (clsName_p == "MoveData") { print("matched: #" tmp_indArg_p "#" params_p "#"); }
		} else {
			tmp_indArg_p = _implClsName;
			tmp_indArg_p = "int " lowerize(tmp_indArg_p) "Id";
			if (match(params_p, tmp_indArg_p)) {
#print("matched: #" tmp_indArg_p "#" params_p "#")
				indicesArg_p = tmp_indArg_p;
			}
		}
	}
#if (clsName_p == "MoveData") { print("so far: #" tmp_indArg_p "#" params_p "#" indicesArg_p "#"); }

	if (indicesArg_p != "") {
		# include the params before the indices arg
		indicesArgs_p = params_p;
		_found = sub(indicesArg_p ".*", "", indicesArgs_p);
		if (!_found) {
			print("indices arg \"" indicesArg_p "\" not found in params: " params_p);
			exit(1);
		}
		indicesArgs_p = indicesArgs_p indicesArg_p;
	} else if (parentImplClsId_p in cls_implId_indicesArgs) {
		# get parent class indices args
#if (clsName_p == "MoveData") { print("parentImplClsId_p: #" parentImplClsId_p "#" cls_implId_indicesArgs[parentImplClsId_p] "#"); }
		indicesArgs_p = cls_implId_indicesArgs[parentImplClsId_p];
	} else {

		print("caution: possible problem with indices args of " implClsName_p);
		# This is very hacky! may very well breack in the future.
		# use only first param, if there is one
		indicesArgs_p = params_p;
		sub(/,.*$/, "", indicesArgs_p);
		if (!match(indicesArgs_p, /^int /)) {
			# only use 'int' type params as indices args
			indicesArgs_p = "";
		}
	}
#print("ind args: #" implClsName_p "#" indicesArgs_p "#")
#if (clsName_p == "Unit") { print( "Unit indicesArgs_p:  " indicesArgs_p); }
#if (clsName_p == "Group") { print("Group indicesArgs_p: " indicesArgs_p); }
#if (clsName_p == "MoveData") { print("end: #" tmp_indArg_p "#" params_p "#" indicesArgs_p "#\n\n"); }
#if (clsName_p == "OrderPreview") { print("end: #" tmp_indArg_p "#" params_p "#" indicesArgs_p "#\n\n"); }

	return indicesArgs_p;
}

function store_class(ancestors_s, clsName_s) {

	if (!(clsName_s in class_ancestors)) {
		# store ancestor-lineup if not yet stored
		class_ancestors[clsName_s] = ancestors_s;
	} else if (!match(class_ancestors[clsName_s], "((^)|(,))" ancestors_s "(($)|(,))")) {
		# add ancestor-lineup
		class_ancestors[clsName_s] = class_ancestors[clsName_s] "," ancestors_s;
	}

	if (!(ancestors_s in ancestors_class)) {
		ancestors_class[ancestors_s] = clsName_s;
	} else if (!match(ancestors_class[ancestors_s], "((^)|(,))" clsName_s "(($)|(,))")) {
	#} else if (!match(ancestors_class[ancestors_s], clsName_s)) { # NO WRAPP
		ancestors_class[ancestors_s] = ancestors_class[ancestors_s] "," clsName_s;
	}
}


function extractAncestors(fullName_ea) {

	ancestors_ea = fullName_ea;

	sub(/[^_]*$/,  "", ancestors_ea); # remove function name, leaving only classes
	sub(/[^_]*_$/, "", ancestors_ea); # remove last class, leaving only ancestors
	gsub(/_/, ",", ancestors_ea);

	return ancestors_ea;
}




function store_cls(clsName_s) {

#print("store_cls: " clsName_s);
	# store only if not yet stored
	if (!(clsName_s in cls_name_id)) {
		_cls_size = cls_id_name["*"];
		cls_name_id[clsName_s] = _cls_size;
		cls_id_name[_cls_size] = clsName_s;
#print("store_cls: " _cls_size " " clsName_s);
		_cls_size++;
		cls_id_name["*"] = _cls_size;
	}
}

function store_implCls(implClsName_s, clsName_s) {

	# store only if not yet stored
	if (!(implClsName_s in cls_implName_name)) {
		cls_implName_name[implClsName_s] = clsName_s;
#print("impl: " implClsName_s " " clsName_s);
	}
}

function store_anc(clsName_s, implClsName_s, ancestors_s, indicesArgs_s) {

	_implId = myRootClass "," ancestors_s implClsName_s;
#print("_implId: " _implId " " implClsName_s);

	if (!(_implId in cls_implId_indicesArgs)) {
		# store only if not yet stored
		cls_implId_indicesArgs[_implId] = indicesArgs_s;
		cls_name_indicesArgs[clsName_s] = indicesArgs_s;
		_cls_size = cls_name_implIds[clsName_s ",*"];
		cls_name_implIds[clsName_s "," _cls_size] = _implId;
#print("_elem: (" clsName_s "," _cls_size ") " _implId);
		_cls_size++;
#print("_size: (" clsName_s ",*" ") " _cls_size);
		cls_name_implIds[clsName_s ",*"] = _cls_size;
#print("anc: " _cls_size " " _implId " " indicesArgs_s);
	} else if (length(indicesArgs_s) > length(cls_implId_indicesArgs[_implId])) {
		# if already stored, but we have more indices args now, store them
		cls_implId_indicesArgs[_implId] = indicesArgs_s;
	}
}

function store_mem(clsName_s, memName_s, retType_s, params_s, isFetcher_s, metaComment_s) {

	_memId = clsName_s "," memName_s;
	if (!(_memId in cls_memberId_retType)) {
		_cls_sizeInd = clsName_s ",*";
		if (!(_cls_sizeInd in cls_name_members)) {
			cls_name_members[_cls_sizeInd] = 0;
		}

		_cls_size = cls_name_members[_cls_sizeInd];
		cls_name_members[clsName_s "," _cls_size] = memName_s;
		_cls_size++;
		cls_name_members[_cls_sizeInd] = _cls_size;

		cls_memberId_retType[_memId]     = retType_s;
		cls_memberId_params[_memId]      = params_s;
		cls_memberId_isFetcher[_memId]   = isFetcher_s;
		cls_memberId_metaComment[_memId] = metaComment_s;
if (clsName_s == "") {
print("ERROR: mem: " _cls_size " #" retType_s " " _memId "(" params_s ")# " isFetcher_s " // " metaComment_p);
}
	}
}



function store_classesNamesByFetchersAndImplClassNames() {

	cls_id_name["*"] = 0;

	store_cls(myRootClass);

	for (f=0; f < size_funcs; f++) {
		fullName    = funcFullName[f];
		metaComment = funcMetaComment[fullName];

		if (part_isFetcher(metaComment)) {
			_clsName = part_getFetchedClass(metaComment);
			store_cls(_clsName);
			_implClsName = part_getFetchedImplClass(metaComment);
			if (_implClsName != _clsName) {
				store_implCls(_implClsName, _clsName);
			}
		}
	}
}

function store_singleNoFetcherClassNames() {

	for (f=0; f < size_funcs; f++) {
		fullName    = funcFullName[f];
		metaComment = funcMetaComment[fullName];

		nameParts_size = split(fullName, nameParts, "_");
		for (p=0; p < nameParts_size; p++) {
			_namePart = nameParts[p+1];
			if (part_isClass(_namePart, "") && !(_namePart in cls_implName_name)) {
				store_cls(_namePart);
			}
		}
	}
}

function store_classNamesAncestors() {

	# initialize ancestor counters to 0
	_cls_size = cls_id_name["*"];
	for (c=0; c < _cls_size; c++) {
		cls_name_implIds[cls_id_name[c] ",*"] = 0;
	}

	# store the root class
	# These two may be set differently by scripts using this script.
	if (!(myRootClass in cls_implId_indicesArgs)) {
		cls_implId_indicesArgs[myRootClass] = "";
	}
	if (!(myRootClass in cls_name_indicesArgs)) {
		cls_name_indicesArgs[myRootClass] = "";
	}
	cls_name_implIds[myRootClass ",*"]  = 1;
	cls_name_implIds[myRootClass "," 0] = myRootClass;

	for (f=0; f < size_funcs; f++) {
		fullName    = funcFullName[f];
		metaComment = funcMetaComment[fullName];
		params      = funcParams[fullName];

		if (part_isFetcher(metaComment)) {
			continue;
			#_clsName     = part_getFetchedClass(metaComment);
			#_implClsName = part_getFetchedImplClass(metaComment);
		} else {
			_implClsName = fullName;
			sub(/(^|_)?[^_]*$/, "", _implClsName); # remove function name, leaving only classes
			sub(/^.*_/,   "", _implClsName); # remove all except last class
			if (_implClsName in cls_implName_name) {
				_clsName = cls_implName_name[_implClsName];
			} else {
				_clsName = _implClsName;
			}
		}

		if (_implClsName != "") {
			_ancestors       = extractAncestors(fullName);
			_parentImplClsId = myRootClass "," _ancestors;
			sub(/,$/, "", _parentImplClsId);
			_indicesArgs     = part_getIndicesArgs(_clsName, _implClsName, params, metaComment, _parentImplClsId);

			store_anc(_clsName, _implClsName, _ancestors, _indicesArgs);
		}
	}

	# add ancestors for the remaining classes
	# these should be a few only, mainly those without fetchers (eg. Engine)
	_cls_size = cls_id_name["*"];
	_unancestorized_size = 0;
	# find un-ancestorized
	for (c=0; c < _cls_size; c++) {
		_clsX_impls_size = cls_name_implIds[cls_id_name[c] ",*"];
		if (_clsX_impls_size == 0) {
			_unancestorized[_unancestorized_size++] = cls_id_name[c];
		}
	}
	# ancestorize
	for (f=0; f < size_funcs; f++) {
		fullName = funcFullName[f];

		for (c=0; c < _unancestorized_size; c++) {
			_uaz = _unancestorized[c];
			if (match(fullName, "(^|_)" _uaz "_")) {
				_implClsName = _uaz;
				_clsName     = _uaz;
				_ancestors   = fullName;
				gsub(/_/, ",", _ancestors);
				sub("(^|,)?" _uaz ",.*$", "", _ancestors);
				_indicesArgs = cls_implId_indicesArgs[_ancestors];
#print("store_anc: #" _clsName "#" _implClsName "#" _ancestors "#" _indicesArgs "#");

				store_anc(_clsName, _implClsName, _ancestors, _indicesArgs);
				_unancestorized[c] = "XXXXXXXXXX";
			}
		}
	}

	#mySortKeys(cls_implId_indicesArgs);
}

function store_classMembers() {

	for (f=0; f < size_funcs; f++) {
		fullName    = funcFullName[f];
		metaComment = funcMetaComment[fullName];
		retType     = funcRetType[fullName];
		params      = funcParams[fullName];

		nameParts_size = split(fullName, nameParts, "_");
		_isFetcher = part_isFetcher(metaComment);
		#if (_isFetcher) {
		#	_clsName = part_getFetchedClass(metaComment);
		#} else {
			_clsName = nameParts[nameParts_size-1];
		#}
		_memName = nameParts[nameParts_size];

		if (_clsName == "") {
			_clsName = myRootClass
		}

		if (_clsName in cls_implName_name) {
			_clsName = cls_implName_name[_clsName];
		}
#print("mm: " _clsName " " metaComment);
		store_mem(_clsName, _memName, retType, params, _isFetcher, metaComment);
	}
}


function store_simpleFetchers() {

	#print("TODO: enlist simple fetchermethods here, for classes that have no fetcher (eg \"public Engine getEngine()\")");
	for (_implId in cls_implId_indicesArgs) {
		_implId_parent = _implId;
		sub(/,[^,]*$/, "", _implId_parent);

		_indArgs        = cls_implId_indicesArgs[_implId];
		_indArgs_parent = cls_implId_indicesArgs[_implId_parent];
		_myIndArgs = _indArgs;
		sub(_indArgs_parent, "", _myIndArgs);
		if ((_myIndArgs == "") && (_implId != "") && (_implId != myRootClass)) {
			#print("needs simple fetcher: " _implId " " _myIndArgs);
			_clsName = _implId_parent;
			sub(/^.*,/, "", _clsName);
			_toFetchCls = _implId
			sub(/^.*,/, "", _toFetchCls);
			_memName = "get" _toFetchCls;
			_retType = "void";
			_params = _indArgs_parent;
			_isFetcher = 1;
			_metaComment = "REF:RETURN->" _toFetchCls;
#print("simple fetcher: #"_clsName "#" _memName "#" _retType "#" _params "#" _isFetcher "#" _metaComment);
			store_mem(_clsName, _memName, _retType, _params, _isFetcher, _metaComment);
		}
	}
}


function store_fullClassNames() {

	for (_cn in cls_name_id) {
		_impls_size = cls_name_implIds[_cn ",*"];
		if (_impls_size > 1) {
			for (_i=0; _i < _impls_size; _i++) {
				_implId = cls_name_implIds[_cn "," _i];
				_lastAncName = _implId;
				sub(/,[^,]*$/, "", _lastAncName); # remove class name
				sub(/^.*,/,    "", _lastAncName); # remove pre last ancestor name
				_implClsName = _implId;
				sub(/^.*,/, "", _implClsName); # remove pre impl class name
				_fullClassName = _lastAncName _implClsName; # eg.: Unit SupportedCommand
				cls_implId_fullClsName[_implId] = _fullClassName;
			}
		} else {
			_fullClassName = _cn;
			_implId = cls_name_implIds[_cn ",0"];
			_implClsName = _implId;
			sub(/^.*,/, "", _implClsName); # remove pre impl class name
			cls_implId_fullClsName[_implId] = _implClsName;
		}
#print("_implId: (" _cn ": " _impls_size ") " _implId " -> " cls_implId_fullClsName[_implId]);
	}

#for (_ii in cls_implId_fullClsName) {
#	print("storedFullClass: " _ii " -> " cls_implId_fullClsName[_ii]);
#}
}


function store_everything() {

	mySort(funcFullNames);

	store_classesNamesByFetchersAndImplClassNames();
	store_singleNoFetcherClassNames();
	store_classNamesAncestors();
	store_classMembers();
	store_simpleFetchers();
#for (_clsId in cls_id_name) {
#	print("cls id name: " _clsId " -> " cls_id_name[_clsId]);
#}
	store_fullClassNames();




# BEGIN: multi line comment
if (1 == 0) {
	additionalClsIndices["*"] = 0;
	additionalClsIndices["-" myClass "*"] = 0;

	# store classes and assign functions
	for (f=0; f < size_funcs; f++) {
		fullName    = funcFullName[f];
		retType     = funcRetType[fullName];
		params      = funcParams[fullName];
		innerParams = funcInnerParams[fullName];
		metaComment = funcMetaComment[fullName];

		size_nameParts = split(fullName, nameParts, "_");

		# go through the classes
		ancestorsP      = "";
		last_ancestorsP = "";
		clsName         = myClass;
		clsId           = ancestorsP "-" clsName;
		last_clsId      = "";

		lastP = nameParts[size_nameParts];

		if (part_isFetcher(metaComment)) {
			cls = part_getClassName(ancestorsP "_" nameP, metaComment)
			classes[classes_size] = "";
		}

		# go through the pre-parts of callback function f (eg, the classes)
		# parts are separated by '_'
		for (np=0; np < size_nameParts-1; np++) {
			last_clsName = clsName;
			last_clsId   = clsId;
			nameP        = nameParts[np+1];

			# register class
			clsName = part_getClassName(ancestorsP "_" nameP, metaComment);
			clsId   = ancestorsP "-" clsName;
			store_class(ancestorsP, clsName);

			if (additionalClsIndices[clsId "*"] == "") {
				additionalClsIndices[clsId "*"] = additionalClsIndices[last_clsId "*"];
			}
			if (part_isMultiSize(nameP, metaComment)) {
				parentNumInd = additionalClsIndices[last_clsId "*"];
				additionalClsIndices[clsId "*"] = parentNumInd + 1;
			}

			secondLast_clsName = last_clsName;
			last_ancestorsP = ancestorsP;
			ancestorsP = ancestorsP "_" nameP;
		}

		nameP = nameParts[size_nameParts];
		secondLast_clsName = last_clsName;
		last_clsName = clsName;
		clsName = part_getClassName(ancestorsP "_" nameP, metaComment);
		last_clsId = clsId;
		clsId = ancestorsP "-" clsName;

		isClass = part_isClass(nameP, metaComment);
#print("fullName: " fullName);

		if (isClass) {
#print("isClass: " nameP);
			if (additionalClsIndices[clsId "*"] == "") {
				additionalClsIndices[clsId "*"] = additionalClsIndices[last_clsId "*"];
			}
			if (part_isMultiSize(nameP, metaComment)) {
				if (ancestorsClass_multiSizes[clsId "*"] == "") {
					ancestorsClass_multiSizes[clsId "*"] = 0;
					size_parentInd = additionalClsIndices[last_clsId "*"];
					additionalClsIndices[clsId "#" size_parentInd] = "";
					additionalClsIndices[clsId "*"] = size_parentInd + 1;
				}

				ancestorsClass_multiSizes[clsId "#" ancestorsClass_multiSizes[clsId "*"]] = fullName;

				ancestorsClass_multiSizes[clsId "*"]++;
			} else if (part_isAvailable(nameP, metaComment)) {
				ancestorsClass_available[clsId] = fullName;
			}
		} else {
			# store names of additional parameters
			size_parentInd = additionalClsIndices[last_clsId "*"];
			if (additionalClsIndices[last_clsId "#" (size_parentInd-1)] == "") {
				size_paramNames = split(innerParams, paramNames, ", ");
				for (pci=0; pci < size_parentInd; pci++) {
					if (additionalClsIndices[last_clsId "#" pci] == "") {
						indName = paramNames[1 + pci];
						additionalClsIndices[last_clsId "#" pci] = indName;
					}
				}
			}

			# assign the function to a class
			isStatic = part_isStatic(nameP, metaComment);
			if (isStatic) {
				belongsTo = secondLast_clsName;
				ancest    = last_ancestorsP;
			} else {
				belongsTo = last_clsName;
				ancest    = last_ancestorsP;
			}
			# store one way ...
			funcBelongsTo[fullName] = ancest "-" belongsTo;

			# ... and the other
			sizeI_belongsTo = ancest "-" belongsTo "*";
			if (ownerOfFunc[sizeI_belongsTo] == "") {
				ownerOfFunc[sizeI_belongsTo] = 0;
			}
			ownerOfFunc[ancest "-" belongsTo "#" ownerOfFunc[sizeI_belongsTo]] = fullName;
			ownerOfFunc[sizeI_belongsTo]++;
		}
	}

	# store interfaces
	for (clsName in class_ancestors) {
		size_ancestorParts = split(class_ancestors[clsName], ancestorParts, ",");

		# check if an interface is needed
		if (size_ancestorParts > 1) {
			interfaces[clsName] = 1;

			# assign functions of the first implementation to the interface as reference
			ancCls     = ancestorParts[1] "-" clsName;
			anc        = ancestorParts[1] "_" clsName;
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
			size_memCls              = split(interface_class[clsName], memCls, ",");
			for (mc=0; mc < size_memCl; mc++) {
				ancestorsInterface_multiSizes[clsName "-" memCls[mc+1] "*"] = ancestorsClass_multiSizes[anc "-" memCls[mc+1] "*"];
			}
			additionalIntIndices[clsName "*"] = additionalClsIndices[ancCls "*"];

			# generate class names for the implementations of the interface
			for (a=0; a < size_ancestorParts; a++) {
				implClsName = ancestorParts[a+1];
				#sub(/^_$/, "ROOT", implClsName);
				gsub(/_/, "", implClsName);
				implClsName = implClsName clsName "Impl";

				implClsNames[ancestorParts[a+1] "-" clsName] = implClsName;
			}
		}
	}
}
# END: multi line comment
}


function wrappFunction(retType, fullName, params) {
	wrappFunctionPlusMeta(retType, fullName, params, "");
}

function wrappFunctionPlusMeta(retType, fullName, params, metaComment) {

	# This function has to be defined in another script
	doWrapp_wf = doWrappOO(fullName, params, metaComment);

	if (doWrapp_wf) {
		params      = trim(params);
		innerParams = removeParamTypes(params);

		funcFullName[size_funcs]  = fullName;
		funcRetType[fullName]     = retType;
		funcParams[fullName]      = params;
		funcInnerParams[fullName] = innerParams;
		funcMetaComment[fullName] = trim(metaComment);
		storeDocLines(funcDocComment, fullName);

		size_funcs++;
	} else {
		print("note: function intentionally NOT wrapped: " fullName);
	}
}


END {
	# finalize things
}

