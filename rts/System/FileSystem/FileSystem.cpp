/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * Glob conversion by Chris Han (based on work by Nathaniel Smith).
 */
#include "FileSystem.h"

#include "Game/GameVersion.h"
#include "System/Log/ILog.h"
#include "System/Platform/Win/win32.h"
#include "System/StringUtil.h"

#include "System/SpringRegex.h"

#include <unistd.h>
#ifdef _WIN32
#include <io.h>
#endif

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
	regex.reserve(glob.size() << 1);
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
					QUOTE(c, regex);
				}
				break;
			case '\\':
				++i;
#ifdef DEBUG
				if (i == glob.end()) {
					LOG_L(L_WARNING, "%s: pattern ends with backslash\n%s", __FUNCTION__, glob.c_str());
				}
#endif
				QUOTE(*i, regex);
				break;
			default:
				QUOTE(c, regex);
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


bool FileSystem::ComparePaths(std::string path1, std::string path2)
{
	path1 = FileSystem::EnsureNoPathSepAtEnd(FileSystem::GetNormalizedPath(path1));
	path2 = FileSystem::EnsureNoPathSepAtEnd(FileSystem::GetNormalizedPath(path2));
	return FileSystemAbstraction::ComparePaths(path1, path2);
}


bool FileSystem::FileExists(std::string file)
{
	return FileSystemAbstraction::FileExists(FileSystem::GetNormalizedPath(file));
}

size_t FileSystem::GetFileSize(std::string file)
{
	if (!CheckFile(file))
		return 0;

	return FileSystemAbstraction::GetFileSize(FileSystem::GetNormalizedPath(file));
}

bool FileSystem::CreateDirectory(std::string dir)
{
	if (!CheckFile(dir))
		return false;

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


bool FileSystem::TouchFile(std::string filePath)
{
	if (!CheckFile(filePath))
		return false;

	// check for read access
	if (access(filePath.c_str(), R_OK) == 0)
		return true;

	FILE* f = fopen(filePath.c_str(), "a+b");
	if (f == nullptr)
		return false;
	fclose(f);
	return (access(filePath.c_str(), R_OK) == 0); // check for read access
}




std::string FileSystem::GetDirectory(const std::string& path)
{
	const size_t s = path.find_last_of("\\/");

	if (s != std::string::npos)
		return path.substr(0, s + 1);

	return ""; // XXX return "./"? (a short test caused a crash because CFileHandler used in Lua couldn't find a file in the base-dir)
}

std::string FileSystem::GetFilename(const std::string& path)
{
	const size_t s = path.find_last_of("\\/");

	if (s != std::string::npos)
		return path.substr(s + 1);

	return path;
}

std::string FileSystem::GetBasename(const std::string& path)
{
	std::string fn = GetFilename(path);
	const size_t dot = fn.find_last_of('.');

	if (dot != std::string::npos)
		return fn.substr(0, dot);

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
	const size_t dot = fileName.rfind('.', l);

	if (dot != std::string::npos)
		return StringToLower(fileName.substr(dot + 1));

	return "";
}


std::string FileSystem::GetNormalizedPath(const std::string& path) {

	std::string normalizedPath = StringReplace(path, "\\", "/"); // convert to POSIX path separators

	normalizedPath = StringReplace(normalizedPath, "/./", "/");

	try {
		normalizedPath = spring::regex_replace(normalizedPath, spring::regex("[/]{2,}"), {"/"});
	} catch (const spring::regex_error& e) {
		LOG_L(L_WARNING, "[%s][1] regex exception \"%s\" (code=%d)", __func__, e.what(), int(e.code()));
	}

	try {
		normalizedPath = spring::regex_replace(normalizedPath, spring::regex("[^/]+[/][.]{2}"), {""});
	} catch (const spring::regex_error& e) {
		LOG_L(L_WARNING, "[%s][2] regex exception \"%s\" (code=%d)", __func__, e.what(), int(e.code()));
	}

	try {
		normalizedPath = spring::regex_replace(normalizedPath, spring::regex("[/]{2,}"), {"/"});
	} catch (const spring::regex_error& e) {
		LOG_L(L_WARNING, "[%s][3] regex exception \"%s\" (code=%d)", __func__, e.what(), int(e.code()));
	}

	return normalizedPath; // maybe use FixSlashes here
}

std::string& FileSystem::FixSlashes(std::string& path)
{
	const char sep = GetNativePathSeparator();
	const auto P = [](const char c) { return (c == '/' || c == '\\'); };

	std::replace_if(std::begin(path), std::end(path), P, sep);

	return path;
}

std::string& FileSystem::ForwardSlashes(std::string& path)
{
	std::replace(std::begin(path), std::end(path), '\\', '/');

	return path;
}


bool FileSystem::CheckFile(const std::string& file)
{
	// Don't allow code to escape from the data directories.
	// Note: this does NOT mean this is a SAFE fopen function:
	// symlink-, hardlink-, you name it-attacks are all very well possible.
	// The check is just meant to "enforce" certain coding behaviour.
	//
	return (file.find("..") == std::string::npos);
}

bool FileSystem::Remove(std::string file)
{
	if (!CheckFile(file))
		return false;

	return FileSystem::DeleteFile(FileSystem::GetNormalizedPath(file));
}

const std::string& FileSystem::GetCacheBaseDir()
{
	static const std::string cacheBaseDir = "cache";
	return cacheBaseDir;
}

const std::string& FileSystem::GetCacheDir()
{
	// cache-dir versioning must not be too finegrained,
	// we do want to regenerate cache after every commit
	//
	// release builds must however also *never* use the
	// same directory as any previous development build
	// (regardless of branch), so keep caches separate
	static const std::string cacheType[2] = {"dev-", "rel-"};
	static const std::string cacheVersion = SpringVersion::GetMajor() + cacheType[SpringVersion::IsRelease()] + SpringVersion::GetBranch();
	static const std::string cacheDir = EnsurePathSepAtEnd(GetCacheBaseDir()) + cacheVersion;
	return cacheDir;
}

