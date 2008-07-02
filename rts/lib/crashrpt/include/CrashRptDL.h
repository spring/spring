///////////////////////////////////////////////////////////////////////////////
//
//  Module: CrashRptDL.h
//
//    Desc: Defines the interface for the CrashRpt.DLL, using a dynamically
//          loaded instance of the library. Can be used when CrashRpt.dll may
//          not be present on the system.
//
//          Note that all functions except GetInstanceDL() and
//          ReleaseInstanceDL() return an integer, which is non-zero ('true')
//          when the real DLL function was located and called. This does not
//          mean the actual DLL function succeeded, however.
//
//          Please don't get confused by the macro usage; all functions do end
//          in DL, even though the name supplied to CRASHRPT_DECLARE does not.
//
// Copyright (c) 2003 Michael Carruth
// Copyright (c) 2003 Grant McDorman
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _CRASHRPT_H_
#define _CRASHRPT_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>
#include <wtypes.h>     // BSTR

// Client crash callback
typedef BOOL (CALLBACK *LPGETLOGFILE) (LPVOID lpvState);
// Stack trace callback
typedef void (*TraceCallbackFunction)(DWORD address, const char *ImageName,
									  const char *FunctionName, DWORD functionDisp,
									  const char *Filename, DWORD LineNumber, DWORD lineDisp,
									  void *data);

// macro to create the inline forwarding function
#define CRASHRPT_DECLARE(function, declare1, declare2, arguments) \
 __inline int function##DL declare1 \
 { \
	 typedef void (*function##_t) declare2; \
	 function##_t p##function; \
	 p##function = (function##_t) GetProcAddress(hModule, #function); \
	 if (p##function != NULL) { \
		 p##function arguments; \
		 return 1; \
	 } \
	 return 0; \
 }



//-----------------------------------------------------------------------------
// GetInstanceDL
//    Returns the instance (module handle) for the CrashRpt DLL.
//
// Parameters
//    none
//
// Return Values
//    If the function succeeds, the return value is a module handle for the CrashRpt
//    shared library (CrashRpt.dll). This handle is required for all the other functions.
//
// Remarks
//    none
//
__inline 
HMODULE
GetInstanceDL()
{
	return LoadLibrary("CrashRpt");
}

//-----------------------------------------------------------------------------
// InstallDL
//    Initializes the library and optionally set the client crash callback and
//    sets up the email details.
//
// Parameters
//    hModule     State information returned from GetInstanceDL() (must not be NULL)
//    pfn         Client crash callback
//    lpTo        Email address to send crash report
//    lpSubject   Subject line to be used with email
//
// Return Values
//    non-zero if successful
//
// Remarks
//    Passing NULL for lpTo will disable the email feature and cause the crash 
//    report to be saved to disk.
//
CRASHRPT_DECLARE(Install, (IN HMODULE hModule, IN LPGETLOGFILE pfn, IN LPCTSTR lpTo OPTIONAL, IN LPCTSTR lpSubject OPTIONAL),  \
                            (LPGETLOGFILE pfn, LPCTSTR lpTo, LPCTSTR lpSubject),  \
                            (pfn, lpTo, lpSubject))

//-----------------------------------------------------------------------------
// UninstallDL
//    Uninstalls the unhandled exception filter set up in InstallDL().
//
// Parameters
//    hModule     Module handle returned from GetInstanceDL()
//
// Return Values
//    non-zero if successful
//
// Remarks
//    This call is optional.  The crash report library will automatically 
//    deinitialize when the library is unloaded.  Call this function to
//    unhook the exception filter manually.
//
CRASHRPT_DECLARE(Uninstall, (IN HMODULE hModule),  \
                            (), \
							())
//-----------------------------------------------------------------------------
// ReleaseInstanceDL
//    Releases the library.
//
// Parameters
//    hModule     Module handle returned from GetInstanceDL()
//
// Return Values
//    void
//
// Remarks
//    This will call UninstallDL before releasing the library.
//
__inline 
void
ReleaseInstanceDL(IN HMODULE hModule)
{
	UninstallDL(hModule);
	FreeLibrary(hModule);
}

//-----------------------------------------------------------------------------
// AddFileDL
//    Adds a file to the crash report.
//
// Parameters
//    hModule     Module handle returned from GetInstanceDL()
//    lpFile      Fully qualified file name
//    lpDesc      Description of file, used by details dialog
//
// Return Values
//    non-zero if successful
//
// Remarks
//    This function can be called anytime after Install() to add one or more
//    files to the generated crash report. If lpFile exactly matches
//    a previously added file, it is not added again.
//
CRASHRPT_DECLARE(AddFile, (IN HMODULE hModule, IN LPCTSTR lpFile, IN LPCTSTR lpDesc),  \
                            (LPCTSTR lpFile, LPCTSTR lpDesc),  \
                            (lpFile, lpDesc))

//-----------------------------------------------------------------------------
// RemoveFileDL
//    Removes a file from the crash report.
//
// Parameters
//    hModule     Module handle returned from GetInstanceDL()
//    lpFile      Fully qualified file name
//
// Return Values
//    non-zero if successful
//
// Remarks
//    The filename must exactly match that provided to AddFile.
//
CRASHRPT_DECLARE(RemoveFile, (IN HMODULE hModule, IN LPCTSTR lpFile),  \
                            (LPCTSTR lpFile),  \
                            (lpFile))

//-----------------------------------------------------------------------------
// AddRegistryHiveDL
//    Adds a RegistryHive to the crash report.
//
// Parameters
//    hModule     Module handle returned from GetInstanceDL()
//    lpRegistryHive      Fully qualified RegistryHive name
//    lpDesc      Description of RegistryHive, used by details dialog
//
// Return Values
//    non-zero if successful
//
// Remarks
//    This function can be called anytime after Install() to add one or more
//    RegistryHives to the generated crash report. If lpRegistryHive exactly matches
//    a previously added file, it is not added again.
//
CRASHRPT_DECLARE(AddRegistryHive, (IN HMODULE hModule, IN LPCTSTR lpRegistryHive, IN LPCTSTR lpDesc),  \
                            (LPCTSTR lpRegistryHive, LPCTSTR lpDesc),  \
                            (lpRegistryHive, lpDesc))

//-----------------------------------------------------------------------------
// RemoveRegistryHiveDL
//    Removes a RegistryHive from the crash report.
//
// Parameters
//    hModule     Module handle returned from GetInstanceDL()
//    lpRegistryHive      Fully qualified RegistryHive name
//
// Return Values
//    non-zero if successful
//
// Remarks
//    The RegistryHive name must exactly match that provided to AddRegistryHive.
//
CRASHRPT_DECLARE(RemoveRegistryHive, (IN HMODULE hModule, IN LPCTSTR lpRegistryHive),  \
                            (LPCTSTR lpRegistryHive),  \
                            (lpRegistryHive))

//-----------------------------------------------------------------------------
// AddEventLogDL
//    Adds an event log to the crash report.
//
// Parameters
//    hModule     Module handle returned from GetInstanceDL()
//    lpEventLog  Event log name ("Application", "Security", "System" or any other known to your system)
//    lpDesc      Description of event log, used by details dialog
//
// Return Values
//    non-zero if successful
//
// Remarks
//    This function can be called anytime after Install() to add one or more
//    event logs to the generated crash report. If lpEventLog exactly matches
//    a previously added file, it is not added again.
//
CRASHRPT_DECLARE(AddEventLog, (IN HMODULE hModule, IN LPCTSTR lpEventLog, IN LPCTSTR lpDesc),  \
                            (LPCTSTR lpEventLog, LPCTSTR lpDesc),  \
                            (lpEventLog, lpDesc))

//-----------------------------------------------------------------------------
// RemoveEventLogDL
//    Removes a EventLog from the crash report.
//
// Parameters
//    hModule     Module handle returned from GetInstanceDL()
//    lpEventLog      Fully qualified EventLog name
//
// Return Values
//    non-zero if successful
//
// Remarks
//    The EventLog name must exactly match that provided to AddEventLog.
//
CRASHRPT_DECLARE(RemoveEventLog, (IN HMODULE hModule, IN LPCTSTR lpEventLog),  \
                            (LPCTSTR lpEventLog),  \
                            (lpEventLog))

//-----------------------------------------------------------------------------
// GenerateErrorReportDL
//    Generates the crash report.
//
// Parameters
//    hModule     Module handle returned from GetInstanceDL()
//    pExInfo     Optional; exception information
//	  message     Optional; message to include in report
//
// Return Values
//    non-zero if successful
//
// Remarks
//    Call this function to manually generate a crash report.
//
__inline
int
GenerateErrorReportDL(
   IN HMODULE hModule,
   IN PEXCEPTION_POINTERS pExInfo OPTIONAL,
   IN BSTR message OPTIONAL
   )
{
	// we can't use GenerateErrorReport(), because that is lacking the PEXCEPTION_POINTERS parameter
	LPVOID instance;
    typedef LPVOID (*GetInstance_t) ();
    GetInstance_t pGetInstance;
    pGetInstance = (GetInstance_t) GetProcAddress(hModule, "GetInstance");
    if (pGetInstance != NULL) {
		typedef void (*GenerateErrorReportEx_t)(LPVOID lpState, PEXCEPTION_POINTERS pExInfo, BSTR message);
		GenerateErrorReportEx_t pGenerateErrorReportEx;

		instance = pGetInstance();
		pGenerateErrorReportEx = (GenerateErrorReportEx_t) GetProcAddress(hModule, "GenerateErrorReportEx");
		if (pGenerateErrorReportEx != NULL) {
			pGenerateErrorReportEx(instance, pExInfo, message);
			return 1;
		}
    }
    return 0;
}


//-----------------------------------------------------------------------------
// StackTraceDL
//    Creates a stack trace.
//
// Parameters
//    hModule     Module handle returned from GetInstanceDL()
//    numSkip     Number of initial stack frames to skip
//    depth       Number of stack frames to process
//	  pFunction   Optional; function to call for each frame
//    pContext    Optional; stack context to trace
//
// Return Values
//    void
//
// Remarks
//    Call this function to manually generate a stack trace. If
//    pFunction is not supplied, stack trace frames are output using
//    OutputDebugString. Can be called without installing (InstallDL)
//    CrashRpt.
//
CRASHRPT_DECLARE(StackTrace, (IN HMODULE hModule, IN int numSkip, IN int depth, IN TraceCallbackFunction pFunction OPTIONAL, IN CONTEXT * pContext OPTIONAL, IN LPVOID data OPTIONAL),  \
                            (int numSkip, int depth, TraceCallbackFunction pFunction, CONTEXT * pContext, LPVOID data),  \
                            (numSkip, depth, pFunction, pContext, data))

#undef CRASHRPT_DECLARE

#endif
