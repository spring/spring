/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FILE_FILTER_H
#define FILE_FILTER_H

#include <string>

/** @brief Provides a way to filter files using globbing. */
class IFileFilter
{
public:
	static IFileFilter* Create();

	virtual ~IFileFilter() {}
	virtual void AddRule(const std::string& rule) = 0;
	virtual bool Match(const std::string& filename) const = 0;
};

#endif // !FILE_FILTER_H
