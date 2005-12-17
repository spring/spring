// RegHandler.h: interface for the RegHandler class.
//
//////////////////////////////////////////////////////////////////////

#ifndef REGHANDLER_H
#define REGHANDLER_H

#include "Platform/ConfigHandler.h"

#include <string>
#include "win32.h"
#include <winreg.h>

using std::string;

class RegHandler: public ConfigHandler 
{
public:
	RegHandler(string keyname,HKEY key=HKEY_CURRENT_USER);
	virtual ~RegHandler();
	virtual void SetInt(std::string name, unsigned int value);
	virtual void SetString(std::string name, std::string value);
	virtual std::string GetString(std::string name, std::string def);
	virtual unsigned int GetInt(std::string name, unsigned int def);
protected:
	HKEY regkey;
};

#endif // REGHANDLER_H

