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

#ifndef _INTERFACEUTIL_H
#define _INTERFACEUTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>	// bool, true, false

#ifdef _MSC_VER
	// Microsoft Visual C++ 7.1: MSC_VER = 1310
	// Microsoft Visual C++ 7.0: MSC_VER = 1300
	#if _MSC_VER > 1310 // >= Visual Studio 2005
		#define PRINTF printf_s
		#define FPRINTF fprintf_s
		#define SNPRINTF sprintf_s
		#define VSNPRINTF vsprintf_s
		#define STRCPY strcpy_s
		#define STRCAT strcat_s
		#define FOPEN fopen_s
	#else              // Visual Studio 2003
		#define PRINTF _printf
		#define FPRINTF _fprintf
		#define SNPRINTF _snprintf
		#define VSNPRINTF _vsnprintf
		#define STRCPY strcpy
		#define STRCAT strcat
		#define FOPEN fopen
	#endif
	#define STRCASECMP stricmp
#else	// _MSC_VER
	// assuming GCC
	#define PRINTF printf
	#define FPRINTF fprintf
	#define SNPRINTF snprintf
	#define VSNPRINTF vsnprintf
	#define STRCPY strcpy
	#define STRCAT strcat
	#define FOPEN fopen
	#define STRCASECMP strcasecmp
#endif	// _MSC_VER

void util_setMyInfo(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues);
const char* util_getMyInfo(const char* key);

//void util_setDataDirs(const char* unversioned, const char* versioned);
const char* util_getDataDirVersioned();
const char* util_getDataDirUnversioned();

char* util_allocStr(unsigned int length);

char* util_allocStrCpy(const char* toCopy);

char* util_allocStrSubCpy(const char* toCopy, int fromPos, int toPos);
char* util_allocStrSubCpyByPointers(const char* toCopy,
		const char* fromPos, const char* toPos);

char* util_allocStrCpyCat(const char* toPart1, const char* toPart2);

void util_strReplace(char* toChange, char toFind, char replacer);

bool util_endsWith(const char* str, const char* suffix);

// suffix example: ".jar"
unsigned int util_listFiles(const char* dir, const char* suffix,
		char** fileNames, bool recursive, const unsigned int maxFileNames);

bool util_fileExists(const char* filePath);

bool util_makeDir(const char* dirPath);

bool util_makeDirRecursive(const char* dirPath);

bool util_getParentDir(const char* path, char* parentPath);

bool util_findFile(const char* dirs[], unsigned int numDirs,
		const char* relativeFilePath, char* absoluteFilePath);

bool util_findDir(const char* dirs[], unsigned int numDirs,
		const char* relativeDirPath, char* absoluteDirPath,
		bool searchOnlyWriteable, bool create);

/**
 * Return NULL if the key was not found.
 */
const char* util_map_getValueByKey(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues,
		const char* key);

#ifdef	__cplusplus
}
#endif

#endif // _INTERFACEUTIL_H
