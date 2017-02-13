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

/**
 * The struct and the helper functions in this file are here for convenience
 * when building AI interfaces. The SSkirmishAISpecifier struct can be of use
 * as key type for C++ maps or C hash maps (eg. to cache loaded Skirmish AIs).
 * Engine side, we are using the C++ class SkirmishAIKey for the same purposes.
 */

#ifndef _SSKIRMISHAISPECIFIER_H
#define _SSKIRMISHAISPECIFIER_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * @brief struct Skirmish Artificial Intelligence specifier
 */
struct SSkirmishAISpecifier {
	const char* shortName; // [may not contain: spaces, '_', '#']
	const char* version;   // [may not contain: spaces, '_', '#']
};

int SSkirmishAISpecifier_hash(
	const struct SSkirmishAISpecifier* const spec
);
		
int SSkirmishAISpecifier_compare(
	const struct SSkirmishAISpecifier* const specThis,
	const struct SSkirmishAISpecifier* const specThat
);
		
		

#if defined __cplusplus
struct SSkirmishAISpecifier_Comparator {
	/**
	 * The key comparison function, a Strict Weak Ordering;
	 * it returns true if its first argument is less
	 * than its second argument, and false otherwise.
	 * This is also defined as map::key_compare.
	 */
	bool operator()(
		const struct SSkirmishAISpecifier& specThis,
		const struct SSkirmishAISpecifier& specThat
	) const;
};
#endif // defined __cplusplus

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _SSKIRMISHAISPECIFIER_H

