///////////////////////////////////////////////////////////////////////////////
//
//  Module: CrashRpt.cpp
//
//    Desc: See CrashRpt.h
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CrashRpt.h"
#include "CrashHandler.h"

#include "StackTrace.h"

#ifdef _DEBUG
#define CRASH_ASSERT(pObj)          \
   if (!pObj || sizeof(*pObj) != sizeof(CCrashHandler))  \
      DebugBreak()                                       
#else
#define CRASH_ASSERT(pObj)
#endif // _DEBUG

// New interfaces
CRASHRPTAPI LPVOID GetInstance()
{
   CCrashHandler *pImpl = CCrashHandler::GetInstance();
   CRASH_ASSERT(pImpl);
   return pImpl;
}

CRASHRPTAPI LPVOID InstallEx(LPGETLOGFILE pfn, LPCTSTR lpcszTo, LPCTSTR lpcszSubject)
{
	OutputDebugString("InstallEx\n");
   CCrashHandler *pImpl = CCrashHandler::GetInstance();
   CRASH_ASSERT(pImpl);
   pImpl->Install(pfn, lpcszTo, lpcszSubject);

   return pImpl;
}

CRASHRPTAPI void UninstallEx(LPVOID lpState)
{
   CCrashHandler *pImpl = (CCrashHandler*)lpState;
   CRASH_ASSERT(pImpl);

   delete pImpl;
}

CRASHRPTAPI void AddFileEx(LPVOID lpState, LPCTSTR lpFile, LPCTSTR lpDesc)
{
   CCrashHandler *pImpl = (CCrashHandler*)lpState;
   CRASH_ASSERT(pImpl);

   pImpl->AddFile(lpFile, lpDesc);
}

CRASHRPTAPI void RemoveFileEx(LPVOID lpState, LPCTSTR lpFile)
{
   CCrashHandler *pImpl = (CCrashHandler*)lpState;
   CRASH_ASSERT(pImpl);

   pImpl->RemoveFile(lpFile);
}

CRASHRPTAPI void AddRegistryHiveEx(LPVOID lpState, LPCTSTR lpHive, LPCTSTR lpDesc)
{
   CCrashHandler *pImpl = (CCrashHandler*)lpState;
   CRASH_ASSERT(pImpl);

   pImpl->AddRegistryHive(lpHive, lpDesc);
}

CRASHRPTAPI void RemoveRegistryHiveEx(LPVOID lpState, LPCTSTR lpFile)
{
   CCrashHandler *pImpl = (CCrashHandler*)lpState;
   CRASH_ASSERT(pImpl);

   pImpl->RemoveRegistryHive(lpFile);
}


CRASHRPTAPI void AddEventLogEx(LPVOID lpState, LPCTSTR lpHive, LPCTSTR lpDesc)
{
   CCrashHandler *pImpl = (CCrashHandler*)lpState;
   CRASH_ASSERT(pImpl);

   pImpl->AddEventLog(lpHive, lpDesc);
}

CRASHRPTAPI void RemoveEventLogEx(LPVOID lpState, LPCTSTR lpFile)
{
   CCrashHandler *pImpl = (CCrashHandler*)lpState;
   CRASH_ASSERT(pImpl);

   pImpl->RemoveEventLog(lpFile);
}

CRASHRPTAPI void GenerateErrorReportEx(LPVOID lpState, PEXCEPTION_POINTERS pExInfo, BSTR message)
{
   CCrashHandler *pImpl = (CCrashHandler*)lpState;
   CRASH_ASSERT(pImpl);

   if (!pImpl->GenerateErrorReport(pExInfo, message)) {
	   DebugBreak();
   }
}

CRASHRPTAPI void StackTrace ( int numSkip, int depth, TraceCallbackFunction pFunction, CONTEXT *pContext, void *data)
{
	DoStackTrace(numSkip, depth > 0 ? depth : 9999, pFunction, pContext, data);
}

// DLL Entry Points usable from Visual Basic
//  explicit export is required to export undecorated names.
#pragma comment(linker, "/export:InstallExVB=_InstallExVB@12")
extern "C" LPVOID  __stdcall InstallExVB(LPGETLOGFILE pfn, LPCTSTR lpcszTo, LPCTSTR lpcszSubject)
{
   return InstallEx(pfn, lpcszTo, lpcszSubject);
}

#pragma comment(linker, "/export:UninstallExVB=_UninstallExVB@4")
extern "C" void __stdcall UninstallExVB(LPVOID lpState)
{
   UninstallEx(lpState);
}

#pragma comment(linker, "/export:AddFileExVB=_AddFileExVB@12")
extern "C" void __stdcall AddFileExVB(LPVOID lpState, LPCTSTR lpFile, LPCTSTR lpDesc)
{
   AddFileEx(lpState, lpFile, lpDesc);
}

#pragma comment(linker, "/export:RemoveFileExVB=_RemoveFileExVB@8")
extern "C" void __stdcall RemoveFileExVB(LPVOID lpState, LPCTSTR lpFile)
{
   RemoveFileEx(lpState, lpFile);
}

#pragma comment(linker, "/export:AddRegistryHiveExVB=_AddRegistryHiveExVB@12")
extern "C" void __stdcall AddRegistryHiveExVB(LPVOID lpState, LPCTSTR lpRegistryHive, LPCTSTR lpDesc)
{
   AddRegistryHiveEx(lpState, lpRegistryHive, lpDesc);
}

#pragma comment(linker, "/export:RemoveRegistryHiveExVB=_RemoveRegistryHiveExVB@8")
extern "C" void __stdcall RemoveRegistryHiveExVB(LPVOID lpState, LPCTSTR lpRegistryHive)
{
   RemoveRegistryHiveEx(lpState, lpRegistryHive);
}

#pragma comment(linker, "/export:GenerateErrorReportExVB=_GenerateErrorReportExVB@12")
extern "C" void __stdcall GenerateErrorReportExVB(LPVOID lpState, PEXCEPTION_POINTERS pExInfo, BSTR message)
{
   GenerateErrorReportEx(lpState, pExInfo, message);
}

#pragma comment(linker, "/export:StackTraceVB=_StackTraceVB@20")
extern "C" void  __stdcall StackTraceVB(int numSkip, int depth, TraceCallbackFunction pFunction, CONTEXT *pContext, void *data)
{
	StackTrace(numSkip, depth, pFunction, pContext, data);
}

// Compatiblity interfaces
CRASHRPTAPI void Install(LPGETLOGFILE pfn, LPCTSTR lpcszTo, LPCTSTR lpcszSubject)
{
	(void) InstallEx(pfn, lpcszTo, lpcszSubject);
}

CRASHRPTAPI void Uninstall()
{
	UninstallEx(CCrashHandler::GetInstance());
}

CRASHRPTAPI void AddFile(LPCTSTR lpFile, LPCTSTR lpDesc)
{
   AddFileEx(CCrashHandler::GetInstance(), lpFile, lpDesc);
}

CRASHRPTAPI void RemoveFile(LPCTSTR lpFile)
{
   RemoveFileEx(CCrashHandler::GetInstance(), lpFile);
}

CRASHRPTAPI void AddRegistryHive(LPCTSTR lpRegistryHive, LPCTSTR lpDesc)
{
   AddRegistryHiveEx(CCrashHandler::GetInstance(), lpRegistryHive, lpDesc);
}

CRASHRPTAPI void RemoveRegistryHive(LPCTSTR lpRegistryHive)
{
   RemoveRegistryHiveEx(CCrashHandler::GetInstance(), lpRegistryHive);
}
CRASHRPTAPI void AddEventLog(LPCTSTR lpEventLog, LPCTSTR lpDesc)
{
   AddEventLogEx(CCrashHandler::GetInstance(), lpEventLog, lpDesc);
}

CRASHRPTAPI void RemoveEventLog(LPCTSTR lpEventLog)
{
   RemoveEventLogEx(CCrashHandler::GetInstance(), lpEventLog);
}

CRASHRPTAPI void GenerateErrorReport(BSTR message)
{
   GenerateErrorReportEx(CCrashHandler::GetInstance(), NULL, message);
}

#pragma comment(linker, "/export:InstallVB=_InstallVB@12")
extern "C" void __stdcall InstallVB(LPGETLOGFILE pfn, LPCTSTR lpTo, LPCTSTR lpSubject)
{
	Install(pfn, lpTo, lpSubject);
}

#pragma comment(linker, "/export:UninstallVB=_UninstallVB@0")
extern "C" void __stdcall UninstallVB()
{
	Uninstall();
}

#pragma comment(linker, "/export:AddFileVB=_AddFileVB@8")
extern "C" void __stdcall AddFileVB(LPCTSTR lpFile, LPCTSTR lpDesc)
{
	AddFile(lpFile, lpDesc);
}

#pragma comment(linker, "/export:RemoveFileVB=_RemoveFileVB@4")
extern "C" void __stdcall RemoveFileVB(LPCTSTR lpFile)
{
	RemoveFile(lpFile);
}

#pragma comment(linker, "/export:AddRegistryHiveVB=_AddRegistryHiveVB@8")
extern "C" void __stdcall AddRegistryHiveVB(LPCTSTR lpRegistryHive, LPCTSTR lpDesc)
{
	AddRegistryHive(lpRegistryHive, lpDesc);
}

#pragma comment(linker, "/export:RemoveRegistryHiveVB=_RemoveRegistryHiveVB@4")
extern "C" void __stdcall RemoveRegistryHiveVB(LPCTSTR lpRegistryHive)
{
	RemoveRegistryHive(lpRegistryHive);
}

#pragma comment(linker, "/export:AddEventLogVB=_AddEventLogVB@8")
extern "C" void __stdcall AddEventLogVB(LPCTSTR lpEventLog, LPCTSTR lpDesc)
{
	AddEventLog(lpEventLog, lpDesc);
}

#pragma comment(linker, "/export:RemoveEventLogVB=_RemoveEventLogVB@4")
extern "C" void __stdcall RemoveEventLogVB(LPCTSTR lpEventLog)
{
	RemoveEventLog(lpEventLog);
}

#pragma comment(linker, "/export:GenerateErrorReportVB=_GenerateErrorReportVB@4")
extern "C" void __stdcall GenerateErrorReportVB(BSTR message)
{
	GenerateErrorReport(message);
}

