/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <algorithm>
#include <cassert>
#include <cctype>

#include "CategoryHandler.h"

#include "System/creg/STL_Map.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"

CR_BIND(CCategoryHandler, )

CR_REG_METADATA(CCategoryHandler, (
	CR_MEMBER(categories),
	CR_MEMBER(firstUnused)
))


static CCategoryHandler instance;


CCategoryHandler* CCategoryHandler::Instance() { return &instance; }

void CCategoryHandler::CreateInstance() { instance.Init(); }
void CCategoryHandler::RemoveInstance() { instance.Kill(); }


unsigned int CCategoryHandler::GetCategory(std::string name)
{
	StringTrimInPlace(name);
	StringToLowerInPlace(name);

	unsigned int cat = 0;

	// the empty category
	if (name.empty())
		return cat;

	if (categories.find(name) == categories.end()) {
		// this category is yet unknown
		if (firstUnused >= CCategoryHandler::GetMaxCategories()) {
			// skip this category
			LOG_L(L_WARNING, "[%s] too many unit categories (%i), skipping %s", __func__, firstUnused, name.c_str());
			cat = 0;
		} else {
			// create the category (bit field value)
			cat = (1 << firstUnused);
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
		if (name.empty())
			continue;

		ret |= GetCategory(name);
	}

	return ret;
}


std::vector<std::string> CCategoryHandler::GetCategoryNames(unsigned int bits) const
{
	std::vector<std::string> names;

	names.reserve(categories.size());

	for (unsigned int bit = 1; bit != 0; bit = (bit << 1)) {
		if ((bit & bits) == 0)
			continue;

		for (auto it = categories.cbegin(); it != categories.cend(); ++it) {
			if (it->second != bit)
				continue;

			names.push_back(it->first);
		}
	}

	return names;
}

