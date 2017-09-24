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

#include "System/MainDefines.h"
#include "System/SafeCStrings.h"

#include <string.h>
#include <stdlib.h>

#if 0
static int string_simpleHash(const char* const input) {
	int b    = 378551;
	int a    = 63689;
	int hash = 0;
	int i;

	int size = strlen(input);
	for (i = 0; i < size; i++) {
		hash = hash * a + input[i];
		a    = a * b;
	}

	return (hash & 0x7FFFFFFF);
}

int SSkirmishAISpecifier_hash(
	const struct SSkirmishAISpecifier* const spec
) {
	const bool useShortName = (spec->shortName != NULL);
	const bool useVersion = (spec->version != NULL);

	size_t hashStringSize = 0;

	if (useShortName)
		hashStringSize += strlen(spec->shortName);

	hashStringSize += 1; // for the '#' char

	if (useVersion)
		hashStringSize += strlen(spec->version);

	hashStringSize += 1; // for the '\0' char

	std::vector<char> hashString(hashStringSize, 0);

	if (useShortName)
		STRCAT_T(&hashString[0], hashStringSize, spec->shortName);

	STRCAT_T(&hashString[0], hashStringSize, "#");

	if (useVersion)
		STRCAT_T(&hashString[0], hashStringSize, spec->version);

	return (string_simpleHash(&hashString[0]));
}
#endif

int SSkirmishAISpecifier_compare(
	const struct SSkirmishAISpecifier* const specThis,
	const struct SSkirmishAISpecifier* const specThat
) {
	int comp = strcmp(specThis->shortName, specThat->shortName);

	if (comp == 0)
		comp = strcmp(specThis->version, specThat->version);

	return comp;
}



#ifdef __cplusplus
bool SSkirmishAISpecifier_Comparator::operator()(
	const struct SSkirmishAISpecifier& specThis,
	const struct SSkirmishAISpecifier& specThat
) const {
	return (SSkirmishAISpecifier_compare(&specThis, &specThat) < 0);
}
#endif // defined __cplusplus

#endif // defined BUILDING_AI_INTERFACE

