/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __CATEGORY_HANDLER_H__
#define __CATEGORY_HANDLER_H__

#include <string>
#include <vector>
#include <map>
#include <boost/noncopyable.hpp>
#include "creg/creg_cond.h"

class CCategoryHandler : public boost::noncopyable
{
	CR_DECLARE(CCategoryHandler);

public:
	static inline unsigned int GetMaxCategories() {
		// as categories work as bitfields,
		// we can not support more then 32
		// due to (sizeof(unsigned int) == 32) on 32bit systems
		return 32;
	}

	static CCategoryHandler* Instance() {
		if (instance == NULL) instance = new CCategoryHandler();
		return instance;
	}
	static void RemoveInstance();

	/**
	 * Returns the categories bit field value.
	 * @return the categories bit field value or 0,
	 *         in case of empty name or too many categories
	 */
	unsigned int GetCategory(std::string name);
	/**
	 * Returns the bitfield values of a list of category names
	 * @see GetCategory(std::string name)
	 */
	unsigned int GetCategories(std::string names);
	std::vector<std::string> GetCategoryNames(unsigned int bits) const;

private:
	static CCategoryHandler* instance;

	CCategoryHandler();
	~CCategoryHandler();

	std::map<std::string,unsigned int> categories;
	unsigned int firstUnused;
};

#endif // __CATEGORY_HANDLER_H__
