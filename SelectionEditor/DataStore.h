#pragma once

#include <vector>
#include <map>
#include <string>

class CDataStore
{
public:
	CDataStore(void);
	~CDataStore(void);

	std::map<std::string,std::string> hotkeys;

	std::string curKey;
	std::string curString;
	void Save(void);
};

extern CDataStore datastore;