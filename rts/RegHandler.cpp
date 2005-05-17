// RegHandler.cpp: implementation of the RegHandler class.
//
//////////////////////////////////////////////////////////////////////

//FIXME the registery is disable for linux, but a common interface to store options have to be provided

#include "StdAfx.h"
#include "RegHandler.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

RegHandler regHandler("Software\\SJ\\spring");

RegHandler::RegHandler(string keyname,HKEY key)
{
#ifndef NO_WINREG
	if(RegCreateKey(key,keyname.c_str(),&regkey)!=ERROR_SUCCESS)
		MessageBox(0,"Failed to crete registry key","Registry error",0);
#endif
}

RegHandler::~RegHandler()
{
#ifndef NO_WINREG
	RegCloseKey(regkey);
#endif
}


unsigned int RegHandler::GetInt(string name, unsigned int def)
{
#ifndef  NO_WINREG
	unsigned char regbuf[100];
	DWORD regLength=100;
	DWORD regType=REG_DWORD;

	if(RegQueryValueEx(regkey,name.c_str(),0,&regType,regbuf,&regLength)==ERROR_SUCCESS)
		return *((unsigned int*)regbuf);
#endif
	return def;
}

string RegHandler::GetString(string name, string def)
{
#ifndef NO_WINREG
	unsigned char regbuf[100];
	DWORD regLength=100;
	DWORD regType=REG_SZ;

	if(RegQueryValueEx(regkey,name.c_str(),0,&regType,regbuf,&regLength)==ERROR_SUCCESS)
		return string((char*)regbuf);
#endif
	return def;
}

void RegHandler::SetString(string name, string value)
{
#ifndef NO_WINREG
	RegSetValueEx(regkey,name.c_str(),0,REG_SZ,(unsigned char*)value.c_str(),value.size()+1);
#endif
}

void RegHandler::SetInt(string name, unsigned int value)
{
#ifndef NO_WINREG
	RegSetValueEx(regkey,name.c_str(),0,REG_DWORD,(unsigned char*)&value,sizeof(int));
#endif	
}
