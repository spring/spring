// RegHandler.h: interface for the RegHandler class.
//
//////////////////////////////////////////////////////////////////////

#ifndef REGHANDLER_H
#define REGHANDLER_H

#include "Platform/ConfigHandler.h"

#include <string>
#include "win32.h"
#include <winreg.h>

class RegHandler: public ConfigHandler 
{
public:
	RegHandler(std::string keyname,HKEY key=HKEY_CURRENT_USER);
	virtual ~RegHandler();
	virtual void SetInt(std::string name, int value);
	virtual void SetString(std::string name, std::string value);
	virtual std::string GetString(std::string name, std::string def);
	virtual int GetInt(std::string name, int def);
protected:
	HKEY regkey;
};

#endif // REGHANDLER_H

