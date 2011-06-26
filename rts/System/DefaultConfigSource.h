/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DEFAULT_CONFIG_SOURCE_H
#define DEFAULT_CONFIG_SOURCE_H

#include <sstream>
#include "ConfigSource.h"

/**
 * @brief Configuration source that holds static defaults
 *
 * Keys and default values for each engine configuration variable
 * are declared in and exposed by this class.
 */
class DefaultConfigSource : public ConfigSource
{
public:
	DefaultConfigSource();

private:
	template<typename T>
	void Add(const std::string& key, const T& value, const std::string& comment = "")
	{
		// note: comment is ignored at this moment
		std::ostringstream buffer;
		buffer << value;
		SetString(key, buffer.str());
	}
};

#endif // DEFAULT_CONFIG_SOURCE_H
