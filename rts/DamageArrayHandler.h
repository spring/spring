#pragma once

#include <string>
#include <map>

class CDamageArrayHandler
{
public:
	CDamageArrayHandler(void);
	~CDamageArrayHandler(void);
	int GetTypeFromName(std::string name);

	int numTypes;
	std::map<std::string,int> name2type;
};

extern CDamageArrayHandler* damageArrayHandler;