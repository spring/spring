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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>	// bool, true, false

#ifdef _MSC_VER
	// Microsoft Visual C++ 7.1: MSC_VER = 1310
	// Microsoft Visual C++ 7.0: MSC_VER = 1300
	#if _MSC_VER > 1310 // >= Visual Studio 2005
		#define PRINT print_s
		#define PRINTF printf_s
		#define FPRINT fprint_s
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
		#define FPRINT _fprint
		#define FPRINTF _fprintf
		#define SNPRINTF _snprintf
		#define VSNPRINTF _vsnprintf
		#define STRCPY strcpy
		#define STRCAT strcat
		#define STRNCAT strncat
		#define FOPEN fopen
	#endif
	#define STRCASECMP stricmp
#else // _MSC_VER
	// assuming GCC
	#define PRINT print
	#define PRINTF printf
	#define FPRINT fprint
	#define FPRINTF fprintf
	#define SNPRINTF snprintf
	#define VSNPRINTF vsnprintf
	#define STRCPY strcpy
	#define STRCAT strcat
	#define STRNCAT strncat
	#define FOPEN fopen
	#define STRCASECMP strcasecmp
#endif // _MSC_VER

/**
 * Stores a C [string, string] properties map,
 * for later retreival of values.
 * The data-dirs can be left empty by supplying these values:
 * dataDirsSize = 0, dataDirs = NULL
 *
 * @see util_getMyInfo()
 * @see util_getDataDirVersioned()
 * @see util_getDataDirUnversioned()
 */
void util_setMyInfo(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues,
		unsigned int dataDirsSize,
		const char** dataDirs);
const char* util_getMyInfo(const char* key);

#if defined BUILDING_AI
/**
 * Returns the data-dir from the stored infos.
 * Example (versioned):
 *     "~/.spring/AI/Skirmish/RAI/0.1"
 * Example (unversioned):
 *     "~/.spring/AI/Skirmish/RAI"
 *
 * You have to initialize before using this function,
 * through calling the function util_setMyInfo() wiht your
 * AI or Interface Info.
 * @see util_setMyInfo()
 *
 * @param  versioned  This can, for example, be used for storing cache that
 *                    is version independand.
 * @param  writeable  if set to true, the returned data-dir
 *                    is guaranteed to be writeable,
 *                    but may has to be created first
 */
//const char* util_getDataDir(bool versioned, bool writeable);

/**
 * Finds a file or directory in the data-dir(s), located at the relativePath,
 * and saves the resulting path in absolutePath.
 *
 * @param  relativePath  what to look for, eg. "log/interface-log.txt"
 * @param  absolutePath  will contain the result, eg.:
 *                       "/home/user/.spring/AI/Interfaces/C/log/interface-log.txt"
 * @param  searchOnlyWriteable  will only search in writable data-dirs
 * @param  createParent  will try to create the parent dir recursively
 * @param  createAsDir  will try to create the path as a dir recursively
 * @return  true if the file exists, the parent dir or the dir was created
 */
bool util_dataDirs_findFile(const char* relativePath, char* absolutePath,
		bool searchOnlyWriteable, bool createParent, bool createAsDir);

/**
 * Finds a file in the data-dir(s), located at the relativePath,
 * and saves the resulting path in absolutePath.
 *
 * @param  relativePath  what to look for, eg. "log/interface-log.txt"
 * @param  forWrite  whether we need a writable file
 * @return  the result, eg.
 *          "/home/user/.spring/AI/Interfaces/C/log/interface-log.txt",
 *          or NULL, if:
 *             (forWrite == false, and the file could not be found)
 *             or
 *             (forWrite == true, and the parent-dir does not exist
 *             and could not be created)
 */
char* util_dataDirs_allocFilePath(const char* relativePath, bool forWrite);
/**
 * Finds a dir in the data-dir(s), located at the relativePath,
 * and saves the resulting path in absolutePath.
 *
 * @param  relativePath  what to look for, eg. "jlib"
 * @param  forWrite  whether we need a writable dir
 * @return  the result, eg.
 *          "/home/user/.spring/AI/Interfaces/Java/jlib",
 *          or NULL, if:
 *             (forWrite == false, and the dir could not be found)
 *             or
 *             (forWrite == true, and the dir does not exist
 *             and could not be created)
 */
char* util_dataDirs_allocDir(const char* relativePath, bool forWrite);
#endif // defined BUILDING_AI

/**
 * Allocates fresh memory for storing a C string of the specified length.
 */
char* util_allocStr(unsigned int length);

/**
 * Allocates fresh memory and copies the supplied string into it.
 * @return  the freshly allocated memory containing the string,
 *          or NULL, if the arguemtn was NULL.
 */
char* util_allocStrCpy(const char* toCopy);

/**
 * Allocates fresh memory and copies a part of the supplied string into it.
 */
char* util_allocStrSubCpy(const char* toCopy, int fromPos, int toPos);
/**
 * Allocates fresh memory and copies a part of the supplied string into it.
 */
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

bool util_startsWith(const char* str, const char* prefix);
bool util_endsWith(const char* str, const char* suffix);

bool util_strToBool(const char* str);

/**
 * Lists all files found below a directory.
 *
 * Example:
 * char* foundFiles[64];
 * (".../AI/Interface/Java/0.1/jlib/", ".jar", foundFiles, true, 64)
 *    ".../AI/Interface/Java/0.1/jlib/vecmath.jar"
 *    ".../AI/Interface/Java/0.1/jlib/jna/jna.jar"
 *    ".../AI/Interface/Java/0.1/jlib/jna/linux-i386.jar"
 *
 * @param  suffix  files to list have to end with this suffix, eg. ".jar"
 * @param  recursive  list files recursively
 */
unsigned int util_listFiles(const char* dir, const char* suffix,
		char** fileNames, bool recursive, const unsigned int maxFileNames);

/**
 * Returns true if the file specified in filePath exists.
 */
bool util_fileExists(const char* filePath);

/**
 * Creates the dir specified in dirPath, if its parent directory exists
 * and we have write access to it.
 *
 * @param  recursive  whether ancestor dirs shall be created aswell,
 *                    if they do not yet exist
 * @return true if the dir could be created or existed already.
 */
bool util_makeDir(const char* dirPath, bool recursive);

/**
 * Saves the parent directory of a file or dir specified in path
 * into parentPath.
 *
 * Example:
 * before: "/home/user/games/spring/AI/Skirmish/RAI/0.1/log.txt"
 * after:  "/home/user/games/spring/AI/Skirmish/RAI/0.1"
 */
bool util_getParentDir(char* path);

/**
 * Finds a file or directory under dirs with the relativeFilePath
 * and saves the resulting path in absoluteFilePath.
 * If searchOnlyWriteable is set, only the first entry in dirs
 * is used for the search, as it is assumed to contain
 * the writeable directory.
 *
 * @return  true if the file exists
 */
bool util_findFile(const char* dirs[], unsigned int numDirs,
		const char* relativeFilePath, char* absoluteFilePath,
		bool searchOnlyWriteable);

/**
 * Finds a directory under dirs with the relativeDirPath
 * and saves the resulting path in absoluteDirPath.
 * If searchOnlyWriteable is set, only the first entry in dirs
 * is used for the search, as it is assumed to contain
 * the writeable directory.
 *
 * @return  true if the file existed or was created
 */
bool util_findDir(const char* dirs[], unsigned int numDirs,
		const char* relativeDirPath, char* absoluteDirPath,
		bool searchOnlyWriteable, bool create);

/**
 * Parses a properties file into a C [string, string] map.
 * Lines that contian only white-spaces and lines beginning
 * with '#' or ';' are ignored.
 * Regex for a valid property:
 * ^[\w]*[^;#][^=]*=.+$
 */
int util_parsePropertiesFile(const char* propertiesFile,
		const char* keys[], const char* values[], int maxProperties);

/**
 * Find the value assigned to a key in a C [string, string] map.
 * @return  the assigned value or NULL if the key was not found
 */
const char* util_map_getValueByKey(
		unsigned int mapSize,
		const char** mapKeys, const char** mapValues,
		const char* key);

/**
 * Free memory.
 * Some of the other functions will not work anymore,
 * after calling this function.
 */
void util_finalize();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _INTERFACEUTIL_H
