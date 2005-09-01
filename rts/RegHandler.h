// RegHandler.h: interface for the RegHandler class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGHANDLER_H__508F534F_9F3D_11D6_AD55_DE4DC0775D55__INCLUDED_)
#define AFX_REGHANDLER_H__508F534F_9F3D_11D6_AD55_DE4DC0775D55__INCLUDED_

#include "ConfigHandler.h"

#include <string>
#include <windows.h>
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

#endif // !defined(AFX_REGHANDLER_H__508F534F_9F3D_11D6_AD55_DE4DC0775D55__INCLUDED_)

