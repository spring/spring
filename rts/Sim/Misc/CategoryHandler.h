#ifndef __CATEGORY_HANDLER_H__
#define __CATEGORY_HANDLER_H__

#include <string>
#include <vector>
#include <map>

class CCategoryHandler
{
	CR_DECLARE(CCategoryHandler);

public:
	static CCategoryHandler* Instance() {
		if (instance == NULL) instance = SAFE_NEW CCategoryHandler();
		return instance;
	}
	static void RemoveInstance();

	unsigned int GetCategory(std::string name);
	unsigned int GetCategories(std::string names);
	std::vector<std::string> GetCategoryNames(unsigned int bits) const;

private:
	static CCategoryHandler* instance;

	CCategoryHandler();
	~CCategoryHandler();

	std::map<std::string,unsigned int> categories;
	int firstUnused;
};

#endif // __CATEGORY_HANDLER_H__
