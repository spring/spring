/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CATEGORY_HANDLER_H
#define _CATEGORY_HANDLER_H

#include <string>
#include <vector>

#include "System/UnorderedMap.hpp"
#include "System/Misc/NonCopyable.h"
#include "System/creg/creg_cond.h"

class CCategoryHandler : public spring::noncopyable
{
	CR_DECLARE_STRUCT(CCategoryHandler)

public:
	// categories work as bitfields, so
	// we can not support more than this
	static constexpr inline unsigned int GetMaxCategories() { return (sizeof(unsigned int) * 8); }

	static CCategoryHandler* Instance();

	static void CreateInstance();
	static void RemoveInstance();


	void Init() { categories.reserve(GetMaxCategories()); firstUnused = 0; }
	void Kill() { categories.clear(); }

	/**
	 * Returns the categories bit-field value.
	 * @return the categories bit-field value or 0,
	 *         in case of empty name or too many categories
	 */
	unsigned int GetCategory(std::string name);

	/**
	 * Returns the bit-field values of a list of category names.
	 * @see GetCategory(std::string name)
	 */
	unsigned int GetCategories(std::string names);

	/**
	 * Returns a list of names of all categories described by the bit-field.
	 * @see GetCategory(std::string name)
	 */
	std::vector<std::string> GetCategoryNames(unsigned int bits) const;

private:
	// iterated in GetCategoryNames; reserved size must be constant
	spring::unordered_map<std::string, unsigned int> categories;

	unsigned int firstUnused = 0;
};

#endif // _CATEGORY_HANDLER_H
