/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONFIG_LOCATER_H
#define CONFIG_LOCATER_H

#include <string>

namespace ConfigLocater {
	/**
	 * @brief Get the name of the default configuration file
	 */
	std::string GetDefaultLocation();
};

#endif // CONFIG_LOCATER_H
