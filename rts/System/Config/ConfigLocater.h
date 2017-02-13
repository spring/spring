/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONFIG_LOCATER_H
#define CONFIG_LOCATER_H

#include <string>
#include <vector>

namespace ConfigLocater {
	/**
	 * @brief Get the names of the default configuration files
	 */
	void GetDefaultLocations(std::vector<std::string>& locations);
}

#endif // CONFIG_LOCATER_H
