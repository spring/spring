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

#include "SGAILibrary.h"

#include <string.h>
#include <stdlib.h>


SGAISpecifyer copySGAISpecifyer(const struct SGAISpecifyer* const orig) {
	
	struct SGAISpecifyer copy;
	
	char* tmpStr = (char *) malloc(sizeof(char) * (strlen(orig->shortName) + 1));
	strcpy(tmpStr, orig->shortName);
	copy.shortName = tmpStr;
	
	tmpStr = (char *) malloc(sizeof(char) * (strlen(orig->version) + 1));
	strcpy(tmpStr, orig->version);
	copy.version = tmpStr;
	
	return copy;
}
void deleteSGAISpecifyer(const struct SGAISpecifyer* const spec) {
	free(const_cast<char*>(spec->shortName));
	free(const_cast<char*>(spec->version));
}

#ifdef	__cplusplus
bool SGAISpecifyer_Comparator::operator()(const struct SGAISpecifyer& a, const struct SGAISpecifyer& b) const {

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
bool SGAISpecifyer_Comparator::IsEmpty(const struct SGAISpecifyer& spec) {
	
	bool empty = false;
	
	if (spec.shortName == NULL || spec.version == NULL) {
		empty = true;
	}
	
	return empty;
}
#endif /* __cplusplus */
