#ifndef __CATEGORY_HANDLER_H__
#define __CATEGORY_HANDLER_H__

#include <string>
#include <vector>
#include <map>

class CCategoryHandler;

extern CCategoryHandler* categoryHandler;

class CCategoryHandler
{
	CR_DECLARE(CCategoryHandler);
public:
	~CCategoryHandler();

	std::map<std::string,unsigned int> categories;

	unsigned int GetCategory(std::string name);
	unsigned int GetCategories(std::string names);
	std::vector<std::string> GetCategoryNames(unsigned int bits) const;

	int firstUnused;

	static CCategoryHandler* Instance(){
		if(instance==0){
			instance=SAFE_NEW CCategoryHandler();
			categoryHandler=instance;
		}
		return instance;
	}
	static void RemoveInstance(){
		delete instance;
		instance=0;
	}
protected:
	CCategoryHandler();
	static CCategoryHandler* instance;
};

#endif // __CATEGORY_HANDLER_H__
