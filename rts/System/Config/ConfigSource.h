/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONFIG_SOURCE_H
#define CONFIG_SOURCE_H

#include <functional>
#include <map>
#include <string>
#include <stdio.h>

/**
 * @brief Abstraction of a read-only configuration source
 */
class ReadOnlyConfigSource
{
public:
	virtual ~ReadOnlyConfigSource() {}

	virtual bool IsSet(const std::string& key) const;
	virtual std::string GetString(const std::string& key) const;

	const std::map<std::string, std::string>& GetData() const { return data; }

protected:
	std::map<std::string, std::string> data;
};

/**
 * @brief Abstraction of a writable configuration source
 */
class ReadWriteConfigSource : public ReadOnlyConfigSource
{
public:
	virtual void SetString(const std::string& key, const std::string& value);
	virtual void Delete(const std::string& key);
};

/**
 * @brief Overlay configuration source
 *
 * Overlay settings get lost after the game ends. Values in the OPTIONS section
 * of the start script will be put into the overlay and Lua scripts can choose
 * to put certain values into the overlay.
 */
class OverlayConfigSource : public ReadWriteConfigSource
{
};

/**
 * @brief File-backed configuration source
 */
class FileConfigSource : public ReadWriteConfigSource
{
public:
	FileConfigSource(const std::string& filename);

	void SetString(const std::string& key, const std::string& value);
	void Delete(const std::string& key);

	std::string GetFilename() const { return filename; }

private:
	void SetStringInternal(const std::string& key, const std::string& value);
	void DeleteInternal(const std::string& key);
	void ReadModifyWrite(std::function<void ()> modify);

	std::string filename;
	std::map<std::string, std::string> comments;

	// helper functions
	void Read(FILE* file);
	void Write(FILE* file);
	char* Strip(char* begin, char* end);
};

/**
 * @brief Configuration source that holds static defaults
 *
 * Keys and default values for each engine configuration variable
 * are exposed by this class.
 */
class DefaultConfigSource : public ReadOnlyConfigSource
{
public:
	DefaultConfigSource();
};

/**
 * @brief Configuration source that holds safemode values
 *
 * Used when spring was started with "--safemode" param
 */
class SafemodeConfigSource : public ReadOnlyConfigSource
{
public:
	SafemodeConfigSource();
};

/**
 * @brief Configuration source that holds safemode values
 *
 * Used when spring was started with "--safemode" param
 */
class HeadlessConfigSource : public ReadOnlyConfigSource
{
public:
	HeadlessConfigSource();
};

/**
 * @brief Configuration source that holds safemode values
 *
 * Used when spring was started with "--safemode" param
 */
class DedicatedConfigSource : public ReadOnlyConfigSource
{
public:
	DedicatedConfigSource();
};

#endif // CONFIG_SOURCE_H
