#!/bin/awk
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
	return match(metaInfo_p, /MAP:/);
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
		fullName    = funcFullName[f];
		retType     = funcRetType[fullName];
		params      = funcParams[fullName];
		innerParams = funcInnerParams[fullName];
		metaComment = funcMetaComment[fullName];

		size_nameParts = split(fullName, nameParts, "_");

		# go through the classes
		ancestorsP = "";
		last_ancestorsP = "";
		clsName = myClass;
		clsId = ancestorsP "-" clsName;
		last_clsId = "";
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
		clsName = part_getClsName(ancestorsP "_" nameP, metaComment);
		last_clsId = clsId;
		clsId = ancestorsP "-" clsName;

		isClass = part_isClass(nameP, metaComment);

		if (isClass) {
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

function wrappFunction(retType, fullName, params) {
	wrappFunctionPlusMeta(retType, fullName, params, "");
}

function wrappFunctionPlusMeta(retType, fullName, params, metaComment) {

	simpleFullName = extractNormalPart(fullName);

	doWrapp_wf = !match(simpleFullName, /^Clb_File/);

	if (doWrapp_wf) {
		params = trim(params);
		sub(/^int teamId(\, )?/, "", params);

		innerParams = removeParamTypes(params);

		funcFullName[size_funcs]  = fullName;
		funcRetType[fullName]     = retType;
		funcParams[fullName]      = params;
		funcInnerParams[fullName] = innerParams;
		funcMetaComment[fullName] = metaComment;
		storeDocLines(funcDocComment, fullName);

		if (!(simpleFullName in funcSimpleFullName)) {
			funcSimpleFullName[simpleFullName] = fullName;
		} else if (!match(funcSimpleFullName[simpleFullName], fullName)) {
			funcSimpleFullName[simpleFullName] = funcSimpleFullName[simpleFullName] "," fullName;
		}

		size_funcs++;
	} else {
		print("warning: function intentionally NOT wrapped: " fullName);
	}
}


END {
	# finalize things
}

