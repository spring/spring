///////////////////////////////////////////////////////////////////////////////
//
//  Module: CrashHandler.cpp
//
//    Desc: See CrashHandler.h
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CrashHandler.h"
#include "zlibcpp.h"
#include "excprpt.h"
#include "maindlg.h"
#include "mailmsg.h"
#include "WriteRegistry.h"

#include <windows.h>

#include <algorithm>

// global app module
CAppModule _Module;

// maps crash objects to processes
CSimpleMap<DWORD, CCrashHandler*> _crashStateMap;

// unhandled exception callback set with SetUnhandledExceptionFilter()
LONG WINAPI CustomUnhandledExceptionFilter(PEXCEPTION_POINTERS pExInfo)
{
	OutputDebugString("Exception\n");
   if (EXCEPTION_BREAKPOINT == pExInfo->ExceptionRecord->ExceptionCode)
   {
	   // Breakpoint. Don't treat this as a normal crash.
	   return EXCEPTION_CONTINUE_SEARCH;
   }

   BOOL result = _crashStateMap.Lookup(GetCurrentProcessId())->GenerateErrorReport(pExInfo, NULL);

   // If we're in a debugger, return EXCEPTION_CONTINUE_SEARCH to cause the debugger to stop;
   // or if GenerateErrorReport returned FALSE (i.e. drop into debugger).
   return (!result || IsDebuggerPresent()) ? EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER;
}

CCrashHandler * CCrashHandler::GetInstance()
{
	CCrashHandler *instance;
	instance = _crashStateMap.Lookup(GetCurrentProcessId());
	if (instance == NULL) {
		// will register
		instance = new CCrashHandler();
	}
	return instance;
}

CCrashHandler::CCrashHandler():
	m_oldFilter(NULL),
	m_lpfnCallback(NULL),
	m_pid(GetCurrentProcessId()),
	m_sTo(_T("")),
	m_sSubject(_T("")),
	m_ipc_event(NULL),
	m_rpt(NULL),
	m_installed(false),
	m_hModule(NULL)
{
   // wtl initialization stuff...
	HRESULT hRes = ::CoInitialize(NULL);
	ATLASSERT(SUCCEEDED(hRes));

   _crashStateMap.Add(m_pid, this);
}

void CCrashHandler::Install(LPGETLOGFILE lpfn, LPCTSTR lpcszTo, LPCTSTR lpcszSubject)
{
	OutputDebugString("::Install\n");
	if (m_installed) {
		Uninstall();
	}
   // save user supplied callback
   m_lpfnCallback = lpfn;
   // save optional email info
   m_sTo = lpcszTo;
   m_sSubject = lpcszSubject;


   // add this filter in the exception callback chain
   m_oldFilter = SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);
/*m_oldErrorMode=*/ SetErrorMode( SEM_FAILCRITICALERRORS );

   m_installed = true;
}

void CCrashHandler::Uninstall()
{
	OutputDebugString("Uninstall\n");
   // reset exception callback (to previous filter, which can be NULL)
   SetUnhandledExceptionFilter(m_oldFilter);
   m_installed = false;
}

CCrashHandler::~CCrashHandler()
{

	Uninstall();

   _crashStateMap.Remove(m_pid);

	::CoUninitialize();

}

void CCrashHandler::AddFile(LPCTSTR lpFile, LPCTSTR lpDesc)
{
   // make sure we don't already have the file
   RemoveFile(lpFile);
   // make sure the file exists
   HANDLE hFile = ::CreateFile(
					 lpFile,
					 GENERIC_READ,
					 FILE_SHARE_READ | FILE_SHARE_WRITE,
					 NULL,
					 OPEN_EXISTING,
					 FILE_ATTRIBUTE_NORMAL,
					 0);
   if (hFile != INVALID_HANDLE_VALUE)
   {
	  // add file to report
	  m_files.push_back(TStrStrPair(lpFile, lpDesc));
	  ::CloseHandle(hFile);
   }
}

void CCrashHandler::RemoveFile(LPCTSTR lpFile)
{
	TStrStrVector::iterator iter;
	for (iter = m_files.begin(); iter != m_files.end(); ++iter) {
		if ((*iter).first == lpFile) {
			iter = m_files.erase(iter);
		}
	}
}

void CCrashHandler::AddRegistryHive(LPCTSTR lpRegistryHive, LPCTSTR lpDesc)
{
   // make sure we don't already have the RegistryHive
   RemoveRegistryHive(lpRegistryHive);
   // Unfortunately, we have no easy way of verifying the existence of
   // the registry hive.
   // add RegistryHive to report
   m_registryHives.push_back(TStrStrPair(lpRegistryHive, lpDesc));
}

void CCrashHandler::RemoveRegistryHive(LPCTSTR lpRegistryHive)
{
	TStrStrVector::iterator iter;
	for (iter = m_registryHives.begin(); iter != m_registryHives.end(); ++iter) {
		if ((*iter).first == lpRegistryHive) {
			iter = m_registryHives.erase(iter);
		}
	}
}

void CCrashHandler::AddEventLog(LPCTSTR lpEventLog, LPCTSTR lpDesc)
{
   // make sure we don't already have the EventLog
   RemoveEventLog(lpEventLog);
   // Unfortunately, we have no easy way of verifying the existence of
   // the event log..
   // add EventLog to report
   m_eventLogs.push_back(TStrStrPair(lpEventLog, lpDesc));
}

void CCrashHandler::RemoveEventLog(LPCTSTR lpEventLog)
{
	TStrStrVector::iterator iter;
	for (iter = m_eventLogs.begin(); iter != m_eventLogs.end(); ++iter) {
		if ((*iter).first == lpEventLog) {
			iter = m_eventLogs.erase(iter);
		}
	}
}

DWORD WINAPI CCrashHandler::DialogThreadExecute(LPVOID pParam)
{
   // New thread. This will display the dialog and handle the result.
   CCrashHandler * self = reinterpret_cast<CCrashHandler *>(pParam);
   CMainDlg          mainDlg;
   CString sTempFileName = CUtility::getTempFileName();
   CZLib             zlib;

   // delete existing copy, if any
   DeleteFile(sTempFileName);

   // zip the report
   if (!zlib.Open(sTempFileName))
      return TRUE;
   
   // add report files to zip
   TStrStrVector::iterator cur = self->m_files.begin();
   for (cur = self->m_files.begin(); cur != self->m_files.end(); cur++)
      zlib.AddFile((*cur).first);

   zlib.Close();

   // display main dialog
   // initialize WTL
   HRESULT hRes = _Module.Init(NULL, GetModuleHandle("CrashRpt.dll"));
   ATLASSERT(SUCCEEDED(hRes));

	::DefWindowProc(NULL, 0, 0, 0L);

   mainDlg.m_pUDFiles = &self->m_files;
   mainDlg.m_sendButton = !self->m_sTo.IsEmpty();
   int status = mainDlg.DoModal();
   if (IDOK == status || IDC_SAVE == status)
   {
      if (IDC_SAVE == status || self->m_sTo.IsEmpty() || 
          !self->MailReport(*self->m_rpt, sTempFileName, mainDlg.m_sEmail, mainDlg.m_sDescription))
      {
		  // write user data to file if to be supplied
		  if (!self->m_userDataFile.IsEmpty()) {
			   HANDLE hFile = ::CreateFile(
								 self->m_userDataFile,
								 GENERIC_READ | GENERIC_WRITE,
								 FILE_SHARE_READ | FILE_SHARE_WRITE,
								 NULL,
								 CREATE_ALWAYS,
								 FILE_ATTRIBUTE_NORMAL,
								 0);
			   if (hFile != INVALID_HANDLE_VALUE)
			   {
				   static const char e_mail[] = "E-mail:";
				   static const char newline[] = "\r\n";
				   static const char description[] = "\r\n\r\nDescription:";
				   DWORD writtenBytes;
				   ::WriteFile(hFile, e_mail, sizeof(e_mail) - 1, &writtenBytes, NULL);
				   ::WriteFile(hFile, mainDlg.m_sEmail, mainDlg.m_sEmail.GetLength(), &writtenBytes, NULL);
				   ::WriteFile(hFile, description, sizeof(description) - 1, &writtenBytes, NULL);
				   ::WriteFile(hFile, mainDlg.m_sDescription, mainDlg.m_sDescription.GetLength(), &writtenBytes, NULL);
				   ::WriteFile(hFile, newline, sizeof(newline) - 1, &writtenBytes, NULL);
				   ::CloseHandle(hFile);
				  // redo zip file to add user data
				   // delete existing copy, if any
				   DeleteFile(sTempFileName);

				   // zip the report
				   if (!zlib.Open(sTempFileName))
					  return TRUE;
   
				   // add report files to zip
				   TStrStrVector::iterator cur = self->m_files.begin();
				   for (cur = self->m_files.begin(); cur != self->m_files.end(); cur++)
					  zlib.AddFile((*cur).first);

				   zlib.Close();
			   }
		  }
         self->SaveReport(*self->m_rpt, sTempFileName);
      }
   }
   // uninitialize
   _Module.Term();

   DeleteFile(sTempFileName);

   self->m_wantDebug = IDC_DEBUG == status;
   // signal main thread to resume
   ::SetEvent(self->m_ipc_event);
   // set flag for debugger break

   // exit thread
   ::ExitThread(0);
   // keep compiler happy.
//   return TRUE;
}

BOOL CCrashHandler::GenerateErrorReport(PEXCEPTION_POINTERS pExInfo, BSTR message)
{
   CExceptionReport  rpt(pExInfo, message);
   unsigned int      i;
   // save state of file list prior to generating report
   TStrStrVector     save_m_files = m_files;
	char temp[_MAX_PATH];

	GetTempPath(sizeof temp, temp);


   // let client add application specific files to report
   if (m_lpfnCallback && !m_lpfnCallback(this))
      return TRUE;

   m_rpt = &rpt;

   // if no e-mail address, add file to contain user data
   m_userDataFile = "";
   if (m_sTo.IsEmpty()) {
	   m_userDataFile = temp + CString("\\") + CUtility::getAppName() + "_UserInfo.txt";
	   HANDLE hFile = ::CreateFile(
						 m_userDataFile,
						 GENERIC_READ | GENERIC_WRITE,
						 FILE_SHARE_READ | FILE_SHARE_WRITE,
						 NULL,
						 CREATE_ALWAYS,
						 FILE_ATTRIBUTE_NORMAL,
						 0);
	   if (hFile != INVALID_HANDLE_VALUE)
	   {
		   static const char description[] = "Your e-mail and description will go here.";
		   DWORD writtenBytes;
		   ::WriteFile(hFile, description, sizeof(description)-1, &writtenBytes, NULL);
		   ::CloseHandle(hFile);
		   m_files.push_back(TStrStrPair(m_userDataFile, LoadResourceString(IDS_USER_DATA)));
	   } else {
		   m_userDataFile = "";
	   }
   }



   // add crash files to report
   CString crashFile = rpt.getCrashFile();
   CString crashLog = rpt.getCrashLog();
   m_files.push_back(TStrStrPair(crashFile, LoadResourceString(IDS_CRASH_DUMP)));
   m_files.push_back(TStrStrPair(crashLog, LoadResourceString(IDS_CRASH_LOG)));

   // Export registry hives and add to report
   std::vector<CString> extraFiles;
   CString file;
   CString number;
   TStrStrVector::iterator iter;
   int n = 0;

   for (iter = m_registryHives.begin(); iter != m_registryHives.end(); iter++) {
	   ++n;
	   number.Format("%d", n);
	   file = temp + CString("\\") + CUtility::getAppName() + "_registry" + number + ".reg";
	   ::DeleteFile(file);

	   // we want to export in a readable format. Unfortunately, RegSaveKey saves in a binary
	   // form, so let's use our own function.
	   if (WriteRegistryTreeToFile((*iter).first, file)) {
		   extraFiles.push_back(file);
		   m_files.push_back(TStrStrPair(file, (*iter).second));
	   } else {
		   OutputDebugString("Could not write registry hive\n");
	   }
   }
   //
   // Add the specified event log(s). Note that this will not work on Win9x/WinME.
   //
   for (iter = m_eventLogs.begin(); iter != m_eventLogs.end(); iter++) {
		HANDLE h;
		h = OpenEventLog( NULL,    // use local computer
				 (*iter).first);   // source name
		if (h != NULL) {

			file = temp + CString("\\") + CUtility::getAppName() +  "_" + (*iter).first + ".evt";

			DeleteFile(file);

			if (BackupEventLog(h, file)) {
				m_files.push_back(TStrStrPair(file, (*iter).second));
			   extraFiles.push_back(file);
			} else {
				OutputDebugString("could not backup log\n");
			}
			CloseEventLog(h);
		} else {
			OutputDebugString("could not open log\n");
		}
   }
 

   // add symbol files to report
   for (i = 0; i < (UINT)rpt.getNumSymbolFiles(); i++)
      m_files.push_back(TStrStrPair((LPCTSTR)rpt.getSymbolFile(i), 
      CString((LPCTSTR)IDS_SYMBOL_FILE)));
 
   // Start a new thread to display the dialog, and then wait
   // until it completes
   m_ipc_event = ::CreateEvent(NULL, FALSE, FALSE, "ACrashHandlerEvent");
   DWORD threadId;
   ::CreateThread(NULL, 0, DialogThreadExecute,
	   reinterpret_cast<LPVOID>(this), 0, &threadId);
   ::WaitForSingleObject(m_ipc_event, INFINITE);
   CloseHandle(m_ipc_event);

   // clean up - delete files we created
   ::DeleteFile(crashFile);
   ::DeleteFile(crashLog);
   if (!m_userDataFile.IsEmpty()) {
	   ::DeleteFile(m_userDataFile);
   }

   std::vector<CString>::iterator file_iter;
   for (file_iter = extraFiles.begin(); file_iter != extraFiles.end(); file_iter++) {
	   ::DeleteFile(*file_iter);
   }

   // restore state of file list
   m_files = save_m_files;

   m_rpt = NULL;

   return !m_wantDebug;
}

BOOL CCrashHandler::SaveReport(CExceptionReport&, LPCTSTR lpcszFile)
{
   // let user more zipped report
   return (CopyFile(lpcszFile, CUtility::getSaveFileName(), TRUE));
}

BOOL CCrashHandler::MailReport(CExceptionReport&, LPCTSTR lpcszFile,
                               LPCTSTR lpcszEmail, LPCTSTR lpcszDesc)
{
   CMailMsg msg;
   msg
      .SetTo(m_sTo)
      .SetFrom(lpcszEmail)
      .SetSubject(m_sSubject.IsEmpty()?_T("Incident Report"):m_sSubject)
      .SetMessage(lpcszDesc)
      .AddAttachment(lpcszFile, CUtility::getAppName() + _T(".zip"));

   return (msg.Send());
}

CString CCrashHandler::LoadResourceString(UINT id)
{
	static int address;
	char buffer[512];
	if (m_hModule == NULL) {
		m_hModule = GetModuleHandle("CrashRpt.dll");
	}
	buffer[0] = '\0';
	if (m_hModule) {
		LoadString(m_hModule, id, buffer, sizeof buffer);
	}
	return buffer;
}
