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

#if defined BUILDING_AI_INTERFACE


#include "SSkirmishAISpecifier.h"

#include <string.h>
#include <stdlib.h>


struct SSkirmishAISpecifier SSkirmishAISpecifier_copy(
		const struct SSkirmishAISpecifier* const orig) {

	struct SSkirmishAISpecifier copy;

	char* tmpStr = (char *) malloc(sizeof(char) * (strlen(orig->shortName) + 1));
	strcpy(tmpStr, orig->shortName);
	copy.shortName = tmpStr;

	tmpStr = (char *) malloc(sizeof(char) * (strlen(orig->version) + 1));
	strcpy(tmpStr, orig->version);
	copy.version = tmpStr;

	return copy;
}

void SSkirmishAISpecifier_delete(
		const struct SSkirmishAISpecifier* const spec) {
	free(const_cast<char*>(spec->shortName));
	free(const_cast<char*>(spec->version));
}



// by DeeViLiSh
static int string_simpleHash(const char* const input) {

	int b    = 378551; // Ramdom Ranges I've chosen (can be modified)
	int a    = 63689;
	int hash = 0;      // Output hash
	int i;             // Temp number that scrolls through the string array

	int size = strlen(input);
	for(i = 0; i < size; i++) { // Loop to convert each character
		hash = hash * a + input[i];       // Algorithm that hashs
		a    = a * b;
	}

	return (hash & 0x7FFFFFFF); // Returns the hashed string
}

int SSkirmishAISpecifier_hash(
		const struct SSkirmishAISpecifier* const spec) {

	bool useShortName = spec->shortName != NULL;
	bool useVersion = spec->version != NULL;

	int size_hashString = 0;
	if (useShortName) {
		size_hashString += strlen(spec->shortName);
	}
	size_hashString += 1; // for the '#' char
	if (useVersion) {
		size_hashString += strlen(spec->version);
	}
	size_hashString += 1; // for the '\0' char

	char hashString[size_hashString];
	hashString[0] = '\0';

	if (useShortName) {
		strcat(hashString, spec->shortName);
	}
	strcat(hashString, "#");
	if (useVersion) {
		strcat(hashString, spec->version);
	}

	int keyHash = string_simpleHash(hashString);

	return keyHash;
}

int SSkirmishAISpecifier_compare(
		const struct SSkirmishAISpecifier* const specThis,
		const struct SSkirmishAISpecifier* const specThat) {

	int comp = strcmp(specThis->shortName, specThat->shortName);
	if (comp == 0) {
		comp = strcmp(specThis->version, specThat->version);
	}
	return comp;
}

bool SSkirmishAISpecifier_isUnspecified(
		const struct SSkirmishAISpecifier* const spec) {
	return spec->shortName == NULL || spec->version == NULL;
}

static const SSkirmishAISpecifier unspecifiedSpec = {NULL, NULL};
struct SSkirmishAISpecifier SSkirmishAISpecifier_getUnspecified() {
	return unspecifiedSpec;
}


#ifdef __cplusplus
bool SSkirmishAISpecifier_Comparator::operator()(
		const struct SSkirmishAISpecifier& specThis,
		const struct SSkirmishAISpecifier& specThat) const {
	return SSkirmishAISpecifier_compare(&specThis, &specThat) < 0;
}
#endif // defined __cplusplus

#endif // defined BUILDING_AI_INTERFACE
