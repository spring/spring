/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * Glob conversion by Chris Han (based on work by Nathaniel Smith).
 */
#ifdef _MSC_VER
#include "System/Platform/Win/win32.h"
#endif
#include "FileSystem.h"

#include "System/Log/ILog.h"
#include "System/Util.h"
#include "System/mmgr.h"

#include <boost/regex.hpp>

////////////////////////////////////////
////////// FileSystem

/**
 * @brief quote macro
 * @param c Character to test
 * @param str string currently being built
 *
 * Given an std::string str that we are assembling,
 * and an upcoming char c, will append
 * an extra '\\' to quote the character if necessary.
 * The do-while is used for legalizing the ';' in "QUOTE(c, regex);".
 */
#define QUOTE(c,str)			\
	do {					\
		if (!(isalnum(c) || (c) == '_'))	\
			str += '\\';		\
		str += c;				\
	} while (0)

std::string FileSystem::ConvertGlobToRegex(const std::string& glob)
{
	std::string regex;
	regex.reserve(glob.size()<<1);
	int braces = 0;
	for (std::string::const_iterator i = glob.begin(); i != glob.end(); ++i) {
		char c = *i;
#ifdef DEBUG
		if (braces >= 5) {
			LOG_L(L_WARNING, "%s: braces nested too deeply\n%s", __FUNCTION__, glob.c_str());
		}
#endif
		switch (c) {
			case '*':
				regex += ".*";
				break;
			case '?':
				regex += '.';
				break;
			case '{':
				braces++;
				regex += '(';
				break;
			case '}':
#ifdef DEBUG
				if (braces == 0) {
					LOG_L(L_WARNING, "%s: closing brace without an equivalent opening brace\n%s", __FUNCTION__, glob.c_str());
				}
#endif
				regex += ')';
				braces--;
				break;
			case ',':
				if (braces > 0) {
					regex += '|';
				} else {
					QUOTE(c,regex);
				}
				break;
			case '\\':
				++i;
#ifdef DEBUG
				if (i == glob.end()) {
					LOG_L(L_WARNING, "%s: pattern ends with backslash\n%s", __FUNCTION__, glob.c_str());
				}
#endif
				QUOTE(*i,regex);
				break;
			default:
				QUOTE(c,regex);
				break;
		}
	}

#ifdef DEBUG
	if (braces > 0) {
		LOG_L(L_WARNING, "%s: unterminated brace expression\n%s", __FUNCTION__, glob.c_str());
	} else if (braces < 0) {
		LOG_L(L_WARNING, "%s: too many closing braces\n%s", __FUNCTION__, glob.c_str());
	}
#endif

	return regex;
}

bool FileSystem::FileExists(std::string file)
{
	FixSlashes(file);
	return FileSystemAbstraction::FileExists(file);
}

size_t FileSystem::GetFileSize(std::string file)
{
	if (!CheckFile(file)) {
		return 0;
	}
	FixSlashes(file);
	return FileSystemAbstraction::GetFileSize(file);
}

bool FileSystem::CreateDirectory(std::string dir)
{
	if (!CheckFile(dir)) {
		return false;
	}

	ForwardSlashes(dir);
	size_t prev_slash = 0, slash;
	while ((slash = dir.find('/', prev_slash + 1)) != std::string::npos) {
		std::string pathPart = dir.substr(0, slash);
		if (!FileSystemAbstraction::IsFSRoot(pathPart) && !FileSystemAbstraction::MkDir(pathPart)) {
			return false;
		}
		prev_slash = slash;
	}
	return FileSystemAbstraction::MkDir(dir);
}




std::string FileSystem::GetDirectory(const std::string& path)
{
	size_t s = path.find_last_of("\\/");
	if (s != std::string::npos) {
		return path.substr(0, s + 1);
	}
	return ""; // XXX return "./"? (a short test caused a crash because CFileHandler used in Lua couldn't find a file in the base-dir)
}

std::string FileSystem::GetFilename(const std::string& path)
{
	size_t s = path.find_last_of("\\/");
	if (s != std::string::npos) {
		return path.substr(s + 1);
	}
	return path;
}

std::string FileSystem::GetBasename(const std::string& path)
{
	std::string fn = GetFilename(path);
	size_t dot = fn.find_last_of('.');
	if (dot != std::string::npos) {
		return fn.substr(0, dot);
	}
	return fn;
}

std::string FileSystem::GetExtension(const std::string& path)
{
	const std::string fileName = GetFilename(path);
	size_t l = fileName.length();
//#ifdef WIN32
	//! windows eats dots and spaces at the end of filenames
	while (l > 0) {
		const char prevChar = fileName[l-1];
		if ((prevChar == '.') || (prevChar == ' ')) {
			l--;
		} else {
			break;
		}
	}
//#endif
	size_t dot = fileName.rfind('.', l);
	if (dot != std::string::npos) {
		return StringToLower(fileName.substr(dot + 1));
	}

	return "";
}


std::string FileSystem::GetNormalizedPath(const std::string& path) {

	std::string normalizedPath = StringReplace(path, "\\", "/"); // convert to POSIX path separators

	normalizedPath = StringReplace(normalizedPath, "/./", "/");
	normalizedPath = boost::regex_replace(normalizedPath, boost::regex("[/]{2,}"), "/");
	normalizedPath = boost::regex_replace(normalizedPath, boost::regex("[^/]+[/][.]{2}"), "");
	normalizedPath = boost::regex_replace(normalizedPath, boost::regex("[/]{2,}"), "/");

	return normalizedPath; // maybe use FixSlashes here
}

std::string& FileSystem::FixSlashes(std::string& path)
{
	int sep = FileSystem::GetNativePathSeparator();
	for (size_t i = 0; i < path.size(); ++i) {
		if (path[i] == '/' || path[i] == '\\') {
			path[i] = sep;
		}
	}

	return path;
}

std::string& FileSystem::ForwardSlashes(std::string& path)
{
	for (size_t i = 0; i < path.size(); ++i) {
		if (path[i] == '\\') {
			path[i] = '/';
		}
	}

	return path;
}


bool FileSystem::CheckFile(const std::string& file)
{
	// Don't allow code to escape from the data directories.
	// Note: this does NOT mean this is a SAFE fopen function:
	// symlink-, hardlink-, you name it-attacks are all very well possible.
	// The check is just ment to "enforce" certain coding behaviour.
	bool hasParentRef = (file.find("..") != std::string::npos);

	return !hasParentRef;
}
// bool FileSystem::CheckDir(const std::string& dir) const {
// 	return CheckFile(dir);
// }
bool FileSystem::Remove(std::string file)
{
	if (!CheckFile(file)) {
		return false;
	}
	FixSlashes(file);
	return FileSystem::DeleteFile(file);
}
