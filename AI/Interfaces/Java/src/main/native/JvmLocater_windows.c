/*
	Copyright (c) 2009 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#if defined _WIN32

#include "JvmLocater_common.h"

#include "System/maindefines.h"

#include <windows.h>
#include <wtypes.h>
#include <commctrl.h>


#define JVM_LIB "jvm.dll"

#define JAVA_LIB "java.dll"

/*
 * Returns the arch path, to get the current arch use the
 * macro GetArch, nbits here is ignored for now.
 */
const char* GetArchPath()
{
#ifdef _M_AMD64
	return "amd64";
#elif defined(_M_IA64)
	return "ia64";
#else
	return "i386";
#endif
}

/*
 * Given a JRE location and a JVM type, construct what the name the
 * JVM shared library will be.  Return true, if such a library
 * exists, false otherwise.
 */
bool GetJVMPath(const char* jrePath, const char* jvmType,
		char* jvmPath, size_t jvmPathSize, const char* arch) 
{
	if (arch == NULL) {
		arch = GetArchPath();
	}

	if ((*jvmType == '/') || (*jvmType == '\\')) {
		SNPRINTF(jvmPath, jvmPathSize, "%s\\"JVM_LIB, jvmType);
	} else {
		SNPRINTF(jvmPath, jvmPathSize, "%s\\bin\\%s\\"JVM_LIB, jrePath,
				jvmType);
	}

	return FileExists(jvmPath);
}

static bool CheckIfJREPath(const char* path, const char* arch)
{
	char libJava[MAXPATHLEN];
	bool found = false;

	if (path != NULL) {
		// Is path a JRE path?
		SNPRINTF(libJava, MAXPATHLEN, "%s\\bin\\"JAVA_LIB, path);
		if (FileExists(libJava)) {
			found = true;
		}
	}

	return found;
}

bool GetJREPathFromBase(char* path, size_t pathSize, const char* basePath,
		const char* arch)
{
	char jrePath[MAXPATHLEN];
	bool found = false;

	if (basePath != NULL) {
	//if (GetApplicationHome(path, pathSize)) {
		// Is basePath a JRE path?
		STRCPYS(jrePath, MAXPATHLEN, basePath);
		if (CheckIfJREPath(jrePath, arch)) {
			STRCPYS(path, pathSize, basePath);
			found = true;
		}

		// Is basePath/jre a JRE path?
		STRCATS(jrePath, MAXPATHLEN, "\\jre");
		if (CheckIfJREPath(jrePath, arch)) {
			STRCPYS(path, pathSize, jrePath);
			found = true;
		}
	}

	return found;
}

/*
 * Helpers to look in the registry for a public JRE.
 */

/* Same for 1.5.0, 1.5.1, 1.5.2 etc. */
#define JRE_REG_KEY "Software\\JavaSoft\\Java Runtime Environment"

static bool GetStringFromRegistry(HKEY key, const char* name, char* buf,
		size_t bufSize)
{
	DWORD type, size;

	if (RegQueryValueEx(key, name, 0, &type, 0, &size) == 0
		&& type == REG_SZ
		&& (size < bufSize)) {
		if (RegQueryValueEx(key, name, 0, 0, (LPBYTE)buf, &size) == 0) {
			return true;
		}
	}
	return false;
}

static bool GetJREPathFromRegistry(char* path, size_t pathSize, const char* arch)
{
	HKEY key, subkey;
	char version[MAXPATHLEN];

	/*
	 * Note: There is a very similar implementation of the following
	 * registry reading code in the Windows java control panel (javacp.cpl).
	 * If there are bugs here, a similar bug probably exists there.  Hence,
	 * changes here require inspection there.
	 */

	// Find the current version of the JRE
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, JRE_REG_KEY, 0, KEY_READ, &key) != 0) {
		return false;
	}

	if (!GetStringFromRegistry(key, "CurrentVersion",
			version, sizeof(version))) {
		RegCloseKey(key);
		return false;
	}

	/*if (strcmp(version, wantedVersion) != 0) {
		RegCloseKey(key);
		return false;
	}*/

	// Find directory where the current version is installed.
	if (RegOpenKeyEx(key, version, 0, KEY_READ, &subkey) != 0) {
		RegCloseKey(key);
		return false;
	}

	if (!GetStringFromRegistry(subkey, "JavaHome", path, pathSize)) {
		RegCloseKey(key);
		RegCloseKey(subkey);
		return false;
	}

	RegCloseKey(key);
	RegCloseKey(subkey);

	simpleLog_logL(SIMPLELOG_LEVEL_FINER, "JRE found in registry!");
	return true;
}

bool GetJREPathOSSpecific(char* path, size_t pathSize, const char* arch)
{
	bool found = false;

	// check if a JRE is specified in the registry
	if (!found) {
		found = GetJREPathFromRegistry(path, pathSize, arch);
	}

	return found;
}

#endif // defined _WIN32
