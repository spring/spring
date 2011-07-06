/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "ConfigSource.h"
#include "ConfigVariable.h"
#include "System/LogOutput.h"
#include "System/Platform/ScopedFileLock.h"

#ifdef WIN32
	#include <io.h>
#endif
#include <string.h>
#include <stdexcept>
#include <boost/bind.hpp>

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
	}
	else {
		if (!(file = fopen(filename.c_str(), "a")))
			throw std::runtime_error("FileConfigSource: Error: Could not write to config file \"" + filename + "\"");
	}
	fclose(file);
}

void FileConfigSource::SetStringInternal(const std::string& key, const std::string& value)
{
	ReadWriteConfigSource::SetString(key, value);
}

void FileConfigSource::SetString(const string& key, const string& value)
{
	ReadModifyWrite(boost::bind(&FileConfigSource::SetStringInternal, this, key, value));
}

void FileConfigSource::DeleteInternal(const string& key)
{
	ReadWriteConfigSource::Delete(key);
	comments.erase(key);
}

void FileConfigSource::Delete(const string& key)
{
	ReadModifyWrite(boost::bind(&FileConfigSource::DeleteInternal, this, key));
}

void FileConfigSource::ReadModifyWrite(boost::function<void ()> modify) {
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
 * Strips whitespace off the string [begin, end] by setting the first
 * non-whitespace character from the end to 0 and returning a pointer
 * to the first non-whitespace character from the beginning.
 *
 * Precondition: end must point to the last character of the string,
 * i.e. the one before the terminating '\0'.
 */
char* FileConfigSource::Strip(char* begin, char* end) {
	while (isspace(*begin)) ++begin;
	while (end >= begin && isspace(*end)) --end;
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
		char* line_stripped = Strip(line, strchr(line, '\0'));
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
				comments[key] = commentBuffer.str();
				// reset the ostringstream
				commentBuffer.clear();
				commentBuffer.str("");
			}
			else {
				// neither a comment nor an empty line nor a key=value line
				logOutput.Print("ConfigSource: Error: Can not parse line: %s\n", line);
			}
		}
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
		logOutput.Print("FileConfigSource: Error: Failed truncating config file.");
	}
	for(StringMap::const_iterator iter = data.begin(); iter != data.end(); ++iter) {
		StringMap::const_iterator comment = comments.find(iter->first);
		if (comment != comments.end()) {
			fputs(comment->second.c_str(), file);
		}
		fprintf(file, "%s = %s\n", iter->first.c_str(), iter->second.c_str());
	}
}

/******************************************************************************/

/**
 * @brief Fill with default values of declared configuration variables.
 */
DefaultConfigSource::DefaultConfigSource()
{
	const ConfigVariable::MetaDataMap& vars = ConfigVariable::GetMetaDataMap();

	for (ConfigVariable::MetaDataMap::const_iterator it = vars.begin(); it != vars.end(); ++it) {
		const ConfigVariableMetaData* metadata = it->second;
		if (metadata->HasDefaultValue()) {
			data[metadata->key] = metadata->GetDefaultValue();
		}
	}
}

/******************************************************************************/
