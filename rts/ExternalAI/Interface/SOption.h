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

// The structs in this files relate to *Options.lua files
// They are used for Mods and Skirmish AIs eg

#ifndef _SOPTION_H
#define	_SOPTION_H

#ifdef	__cplusplus
extern "C" {
#endif

struct OptionListItem {
	const char* key;
	const char* name;
	const char* desc;
};

enum OptionType {
	opt_error   = 0,
	opt_bool    = 1,
	opt_list    = 2,
	opt_number  = 3,
	opt_string  = 4,
	opt_section = 5
};

struct Option {
#ifdef	__cplusplus
	Option() : typeCode(opt_error) {}
#endif

	const char* key;
	const char* name;
	const char* desc;
	const char* section;
	const char* style;

	const char* type; // "bool", "number", "string", "list", ... (see enum OptionType)

	enum OptionType typeCode;

//	bool   boolDef;
	int   boolDef; // 0 -> false, 1 -> true

	float  numberDef;
	float  numberMin;
	float  numberMax;
	float  numberStep; // aligned to numberDef

	const char* stringDef;
	int    stringMaxLen;

	const char* listDef;
	int numListItems;
	struct OptionListItem* list;
};


#if	defined(__cplusplus) && !defined(BUILDING_AI) && !defined(BUILDING_AI_INTERFACE)
int ParseOptions(
		const char* fileName,
		const char* fileModes,
		const char* accessModes,
		const char* mapName,
		Option options[], unsigned int max);
#endif	// defined(__cplusplus) && !defined(BUILDING_AI) && !defined(BUILDING_AI_INTERFACE)

#ifdef	__cplusplus
}	// extern "C"
#endif

#endif	// _SOPTION_H */

