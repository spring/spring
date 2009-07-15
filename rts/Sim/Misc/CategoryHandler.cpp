#include "StdAfx.h"
#include <algorithm>
#include <cctype>
#include "mmgr.h"

#include "CategoryHandler.h"
#include "LogOutput.h"
#include "creg/STL_Map.h"
#include "Util.h"

CR_BIND(CCategoryHandler, );

CR_REG_METADATA(CCategoryHandler, (
		CR_MEMBER(categories),
		CR_MEMBER(firstUnused),
		CR_RESERVED(8)
		));

CCategoryHandler* CCategoryHandler::instance;


void CCategoryHandler::RemoveInstance()
{
	delete instance;
	instance = NULL;
}


CCategoryHandler::CCategoryHandler() : firstUnused(0)
{
}


CCategoryHandler::~CCategoryHandler()
{
}


unsigned int CCategoryHandler::GetCategory(std::string name)
{
	StringToLowerInPlace(name);
	while(!name.empty() && *name.begin()==' ')
		name.erase(name.begin());
	if(name.empty())
		return 0;
	// Remove some categories that we don't think we need since we have too few of them.
	if (categories.find(name) == categories.end()) {
		if(firstUnused > 31) {
			logOutput.Print("WARNING: too many unit categories %i missed %s", ++firstUnused, name.c_str());
			categories[name] = 0; // suppress further warnings about this category
			return 0;
		}
		categories[name] = (1 << (firstUnused++));
//		logOutput.Print("New cat %s #%i",name.c_str(),firstUnused);
	}
	return categories[name];
}


unsigned int CCategoryHandler::GetCategories(std::string names)
{
	StringToLowerInPlace(names);
	unsigned int ret = 0;

	while(!names.empty()) {
		std::string name = names;

		if(names.find_first_of(' ') != std::string::npos)
			name.erase(name.find_first_of(' '), 5000);

		if(names.find_first_of(' ') == std::string::npos)
			names.clear();
		else
			names.erase(0, names.find_first_of(' ') + 1);

		ret |= GetCategory(name);
	}
	return ret;
}


std::vector<std::string> CCategoryHandler::GetCategoryNames(unsigned int bits) const
{
	std::vector<std::string> names;
	unsigned int bit;
	for (bit = 1; bit != 0; bit = (bit << 1)) {
		if ((bit & bits) != 0) {
			std::map<std::string,unsigned int>::const_iterator it;
			for (it = categories.begin(); it != categories.end(); ++it) {
				if (it->second == bit) {
					names.push_back(it->first);
				}
			}
		}
	}
	return names;
}
