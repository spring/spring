#include "stdafx.h"
#include "unitsync.h"
#include <windows.h>

#include "HpiHandler.h"

#include "Syncer.h"
#include "SyncServer.h"

#include <string>

//This means that the DLL can only support one instance. Don't think this should be a problem.
static CSyncer *syncer = NULL;

//The SunParswer wants this
HWND hWnd = 0;

BOOL __stdcall DllMain(HINSTANCE hInst,
                       DWORD dwReason,
                       LPVOID lpReserved) {
   return TRUE;
}

extern "C" __declspec(dllexport) void __stdcall Message(char* p_szMessage)
{
   MessageBox(NULL, p_szMessage, "Message from DLL", MB_OK);
}

extern "C" int __stdcall Init(bool isServer, int id)
{
	if (!hpiHandler)
		hpiHandler = new CHpiHandler();

	if (isServer) {
		syncer = new CSyncServer(id);
	}
	else {
		syncer = new CSyncer(id);
	}

	return 1;
}

extern "C" int __stdcall ProcessUnits(void)
{
	return syncer->ProcessUnits();
}

extern "C" const char * __stdcall GetCurrentList()
{
	string tmp = syncer->GetCurrentList();
/*	int tmpLen = (int)tmp.length();

	if (tmpLen > *bufLen) {
		*bufLen = tmpLen;
		return -1;
	}

	strcpy(buffer, tmp.c_str());
	buffer[tmpLen] = 0;
	*bufLen = tmpLen;

	return tmpLen; */

	return GetStr(tmp);
}

/*void AddClient(int id, char *unitList)
void RemoveClient(int id)
char *GetClientDiff(int id)
void InstallClientDiff(char *diff) */

extern "C" void __stdcall AddClient(int id, char *unitList)
{
	((CSyncServer *)syncer)->AddClient(id, unitList);
}

extern "C" void __stdcall RemoveClient(int id)
{
	((CSyncServer *)syncer)->RemoveClient(id);
}

extern "C" const char * __stdcall GetClientDiff(int id)
{
	string tmp = ((CSyncServer *)syncer)->GetClientDiff(id);
	return GetStr(tmp);
}

extern "C" void __stdcall InstallClientDiff(char *diff)
{
	syncer->InstallClientDiff(diff);
}

/*int GetUnitCount()
char *GetUnitName(int unit)
int IsUnitDisabled(int unit)
int IsUnitDisabledByClient(int unit, int clientId)*/

extern "C" int __stdcall GetUnitCount()
{
	return syncer->GetUnitCount();
}

extern "C" const char * __stdcall GetUnitName(int unit)
{
	string tmp = syncer->GetUnitName(unit);
	return GetStr(tmp);
}

extern "C" const char * __stdcall GetFullUnitName(int unit)
{
	string tmp = syncer->GetFullUnitName(unit);
	return GetStr(tmp);
}

extern "C" int __stdcall IsUnitDisabled(int unit)
{
	if (syncer->IsUnitDisabled(unit))
		return 1;
	else
		return 0;
}

extern "C" int __stdcall IsUnitDisabledByClient(int unit, int clientId)
{
	if (syncer->IsUnitDisabledByClient(unit, clientId))
		return 1;
	else
		return 0;
}

//////////////////////////
//////////////////////////

//Just returning str.c_str() does not work
const char *GetStr(string str)
{
	static char strBuf[STRBUF_SIZE];
	
	if (str.length() + 1 > STRBUF_SIZE) {
		sprintf(strBuf, "Increase STRBUF_SIZE (needs %d bytes)", str.length() + 1);
	}
	else {
		strcpy(strBuf, str.c_str());
	}

	return strBuf;
}

void PrintLoadMsg(const char* text)
{
}