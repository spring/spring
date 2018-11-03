/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ConfigSource.h"
#include "ConfigVariable.h"
#include "System/Log/ILog.h"
#include "System/Platform/ScopedFileLock.h"

#ifdef WIN32
	#include <io.h>
#else
	#include <unistd.h>
#endif
#include <cstring>
#include <stdexcept>
#include <functional>

/******************************************************************************/

using std::map;
using std::string;

typedef map<string, string> StringMap;

/******************************************************************************/

bool ReadOnlyConfigSource::IsSet(const string& key) const
{
	return data.find(key) != data.end();
}

string ReadOnlyConfigSource::GetString(const string& key) const
{
	StringMap::const_iterator pos = data.find(key);
	if (pos == data.end()) {
		throw std::runtime_error("ConfigSource: Error: Key does not exist: " + key);
	}
	return pos->second;
}

/******************************************************************************/

void ReadWriteConfigSource::SetString(const string& key, const string& value)
{
	data[key] = value;
}

void ReadWriteConfigSource::Delete(const string& key)
{
	data.erase(key);
}

/******************************************************************************/

FileConfigSource::FileConfigSource(const string& filename) : filename(filename)
{
	FILE* file;

	if ((file = fopen(filename.c_str(), "r"))) {
		ScopedFileLock scoped_lock(fileno(file), false);
		Read(file);
		fclose(file);
	}
	else if ((file = fopen(filename.c_str(), "a"))) {
		// TODO: write some initial contents into the config file?
		fclose(file);
	}
	else {
		LOG_L(L_ERROR, "FileConfigSource: Error: Could not write to config file \"%s\"", filename.c_str());
	}
}

void FileConfigSource::SetStringInternal(const std::string& key, const std::string& value)
{
	ReadWriteConfigSource::SetString(key, value);
}

void FileConfigSource::SetString(const string& key, const string& value)
{
	ReadModifyWrite(std::bind(&FileConfigSource::SetStringInternal, this, key, value));
}

void FileConfigSource::DeleteInternal(const string& key)
{
	ReadWriteConfigSource::Delete(key);
	// note: comments intentionally not deleted (see Write)
	// comments.erase(key);
}

void FileConfigSource::Delete(const string& key)
{
	ReadModifyWrite(std::bind(&FileConfigSource::DeleteInternal, this, key));
}

void FileConfigSource::ReadModifyWrite(std::function<void ()> modify) {
	FILE* file = fopen(filename.c_str(), "r+");

	if (file) {
		ScopedFileLock scoped_lock(fileno(file), true);
		Read(file);
		modify();
		Write(file);
	}
	else {
		modify();
	}

	// must be outside above 'if (file)' block because of the lock.
	if (file) {
		fclose(file);
	}
}

/**
 * @brief strip whitespace
 *
 * Strips whitespace off the string [begin, end] by setting the last
 * whitespace character from the end to 0 and returning a pointer
 * to the first non-whitespace character from the beginning.
 *
 * Precondition: end must point to the last character of the string,
 * i.e. the one before the terminating '\0'.
 */
char* FileConfigSource::Strip(char* begin, char* end) {
	while (end >= begin && isspace(*end)) --end;
	while (begin <= end && isspace(*begin)) ++begin;
	*(end + 1) = '\0';
	return begin;
}

/**
 * @brief Rewind file and re-read it.
 */
void FileConfigSource::Read(FILE* file)
{
	std::ostringstream commentBuffer;
	char line[500];
	rewind(file);
	while (fgets(line, sizeof(line), file)) {
		char* line_stripped = Strip(line, strchr(line, '\0') - 1);
		if (*line_stripped == '\0' || *line_stripped == '#') {
			// an empty line or a comment line
			// note: trailing whitespace has been removed by Strip
			commentBuffer << line << "\n";
		}
		else {
			char* eq = strchr(line_stripped, '=');
			if (eq) {
				// a "key=value" line
				char* key = Strip(line_stripped, eq - 1);
				char* value = Strip(eq + 1, strchr(eq + 1, '\0') - 1);
				data[key] = value;
				if (commentBuffer.tellp() > 0) {
					comments[key] = commentBuffer.str();
					// reset the ostringstream
					commentBuffer.clear();
					commentBuffer.str("");
				}
			}
			else {
				// neither a comment nor an empty line nor a key=value line
				LOG_L(L_ERROR, "ConfigSource: Error: Can not parse line: %s", line);
			}
		}
	}

	if (commentBuffer.tellp() > 0) {
		// Must be sorted after all valid config variable names.
		const std::string AT_THE_END = "\x7f";
		comments[AT_THE_END] = commentBuffer.str();
	}
}

/**
 * @brief Truncate file and write data to it.
 */
void FileConfigSource::Write(FILE* file)
{
	rewind(file);
#ifdef WIN32
	int err = _chsize(fileno(file), 0);
#else
	int err = ftruncate(fileno(file), 0);
#endif
	if (err != 0) {
		LOG_L(L_ERROR, "FileConfigSource: Error: Failed truncating config file.");
	}

	// Iterate through both containers at once, interspersing
	// comment lines with key value pair lines.
	StringMap::const_iterator iter = data.begin();
	StringMap::const_iterator commentIter = comments.begin();
	while (iter != data.end()) {
		while (commentIter != comments.end() && commentIter->first <= iter->first) {
			fputs(commentIter->second.c_str(), file);
			++commentIter;
		}
		fprintf(file, "%s = %s\n", iter->first.c_str(), iter->second.c_str());
		++iter;
	}

	// Finish by writing all new remaining comments.
	while (commentIter != comments.end()) {
		fputs(commentIter->second.c_str(), file);
		++commentIter;
	}
}

/******************************************************************************/

/**
 * @brief Fill with default values of declared configuration variables.
 */
DefaultConfigSource::DefaultConfigSource()
{
	const ConfigVariable::MetaDataMap& vars = ConfigVariable::GetMetaDataMap();

	for (const auto& element: vars) {
		const ConfigVariableMetaData* metadata = element.second;
		if (metadata->GetDefaultValue().IsSet()) {
			data[metadata->GetKey()] = metadata->GetDefaultValue().ToString();
		}
	}
}


/**
 * @brief Fill with safemode values of declared configuration variables.
 */
SafemodeConfigSource::SafemodeConfigSource()
{
	const ConfigVariable::MetaDataMap& vars = ConfigVariable::GetMetaDataMap();

	for (const auto& element: vars) {
		const ConfigVariableMetaData* metadata = element.second;
		if (metadata->GetSafemodeValue().IsSet()) {
			data[metadata->GetKey()] = metadata->GetSafemodeValue().ToString();
		}
	}
}

/**
 * @brief Fill with dedicated values of declared configuration variables.
 */
DedicatedConfigSource::DedicatedConfigSource()
{
	const ConfigVariable::MetaDataMap& vars = ConfigVariable::GetMetaDataMap();

	for (const auto& element: vars) {
		const ConfigVariableMetaData* metadata = element.second;
		if (metadata->GetDedicatedValue().IsSet()) {
			data[metadata->GetKey()] = metadata->GetDedicatedValue().ToString();
		}
	}
}


/**
 * @brief Fill with headless values of declared configuration variables.
 */
HeadlessConfigSource::HeadlessConfigSource()
{
	const ConfigVariable::MetaDataMap& vars = ConfigVariable::GetMetaDataMap();

	for (const auto& element: vars) {
		const ConfigVariableMetaData* metadata = element.second;
		if (metadata->GetHeadlessValue().IsSet()) {
			data[metadata->GetKey()] = metadata->GetHeadlessValue().ToString();
		}
	}
}

/******************************************************************************/
