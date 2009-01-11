#!/bin/awk
#
# This awk script contains common functions that may be used by other scripts.
# Contains functions that help parsing storing and printing API doc comments,
# think of Doxygen or JavaDoc.
# the callback in: rts/ExternalAI/Interface/SAICallback.h
# use like this:
# 	awk -f yourScript.awk -f common.awk [additional-params]
# this should work with all flavours of AWK (eg. gawk, mawk, nawk, ...)
#

# NOTE: This functions HAS TO be implemented by the script using this as sort of
#       an include!
# This function has to return true (1) if a doc comment (eg: /** foo bar */)
# can be deleted.
# If there is no special condition you want to apply,
# it should always return true (1),
# cause there are additional mechanism to prevent accidential deleting.
#function canDeleteDocumentation() {
#	return 1;
#}


BEGIN {
	# initialize things
}

# Returns true (1) if the current line is part of a documentation comment
function isInsideDoc() {
	return isInsideDocComment__doc;
}
# Returns true (1) if stored docummentation is available
function hasStoredDoc() {
	return docComLines_num__doc > 0;
}
# Returns an array with doc lines, beginning at index 0
# and ending at index getStoredDocLines()-1
#function getStoredDoc() {
#	return docComLines__doc;
#}
# Returns the number of stored doc lines
#function getStoredDocLines() {
#	return docComLines_num__doc;
#}
# Stores the current doc lines in a container at a specified index
# This container can then be passed to printFunctionComment_Common() eg.
function storeDocLines(container__doc, index__doc) {

	container__doc[index__doc, "*"] = docComLines_num__doc;
	for (l__doc=0; l__doc < docComLines_num__doc; l__doc++) {
		container__doc[index__doc, l__doc] = docComLines__doc[l__doc];
	}
	docComLines_num__doc = 0;
}

# Prints a saved doc comment to a file with the specified indent
# expects the first doc-line at docsMap__doc[docIndex__doc, 0]
# expects the number of doc-lines in docsMap__doc[docIndex__doc, "*"]
function printFunctionComment_Common(outFile__doc, container__doc, index__doc, indent__doc) {

	numLines__doc = container__doc[index__doc, "*"];
	# print the documentation comment only if it is not empty
	if (numLines__doc > 0) {
		print(indent__doc "/**") >> outFile__doc;
		for (l__doc=0; l__doc < numLines__doc; l__doc++) {
			docLine__doc = container__doc[index__doc, l__doc];
			print(indent__doc " * " docLine__doc) >> outFile__doc;
		}
		print(indent__doc " */") >> outFile__doc;
	}
}

# end of doc comment
/\*\// {

	if (isInsideDocComment__doc == 1) {
		usefullLinePart__doc = $0;
		sub(/\*\/.*/, "", usefullLinePart__doc);
		sub(/^[ \t]*(\*)?/, "", usefullLinePart__doc);
		sub(/^[ \t]/, "", usefullLinePart__doc);
		usefullLinePart__doc = rtrim(usefullLinePart__doc);
		if (usefullLinePart__doc != "") {
			docComLines__doc[docComLines_num__doc++] = usefullLinePart__doc;
		}
	}
	isInsideDocComment__doc = 0;
}


# inside of doc comment
{
	if (isInsideDocComment__doc == 1) {
		usefullLinePart__doc = $0;
		sub(/^[ \t]*(\*)?/, "", usefullLinePart__doc);
		sub(/^[ \t]/, "", usefullLinePart__doc);
		usefullLinePart__doc = rtrim(usefullLinePart__doc);
		docComLines__doc[docComLines_num__doc++] = usefullLinePart__doc;
	} else {
		if (trim($0) != "") {
			linesWithNoDocComment__doc++;
		}
		# delete the last stored doc comment if it is not applicable to anything
		if (linesWithNoDocComment__doc > 2 && canDeleteDocumentation()) {
			docComLines_num__doc = 0;
		}
	}
}

# beginn of doc comment
/^[ \t]*\/\*\*/ {

	isInsideDocComment__doc = 1;
	docComLines_num__doc = 0;
	linesWithNoDocComment__doc = 0;

	usefullLinePart__doc = $0;
	sub(/^[ \t]*\/\*\*/, "", usefullLinePart__doc);
	sub(/^[ \t]/, "", usefullLinePart__doc);
	if (sub(/\*\/.*/, "", usefullLinePart__doc)) {
		isInsideDocComment__doc = 0;
	}
	usefullLinePart__doc = rtrim(usefullLinePart__doc);
	if (usefullLinePart__doc != "") {
		docComLines__doc[docComLines_num__doc++] = usefullLinePart__doc;
	}
}


END {
	# finalize things
}

