#ifndef __DAMAGE_ARRAY_HANDLER_H__
#define __DAMAGE_ARRAY_HANDLER_H__

#include <string>
#include <vector>
#include <map>

class CDamageArrayHandler
{
public:
	CDamageArrayHandler(void);
	~CDamageArrayHandler(void);
	int GetTypeFromName(std::string name);

	int numTypes;
	std::map<std::string,int> name2type;
	std::vector<std::string> typeList;
};

extern CDamageArrayHandler* damageArrayHandler;

#endif // __DAMAGE_ARRAY_HANDLER_H__
