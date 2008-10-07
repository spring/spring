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

#include "SAIInterfaceLibrary.h"

#include <string.h>
#include <stdlib.h>

SAIInterfaceSpecifyer copySAIInterfaceSpecifyer(const struct SAIInterfaceSpecifyer* const orig) {
	
	struct SAIInterfaceSpecifyer copy;
	
	char* tmpStr = (char *) malloc(sizeof(char) * strlen(orig->shortName) + 1);
	strcpy(tmpStr, orig->shortName);
	copy.shortName = tmpStr;
	
	tmpStr = (char *) malloc(sizeof(char) * strlen(orig->version) + 1);
	strcpy(tmpStr, orig->version);
	copy.version = tmpStr;
	
	return copy;
}
void deleteSAIInterfaceSpecifyer(const struct SAIInterfaceSpecifyer* const spec) {
	free(const_cast<char*>(spec->shortName));
	free(const_cast<char*>(spec->version));
}


#ifdef	__cplusplus

#ifdef	__USE_CREG
CR_BIND(SSAIKey,)
CR_BIND(SGAIKey,)
#endif /* __USE_CREG */

bool SAIInterfaceSpecifyer_Comparator::operator()(const struct SAIInterfaceSpecifyer& a, const struct SAIInterfaceSpecifyer& b) const {

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
bool SAIInterfaceSpecifyer_Comparator::IsEmpty(const struct SAIInterfaceSpecifyer& spec) {
	
	bool empty = false;
	
	if (spec.shortName == NULL || spec.version == NULL) {
		empty = true;
	}
	
	return empty;
}

bool SSAIKey_Comparator::operator()(const struct SSAIKey& a, const struct SSAIKey& b) const {
	
	bool isLess = false;
	
	bool interfaceLess = SAIInterfaceSpecifyer_Comparator()(a.interface, b.interface);
	if (interfaceLess) {
		isLess = true;
	} else if (!(SAIInterfaceSpecifyer_Comparator()(b.interface, a.interface))) { // -> interfaces are equal
		bool aiLess = SSAISpecifyer_Comparator()(a.ai, b.ai);
		
		if (aiLess) {
			isLess = true;
		}
	}

	return isLess;
}
bool SSAIKey_Comparator::IsEmpty(const struct SSAIKey& key) {
	
	bool empty = false;
	
	if (SAIInterfaceSpecifyer_Comparator::IsEmpty(key.interface)
			|| SSAISpecifyer_Comparator::IsEmpty(key.ai)) {
		empty = true;
	}
	
	return empty;
}

bool SGAIKey_Comparator::operator()(const struct SGAIKey& a, const struct SGAIKey& b) const {

	bool isLess = false;
	
	bool interfaceLess = SAIInterfaceSpecifyer_Comparator()(a.interface, b.interface);
	if (interfaceLess) {
		isLess = true;
	} else if (!(SAIInterfaceSpecifyer_Comparator()(b.interface, a.interface))) { // -> interfaces are equal
		bool aiLess = SGAISpecifyer_Comparator()(a.ai, b.ai);
		
		if (aiLess) {
			isLess = true;
		}
	}

	return isLess;
}
bool SGAIKey_Comparator::IsEmpty(const struct SGAIKey& key) {
	
	bool empty = false;
	
	if (SAIInterfaceSpecifyer_Comparator::IsEmpty(key.interface)
			|| SGAISpecifyer_Comparator::IsEmpty(key.ai)) {
		empty = true;
	}
	
	return empty;
}

#endif /* __cplusplus */

