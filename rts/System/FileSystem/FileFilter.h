#ifndef FILEFILTER_H
#define FILEFILTER_H

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

#endif // !FILEFILTER_H
