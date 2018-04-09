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

#include <stdio.h>       // fgets()
#include <string.h>      // strcpy(), str...()
#include <stdlib.h>      // malloc(), calloc(), free()
#include <stdarg.h>      // var-args
#include <sys/stat.h>    // used for check if a file exists
#ifdef _WIN32
#include <io.h>          // needed for dir listing
#include <direct.h>      // mkdir()
#else // WIN32
#include <sys/stat.h>    // mkdir()
#include <sys/types.h>   // mkdir()
#include <dirent.h>      // needed for dir listing
#endif // WIN32

#include "Util.h"

#include "System/MainDefines.h"
#include "System/SafeCStrings.h"


char* util_allocStr(unsigned int length) {
	return (char*) calloc(length+1, sizeof(char));
}

char* util_allocStrCpy(const char* toCopy) {

	if (toCopy == NULL) {
		return NULL;
	}

	const size_t copy_sizeMax = strlen(toCopy) + 1;
	char* copy = (char*) calloc(copy_sizeMax, sizeof(char));
	STRCPY_T(copy, copy_sizeMax, toCopy);
	return copy;
}

char* util_allocStrSubCpy(const char* toCopy, int fromPos, int toPos) {

	if (toPos < 0) {
		toPos = strlen(toCopy);
	}

	if (fromPos < 0 || toPos < fromPos) {
		return NULL;
	}

	unsigned int newSize = toPos - fromPos;
	char* copy = (char*) calloc(newSize+1, sizeof(char));
	unsigned int i;
	for (i=0; i < newSize; ++i) {
		copy[i] = toCopy[fromPos + i];
	}

	return copy;
}

char* util_allocStrSubCpyByPointers(const char* toCopy,
		const char* fromPos, const char* toPos) {

	unsigned int newSize = toPos - fromPos;
	char* copy = (char*) calloc(newSize+1, sizeof(char));
	unsigned int i;
	for (i=0; i < newSize; ++i) {
		copy[i] = *(fromPos + i);
	}

	return copy;
}

// ANSI C requires at least one named parameter.
char* util_allocStrCat(int numParts, const char* first, ...) {

	va_list args;
	char *result, *p;
	const char* str;
	size_t length = 0;
	int si;

	// Find the total memory required.
	va_start(args, first);
	for (str = first, si = 0; si < numParts; ++si,
			((str) = (const char *) va_arg(args, const char*))) {
		length += strlen(str);
	}
	va_end(args);

	// Create the string.  Doing the copy ourselves avoids useless string
	// traversals of result, if using strcat(), or string, if using strlen()
	// to increment a pointer into result, at the cost of losing the
	// native optimization of strcat() if any.
	result = util_allocStr(length);
	p = result;
	va_start(args, first);
	for (str = first, si = 0; si < numParts; ++si,
			((str) = (const char *) va_arg(args, const char*))) {
		while (*str != '\0') {
			*p++ = *str++;
		}
	}
	va_end(args);
	*p = '\0';

	return result;
}
// ANSI C requires at least one named parameter.
char* util_allocStrCat_nt(const char* first, ...)
{
	va_list args;
	char *result, *p;
	const char* str;
	size_t length = 0;

	// Find the total memory required.
	va_start(args, first);
	for (str = first; str != NULL;
			((str) = (const char *) va_arg(args, const char*))) {
		length += strlen(str);
	}
	va_end(args);

	// Create the string.  Doing the copy ourselves avoids useless string
	// traversals of result, if using strcat(), or string, if using strlen()
	// to increment a pointer into result, at the cost of losing the
	//native optimization of strcat() if any.
	result = util_allocStr(length);
	p = result;
	va_start(args, first);
	for (str = first; str != NULL;
			((str) = (const char *) va_arg(args, const char*))) {
		while (*str != '\0') {
			*p++ = *str++;
		}
	}
	va_end(args);
	*p = '\0';

	return result;
}
char* util_allocStrCatFSPath(int numParts, const char* first, ...) {

	va_list args;
	char *result, *p;
	const char* str;
	size_t length = 0;
	int si;

	// Find the maximum of the total memory required.
	va_start(args, first);
	for (str = first, si = 0; si < numParts; ++si,
			((str) = (const char *) va_arg(args, const char*))) {
		length += strlen(str);
	}
	va_end(args);
	length += numParts;

	// Create the string.  Doing the copy ourselves avoids useless string
	// traversals of result, if using strcat(), or string, if using strlen()
	// to increment a pointer into result, at the cost of losing the
	// native optimization of strcat() if any.
	result = util_allocStr(length);
	p = result;
	char thisChar;
	char lastChar = '\0';
	va_start(args, first);
	for (str = first, si = 0; si < numParts; ++si,
			((str) = (const char *) va_arg(args, const char*))) {
		if (si > 0 && lastChar != '/') {
			*p++ = '/';
			lastChar = '/';
		}
		while (*str != '\0') {
			thisChar = *str;
			if (thisChar == '\\') {
				thisChar = '/';
			}
			if ((thisChar != '/') || (thisChar != lastChar)) {
				*p = thisChar;
				lastChar = thisChar;
				p++;
			}
			str++;
		}
	}
	va_end(args);
	*p = '\0';
	if (cPS != '/') {
		util_strReplaceChar(result, '/', cPS);
	}

	return result;
}

// 0x20 - space
// 0x09 - horizontal tab
// 0x0a - linefeed
// 0x0b - vertical tab
// 0x0c - form feed
// 0x0d - carriage return
static const char whiteSpaces[] = {0x20, 0x09, 0x0a, 0x0b, 0x0c, 0x0d};
static const int whiteSpaces_size = 6;

bool util_isWhiteSpace(char c) {

	int i;
	for (i=0; i < whiteSpaces_size; ++i) {
		if (c == whiteSpaces[i]) {
			return true;
		}
	}
	return false;
}

void util_strLeftTrim(char* toTrim) {

	if (toTrim == NULL) {
		return;
	}

	int len = strlen(toTrim);
	char* first = toTrim;
	int num = 0;

	// find leading white spaces
	while (*first != '\0') {
		if (util_isWhiteSpace(*first)) {
			first++;
			num++;
			continue;
		}
		break;
	}

	// trim
	if (num > 0) {
		int i;
		// back stack string chars; '\0' inclusive
		for (i=0; i+num <= len; ++i) {
			toTrim[i] = toTrim[i+num];
		}
	}
}
void util_strRightTrim(char* toTrim) {

	if (toTrim == NULL) {
		return;
	}

	int len = strlen(toTrim);
	char* last = toTrim - 1 + len;

	// find trailing white spaces
	while (last > toTrim) {
		if (util_isWhiteSpace(*last)) {
			last--;
			continue;
		}
		break;
	}

	// trim
	*(last+1) = '\0';
}
void util_strTrim(char* toTrim) {

	util_strLeftTrim(toTrim);
	util_strRightTrim(toTrim);
}
char* util_allocStrTrimed(const char* toTrim) {

	char* strCpy = util_allocStrCpy(toTrim);
	util_strTrim(strCpy);
	return strCpy;

}

void util_strReplaceChar(char* toChange, char toFind, char replacer) {

	const unsigned int len = strlen(toChange);
	unsigned int i;
	for (i = 0; i < len; i++) {
		if (toChange[i] == toFind) {
			toChange[i] = replacer;
		}
	}
}
char* util_allocStrReplaceStr(const char* toChange, const char* toFind,
		const char* replacer) {

	// evaluate number of occurences of toFind in toChange
	unsigned int numFinds = 0;
	const char* found = NULL;
	for (found = strstr(toChange, toFind); found != NULL; numFinds++) {
		found = strstr(found+1, toFind);
	}

	int toChange_len = strlen(toChange);
	int toFind_len = strlen(toFind);
	int replacer_len = strlen(replacer);

	// evaluate the result strings length
	unsigned int result_len =
			toChange_len + (numFinds * (replacer_len - toFind_len));
	char* result = util_allocStr(result_len);
	result[0] = '\0';

	const char* endLastFound = toChange;
	for (found = strstr(endLastFound, toFind); found != NULL; ) {
		STRNCAT(result, endLastFound, (found - toChange));
		STRCAT(result, replacer);
		endLastFound = found + toFind_len;
		found = strstr(found+1, toFind);
	}
	STRNCAT(result, endLastFound, (toChange + toChange_len - endLastFound));

	return result;
}

bool util_startsWith(const char* str, const char* prefix) {

	bool startsWith = false;

	const unsigned int l_str = strlen(str);
	const unsigned int l_prefix = strlen(prefix);

	if (l_str >= l_prefix) {
		startsWith = true;

		unsigned int i;
		for (i=0; i < l_prefix; ++i) {
			if (str[i] != prefix[i]) {
				startsWith = false;
				break;
			}
		}
	}

	return startsWith;
}
bool util_endsWith(const char* str, const char* suffix) {

	bool endsWith = false;

	const unsigned int l_str = strlen(str);
	const unsigned int l_suffix = strlen(suffix);

	if (l_str >= l_suffix) {
		endsWith = true;

		unsigned int i;
		for (i = 1; i <= l_suffix; ++i) {
			if (str[l_str - i] != suffix[l_suffix - i]) {
				endsWith = false;
				break;
			}
		}
	}

	return endsWith;
}

bool util_strToBool(const char* str) {

	if (str == NULL) {
		return false;
	}

	bool res = true;

	char* strT = util_allocStrTrimed(str);
	if (strcmp(strT, "0") == 0
			|| strcmp(strT, "NO") == 0
			|| strcmp(strT, "No") == 0
			|| strcmp(strT, "no") == 0
			|| strcmp(strT, "n") == 0
			|| strcmp(strT, "N") == 0
			|| strcmp(strT, "FALSE") == 0
			|| strcmp(strT, "False") == 0
			|| strcmp(strT, "false") == 0
			|| strcmp(strT, "f") == 0
			|| strcmp(strT, "F") == 0)
	{
		res = false;
	}

	free(strT);
	return res;
}


#if defined WIN32
static bool util_isFile(const struct _finddata_t* fileInfo) {
	return !(fileInfo->attrib & _A_SUBDIR)
			&& fileInfo->attrib & (_A_NORMAL | _A_HIDDEN | _A_ARCH);
}
static bool util_isNormalDir(const struct _finddata_t* fileInfo) {
	return strcmp(fileInfo->name, ".") != 0
			&& strcmp(fileInfo->name, "..") != 0
			&& (fileInfo->attrib & _A_SUBDIR);
}
static unsigned int util_listFilesRec(const char* dir, const char* suffix,
		char** fileNames, bool recursive, const unsigned int maxFileNames,
		unsigned int numFileNames, const char* relPath) {

	if (numFileNames >= maxFileNames) {
		return numFileNames;
	}

	struct _finddata_t fileInfo;
	int handle;

	// look for files which end in: suffix
	char* suffixFilesSpec = util_allocStrCat(3, dir, "\\*", suffix);
	handle = _findfirst(suffixFilesSpec, &fileInfo);
	if (handle != -1L) {
		if (util_isFile(&fileInfo)) {
			fileNames[numFileNames++] = util_allocStrCpy(fileInfo.name);
		}
		while (_findnext(handle, &fileInfo) == 0
				&& numFileNames < maxFileNames) {
			if (util_isFile(&fileInfo)) {
				fileNames[numFileNames++] = util_allocStrCat(2, relPath, fileInfo.name);
			}
		}
		_findclose(handle);
	}
	free(suffixFilesSpec);

	// search in sub-directories
	if (recursive) {
		char* subDirsSpec = util_allocStrCat(2, dir, "\\*.*");
		handle = _findfirst(subDirsSpec, &fileInfo);
		if (handle != -1L) {
			// check if not current or parent directories
			if (util_isNormalDir(&fileInfo)) {
				char* subDir = util_allocStrCat(3, dir, "\\", fileInfo.name);
				char* subRelPath = util_allocStrCat(3, relPath, fileInfo.name, "\\");
				numFileNames = util_listFilesRec(subDir, suffix, fileNames,
						recursive, maxFileNames, numFileNames, subRelPath);
				free(subDir);
				free(subRelPath);
			}
			while (_findnext(handle, &fileInfo) == 0
					&& numFileNames < maxFileNames) {
				if (util_isNormalDir(&fileInfo)) {
					char* subDir = util_allocStrCat(3, dir, "\\", fileInfo.name);
					char* subRelPath = util_allocStrCat(3, relPath, fileInfo.name, "\\");
					numFileNames = util_listFilesRec(subDir, suffix, fileNames,
							recursive, maxFileNames, numFileNames, subRelPath);
					free(subDir);
					free(subRelPath);
				}
			}
			_findclose(handle);
		}
		free(subDirsSpec);
	}

	return numFileNames;
}
unsigned int util_listFiles(const char* dir, const char* suffix,
		char** fileNames, bool recursive, const unsigned int maxFileNames) {
	return util_listFilesRec(
			dir, suffix, fileNames, recursive, maxFileNames, 0, "");
}
#else // defined WIN32

static const char* fileSelectorSuffix = NULL;

static void util_initFileSelector(const char* suffix) {
	fileSelectorSuffix = suffix;
}

#if defined(__APPLE__)
static int util_fileSelector(struct dirent* fileDesc) {
#else
static int util_fileSelector(const struct dirent* fileDesc) {
#endif
	return util_endsWith(fileDesc->d_name, fileSelectorSuffix);
}

static unsigned int util_listFilesU(const char* dir, struct dirent*** files) {

	int foundDirs = scandir(dir, files, &util_fileSelector, alphasort);

	if (foundDirs < 0) { // error, act as if no file found
		foundDirs = 0;
	}

	return (unsigned int) foundDirs;
}

static unsigned int util_listFilesRec(const char* dir, const char* suffix,
		char** fileNames, bool recursive, const unsigned int maxFileNames,
		unsigned int numFiles, const char* relPath) {

	struct dirent** files;
	util_initFileSelector(suffix);
	unsigned int currentNumFiles = util_listFilesU(dir, &files);
	unsigned int f;
	for (f = 0; f < currentNumFiles && numFiles < maxFileNames; ++f) {
		fileNames[numFiles++] = util_allocStrCat(2, relPath, files[f]->d_name);
	}
/*
	for (; f < currentNumFiles; ++f) {
		free(files[f]);
	}
*/

	if (recursive) {
		struct stat dirStat;
		util_initFileSelector("");
		currentNumFiles = util_listFilesU(dir, &files);
		for (f = 0; f < currentNumFiles && numFiles < maxFileNames; ++f) {
			if (strcmp(files[f]->d_name, ".") == 0
					|| strcmp(files[f]->d_name, "..") == 0) {
				continue;
			}
			char* subDir = util_allocStrCat(4, dir, "/", relPath, files[f]->d_name);
			char* subRelPath = util_allocStrCat(3, relPath, files[f]->d_name, "/");
			int retStat = stat(subDir, &dirStat);
			if (retStat == 0 && S_ISDIR(dirStat.st_mode)) {
				numFiles = util_listFilesRec(subDir, suffix, fileNames,
						recursive, maxFileNames, numFiles, subRelPath);
			}
			free(subDir);
			free(subRelPath);
		}
	}

	return numFiles;
}
unsigned int util_listFiles(const char* dir, const char* suffix,
		char** fileNames, bool recursive, const unsigned int maxFileNames) {
	return util_listFilesRec(
			dir, suffix, fileNames, recursive, maxFileNames, 0, "");
}
#endif // defined WIN32

void util_removeTrailingSlash(char* fsPath) {

	if (fsPath != NULL && strlen(fsPath)) {
		int len = strlen(fsPath);
		if (len > 0) {
			char lastChar = fsPath[len-1];
			if ((lastChar == '/') || (lastChar == '\\')) {
				fsPath[len-1] = '\0';
			}
		}
	}
}

bool util_fileExists(const char* filePath) {

	struct stat fileInfo;
	bool exists;
	int intStat;

	char* filePath_cpy = util_allocStrCpy(filePath);
	util_removeTrailingSlash(filePath_cpy);

	// Attempt to get the file attributes
	intStat = stat(filePath_cpy, &fileInfo);
	if (intStat == 0) {
		// We were able to get the file attributes
		// so the file obviously exists.
		exists = true;
	} else {
		// We were not able to get the file attributes.
		// This may mean that we don't have permission to
		// access the folder which contains this file. If you
		// need to do that level of checking, lookup the
		// return values of stat which will give you
		// more details on why stat failed.
		exists = false;
	}
	free(filePath_cpy);

	return exists;
}

static bool util_makeDirS(const char* dirPath) {

	#if defined WIN32
		int mkStat = _mkdir(dirPath);
		if (mkStat == 0) {
			return true;
		} else {
			return false;
		}
	#else // defined WIN32
		// with read/write/search permissions for owner and group,
		// and with read/search permissions for others
		int mkStat = mkdir(dirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (mkStat == 0) {
			return true;
		} else {
			return false;
		}
	#endif // defined WIN32
}

bool util_makeDir(const char* dirPath, bool recursive) {

	char* dirPath_cpy = util_allocStrCpy(dirPath);
	util_removeTrailingSlash(dirPath_cpy);

	bool exists = util_fileExists(dirPath_cpy);

	if (!exists) {
		char* parentDir = util_allocStrCpy(dirPath_cpy);
		bool evaluatedParent = util_getParentDir(parentDir);
		if (evaluatedParent) {
			bool parentExists = util_fileExists(parentDir);

			if (!parentExists && recursive) {
				parentExists = util_makeDir(parentDir, recursive);
			}

			if (parentExists) {
				exists = util_makeDirS(dirPath_cpy);
			}
		}
		free(parentDir);
		parentDir = NULL;
	}
	free(dirPath_cpy);

	return exists;
}

bool util_getParentDir(char* path) {
	if (path == NULL)
		return false;
	util_removeTrailingSlash(path);

	char* ptr = strrchr(path, '/'); // search char from end reverse
	if (ptr == NULL) {
		ptr = strrchr(path, '\\'); // search char from end reverse
		if (ptr == NULL) {
			return false;
		}
	}

	// replace the last '/' or '\\' with an string terminationg char
	*ptr = '\0';

	return true;
}

bool util_isPathAbsolute(const char* path) {

	if (path == NULL) {
		return false;
	}

#ifdef _WIN32
	return strlen(path) > 2 && path[1] == ':' && (path[2] == '\\' || path[2] == '/');
#else // _WIN32
	return strlen(path) > 0 && path[0] == '/';
#endif // _WIN32
}

bool util_findFile(const char* dirs[], unsigned int numDirs,
		const char* relativeFilePath, char* absoluteFilePath,
		bool searchOnlyWriteable) {

	bool found = false;

	if (util_isPathAbsolute(relativeFilePath)) {
		STRCPY(absoluteFilePath, relativeFilePath);
		found = true;
	}

	if (!found && searchOnlyWriteable && numDirs > 1) {
		numDirs = 1;
	}

	unsigned int d;
	for (d=0; d < numDirs && !found; ++d) {
		char* tmpPath = util_allocStrCatFSPath(2, dirs[d], relativeFilePath);

		if (util_fileExists(tmpPath)) {
			STRCPY(absoluteFilePath, tmpPath);
			found = true;
		}

		free(tmpPath);
		tmpPath = NULL;
	}

	return found;
}

bool util_findDir(const char* dirs[], unsigned int numDirs,
		const char* relativeDirPath, char* absoluteDirPath,
		bool searchOnlyWriteable, bool create) {

	bool found = false;

	// check if it is an absolute file path
	if (util_fileExists(relativeDirPath)) {
		STRCPY(absoluteDirPath, relativeDirPath);
		found = true;
	}

	if (searchOnlyWriteable && numDirs > 1) {
		numDirs = 1;
	}

	unsigned int d;
	for (d=0; d < numDirs && !found; ++d) {
		char* tmpPath = util_allocStrCatFSPath(2, dirs[d], relativeDirPath);

		if (util_fileExists(tmpPath)) {
			STRCPY(absoluteDirPath, tmpPath);
			found = true;
		}

		free(tmpPath);
		tmpPath = NULL;
	}

	// not found -> create it
	if (!found && create && numDirs >= 1) {
		// use dirs[0], as it is assumed this is the writable dir
		char* tmpPath = util_allocStrCatFSPath(2, dirs[0], relativeDirPath);
		STRCPY(absoluteDirPath, tmpPath);
		free(tmpPath);
		tmpPath = NULL;
		found = util_makeDir(absoluteDirPath, true);
	}

	return found;
}

static inline bool util_isEndOfLine(char c) {
	return (c == '\r') || (c == '\n');
}
/**
 * Saves the property found in propLine into the arrays at the given index.
 * @return  false if no property was found at this line
 */
static bool util_parseProperty(const char* propLine,
		const char* keys[], const char* values[], int propStoreIndex) {

	const unsigned int len = strlen(propLine);
	unsigned int pos = 0; // current pos
	char c; // current char

	// skip white spaces
	while (pos < len) {
		c = propLine[pos];
		if (util_isWhiteSpace(c)) {
			pos++;
		} else {
			break;
		}
	}

	// return, if the line was empty (contained only white spaces)
	if (pos >= len) {
		return false;
	}

	// check if it is a comment line
	if (pos < len) {
		c = propLine[pos];
		if ((c == '#') || (c == ';')) {
			return false;
		}
	}

	// now it has to be a property line
	// if not: bad properties file format

	// read the key
	int keyStartPos = pos;
	char* key = NULL;
	while (pos < len) {
		c = propLine[pos];
		bool wb = util_isWhiteSpace(c);
		bool es = (c == '=');
		if (!wb && !es) {
			pos++;
		} else {
			break;
		}
	}
	key = util_allocStrSubCpy(propLine, keyStartPos, pos);

	// skip white spaces
	while (pos < len) {
		c = propLine[pos];
		if (util_isWhiteSpace(c)) {
			pos++;
		} else {
			break;
		}
	}

	// check if it is an equals sign: '='
	if (pos < len) {
		c = propLine[pos];
		if (c != '=') {
			free(key);
			key = NULL;
			return false;
		}
		pos++;
	} else {
		free(key);
		key = NULL;
		return false;
	}

	// skip white spaces
	while (pos < len) {
		c = propLine[pos];
		if (util_isWhiteSpace(c)) {
			pos++;
		} else {
			break;
		}
	}

	// read the value
	int valueStartPos = pos;
	char* value = NULL;
	while (pos < len) {
		c = propLine[pos];
		bool eol = util_isEndOfLine(c);
		if (!eol) {
			pos++;
		} else {
			break;
		}
	}
	value = util_allocStrSubCpy(propLine, valueStartPos, pos);

	keys[propStoreIndex] = key;
	values[propStoreIndex] = value;

	return true;
}

#define MAXLINELENGTH 2048
int util_parsePropertiesFile(const char* propertiesFile,
		const char* keys[], const char* values[], int maxProperties) {

	int numProperties = 0;

	FILE* file;
	char line[MAXLINELENGTH + 1];

	file = fopen(propertiesFile , "r");
	if (file == NULL) {
		return numProperties;
	}

	char* error = NULL;
	bool propertyFound = false;
	while (numProperties < maxProperties) {
		// returns NULL on error or EOF
		error = fgets(line, MAXLINELENGTH + 1, file);
		if (error == NULL) {
			break;
		}
		// remove EOL chars
		int lci = strlen(line)-1;
		while (lci >= 0 && util_isEndOfLine(line[lci])) {
			line[lci] = '\0';
			lci--;
		}
		propertyFound = util_parseProperty(line, keys, values, numProperties);
		if (propertyFound) {
			numProperties++;
		}
	}
	fclose(file);

	return numProperties;
}

const char* util_map_getValueByKey(
		unsigned int mapSize,
		const char** mapKeys, const char** mapValues,
		const char* key) {

	const char* value = NULL;

	unsigned int i;
	for (i = 0; i < mapSize; i++) {
		if (strcmp(mapKeys[i], key) == 0) {
			value = mapValues[i];
			break;
		}
	}

	return value;
}

