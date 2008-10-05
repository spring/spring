/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>
	
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SSAILibrary.h"

#include <string.h>
#include <stdlib.h>


SSAISpecifyer copySSAISpecifyer(const struct SSAISpecifyer* const orig) {
	
	struct SSAISpecifyer copy;
	
	char* tmpStr = (char *) malloc(sizeof(char) * (strlen(orig->shortName) + 1));
	strcpy(tmpStr, orig->shortName);
	copy.shortName = tmpStr;
	
	tmpStr = (char *) malloc(sizeof(char) * (strlen(orig->version) + 1));
	strcpy(tmpStr, orig->version);
	copy.version = tmpStr;
	
	return copy;
}
void deleteSSAISpecifyer(const struct SSAISpecifyer* const spec) {
	free(const_cast<char*>(spec->shortName));
	free(const_cast<char*>(spec->version));
}

#ifdef	__cplusplus
bool SSAISpecifyer_Comparator::operator()(const struct SSAISpecifyer& a, const struct SSAISpecifyer& b) const {

	bool isLess = false;
	
	int shortNameComp = strcmp(a.shortName, b.shortName);
	if (shortNameComp < 0) {
		return isLess = true;
	} else if (shortNameComp == 0) {
		int versionComp = strcmp(a.version, b.version);
		
		if (versionComp < 0) {
			isLess = true;
		}
	}

	return isLess;
}
bool SSAISpecifyer_Comparator::IsEmpty(const struct SSAISpecifyer& spec) {
	
	bool empty = false;
	
	if (spec.shortName == NULL || spec.version == NULL) {
		empty = true;
	}
	
	return empty;
}
#endif /* __cplusplus */

//char* SSAISpecifyer_toString(const SSAISpecifyer* sSAISpecifyer) {
//
//	char *spec = (char *)calloc(strlen(sSAISpecifyer->shortName) +
//			strlen(sSAISpecifyer->version) + 1 + 1, sizeof(char));
//
//	strcpy(spec, sSAISpecifyer->shortName);
//	strcat(spec, "#");
//	strcat(spec, sSAISpecifyer->version);
//
//	return spec;
//}
