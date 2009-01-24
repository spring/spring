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
		#define PRINT print_s
		#define PRINTF printf_s
		#define FPRINTF fprintf_s
		#define SNPRINTF sprintf_s
		#define VSNPRINTF vsprintf_s
		#define STRCPY strcpy_s
		#define STRCAT strcat_s
		#define STRNCAT strncat_s
		#define FOPEN fopen_s
	#else              // Visual Studio 2003
		#define PRINT _print
		#define PRINTF _printf
		#define FPRINTF _fprintf
		#define SNPRINTF _snprintf
		#define VSNPRINTF _vsnprintf
		#define STRCPY strcpy
		#define STRCAT strcat
		#define STRNCAT strncat
		#define FOPEN fopen
	#endif
	#define STRCASECMP stricmp
#else	// _MSC_VER
	// assuming GCC
	#define PRINT print
	#define PRINTF printf
	#define FPRINTF fprintf
	#define SNPRINTF snprintf
	#define VSNPRINTF vsnprintf
	#define STRCPY strcpy
	#define STRCAT strcat
	#define STRNCAT strncat
	#define FOPEN fopen
	#define STRCASECMP strcasecmp
#endif	// _MSC_VER

void util_setMyInfo(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues);
const char* util_getMyInfo(const char* key);

#if defined BUILDING_AI || defined BUILDING_AI_INTERFACE
/**
 * You have to initialize before using this function,
 * through calling the function util_setMyInfo() wiht your
 * AI or Interface Info.
 */
const char* util_getDataDirVersioned();
/**
 * You have to initialize before using this function,
 * through calling the function util_setMyInfo() wiht your
 * AI or Interface Info.
 */
const char* util_getDataDirUnversioned();
#endif

char* util_allocStr(unsigned int length);

char* util_allocStrCpy(const char* toCopy);

char* util_allocStrSubCpy(const char* toCopy, int fromPos, int toPos);
char* util_allocStrSubCpyByPointers(const char* toCopy,
		const char* fromPos, const char* toPos);

/**
 * Concatenates a variable number of C strings into newly allocated memory.
 * The number of strings to concatenate has to be supplied as first argument.
 */
char* util_allocStrCat(int numParts, const char* first, ...);
/**
 * Concatenates a variable number of C strings into newly allocated memory.
 * The last argument has to be NULL, as there would be no way to determine
 * the number of arguments otherwise.
 */
char* util_allocStrCat_nt(const char* first, ...);

/**
 * Concatenates a variable number of C strings,
 * which represent file system parts (eg. dirs or files)
 * into newly allocated memory.
 * The number of parts to concatenate has to be supplied as first argument.
 * sample call:
 * (3, "/home/user/tmp", "appName/", "/i/am/fine")
 * output:
 * "/home/user/tmp/appName/i/am/fine"
 */
char* util_allocStrCatFSPath(int numParts, const char* first, ...);

bool util_isWhiteSpace(char c);

void util_strLeftTrim(char* toTrim);
void util_strRightTrim(char* toTrim);
void util_strTrim(char* toTrim);
char* util_allocStrTrimed(const char* toTrim);

void util_strReplaceChar(char* toChange, char toFind, char replacer);
char* util_allocStrReplaceStr(const char* toChange, const char* toFind,
		const char* replacer);

bool util_endsWith(const char* str, const char* suffix);

bool util_strToBool(const char* str);

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

int util_parsePropertiesFile(const char* propertiesFile,
		const char* keys[], const char* values[], int maxProperties);

/**
 * Return NULL if the key was not found.
 */
const char* util_map_getValueByKey(
		unsigned int mapSize,
		const char** mapKeys, const char** mapValues,
		const char* key);

#ifdef	__cplusplus
}
#endif

#endif // _INTERFACEUTIL_H
