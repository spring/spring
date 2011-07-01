/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DEFAULT_CONFIG_SOURCE_H
#define DEFAULT_CONFIG_SOURCE_H

#include "ConfigSource.h"

/**
 * @brief Configuration source that holds static defaults
 *
 * Keys and default values for each engine configuration variable
 * are declared in and exposed by this class.
 */
class DefaultConfigSource : public ReadOnlyConfigSource
{
public:
	DefaultConfigSource();
};

#endif // DEFAULT_CONFIG_SOURCE_H
