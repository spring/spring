#!/bin/awk
#
# This awk script contains common functions that may be used by other scripts.
# Contains functions suitable for creating Object Oriented wrappers of
# the callback in: rts/ExternalAI/Interface/SAICallback.h
# use like this:
# 	awk -f yourScript.awk -f common.awk [additional-params]
# this should work with all flavours of AWK (eg. gawk, mawk, nawk, ...)
#

BEGIN {
	# initialize things
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

function wrappFunction(retType, fullName, params) {

	simpleFullName = extractNormalPart(fullName);

	doWrapp = !match(simpleFullName, /^Clb_File/);

	if (doWrapp) {
		sub(/^int teamId(\, )?/, "", params);

		innerParams = removeParamTypes(params);

		funcFullName[size_funcs] = fullName;
		funcRetType[fullName] = retType;
		funcParams[fullName] = params;
		funcInnerParams[fullName] = innerParams;
		storeDocLines(funcDocComment, fullName);

		if (!(simpleFullName in funcSimpleFullName)) {
			funcSimpleFullName[simpleFullName] = fullName;
		} else if (!match(funcSimpleFullName[simpleFullName], fullName)) {
			funcSimpleFullName[simpleFullName] = funcSimpleFullName[simpleFullName] "," fullName;
		}

		size_funcs++;
	} else {
		print("function intentionally NOT wrapped: " fullName);
	}
}


END {
	# finalize things
}

