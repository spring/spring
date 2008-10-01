// RegHandler.cpp: implementation of the RegHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "RegHandler.h"
#include <SDL_types.h>
#include "mmgr.h"

using std::string;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

RegHandler::RegHandler(string keyname, HKEY key)
{
	if (RegCreateKey(key, keyname.c_str(), &regkey) != ERROR_SUCCESS)
		MessageBox(0, "Failed to create registry key", "Registry error", 0);
}

RegHandler::~RegHandler()
{
	RegCloseKey(regkey);
}


int RegHandler::GetInt(string name, int def)
{
	unsigned char regbuf[100]={'\0'};
	DWORD regLength=sizeof(regbuf); // this is windows specific stuff, so no need to use sdl types
	DWORD regType=REG_DWORD;

	if(RegQueryValueEx(regkey,name.c_str(),0,&regType,regbuf,&regLength)==ERROR_SUCCESS)
		return *((int*)regbuf);
	else
		SetInt(name, def);
		
	return def;
}

string RegHandler::GetString(string name, string def)
{
	unsigned char regbuf[100]={'\0'};
	DWORD regLength=sizeof(regbuf);
	DWORD regType=REG_SZ;

	if(RegQueryValueEx(regkey,name.c_str(),0,&regType,regbuf,&regLength)==ERROR_SUCCESS) {
		return string((char*)regbuf);
	}
	else
		SetString(name, def);
		
	return def;
}

void RegHandler::SetString(string name, string value)
{
	RegSetValueEx(regkey,name.c_str(),0,REG_SZ,(unsigned char*)value.c_str(),value.size()+1);
}

void RegHandler::SetInt(string name, int value)
{
	RegSetValueEx(regkey,name.c_str(),0,REG_DWORD,(unsigned char*)&value,sizeof(int));
}
