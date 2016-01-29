/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <algorithm>
#include <cassert>
#include <cctype>

#include "CategoryHandler.h"

#include "System/creg/STL_Map.h"
#include "System/Util.h"
#include "System/Log/ILog.h"

CR_BIND(CCategoryHandler, )

CR_REG_METADATA(CCategoryHandler, (
	CR_MEMBER(categories),
	CR_MEMBER(firstUnused)
))

CCategoryHandler* CCategoryHandler::instance = NULL;

CCategoryHandler* CCategoryHandler::Instance() {
	assert(instance != NULL);
	return instance;
}


void CCategoryHandler::CreateInstance() {
	if (instance == NULL) {
		instance = new CCategoryHandler();
	}
}

void CCategoryHandler::RemoveInstance()
{
	delete instance;
	instance = NULL;
}


unsigned int CCategoryHandler::GetCategory(std::string name)
{
	StringTrimInPlace(name);
	StringToLowerInPlace(name);

	if (name.empty())
		return 0; // the empty category

	unsigned int cat = 0;

	if (categories.find(name) == categories.end()) {
		// this category is yet unknown
		if (firstUnused >= CCategoryHandler::GetMaxCategories()) {
			// skip this category
			LOG_L(L_WARNING, "too many unit categories (%i), skipping %s",
					firstUnused, name.c_str());
			cat = 0;
		} else {
			// create the category (bit field value)
			cat = (1 << firstUnused);
			//LOG_L(L_DEBUG, "New unit-category %s #%i", name.c_str(), firstUnused);
		}
		// if (cat == 0), this will prevent further warnings for this category
		categories[name] = cat;
		firstUnused++;
	} else {
		// this category is already known
		cat = categories[name];
	}

	return cat;
}


unsigned int CCategoryHandler::GetCategories(std::string names)
{
	unsigned int ret = 0;

	StringToLowerInPlace(names);

	// split on ' '
	std::stringstream namesStream(names);
	std::string name;

	while (std::getline(namesStream, name, ' ')) {
		if (!name.empty()) {
			ret |= GetCategory(name);
		}
	}

	return ret;
}


std::vector<std::string> CCategoryHandler::GetCategoryNames(unsigned int bits) const
{
	std::vector<std::string> names;

	for (unsigned int bit = 1; bit != 0; bit = (bit << 1)) {
		if ((bit & bits) != 0) {
			for (auto it = categories.cbegin(); it != categories.cend(); ++it) {
				if (it->second == bit) {
					names.push_back(it->first);
				}
			}
		}
	}

	return names;
}

