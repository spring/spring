/* Author: Tobi Vollebregt */

#include "StdAfx.h"
#include "lib/gml/gml.h"
#include <windows.h>
#include <process.h>
#include <imagehlp.h>
#include <SDL.h> // for SDL_Quit
#include "CrashHandler.h"
#include "Game/GameVersion.h"
#include "LogOutput.h"
#include "NetProtocol.h"
#include "Util.h"

namespace CrashHandler {
	namespace Win32 {

		
void SigAbrtHandler(int signal)
{
	// cause an exception if on windows
	// TODO FIXME do a proper stacktrace dump here
	*((int*)(0)) = 0;
}

// Set this to the desired printf style output function.
// Currently we write through the logOutput class to infolog.txt
#define PRINT logOutput.Print

/** Convert exception code to human readable string. */
static const char *ExceptionName(DWORD exceptionCode)
{
	switch (exceptionCode) {
		case EXCEPTION_ACCESS_VIOLATION:         return "Access violation";
		case EXCEPTION_DATATYPE_MISALIGNMENT:    return "Datatype misalignment";
		case EXCEPTION_BREAKPOINT:               return "Breakpoint";
		case EXCEPTION_SINGLE_STEP:              return "Single step";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "Array bounds exceeded";
		case EXCEPTION_FLT_DENORMAL_OPERAND:     return "Float denormal operand";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "Float divide by zero";
		case EXCEPTION_FLT_INEXACT_RESULT:       return "Float inexact result";
		case EXCEPTION_FLT_INVALID_OPERATION:    return "Float invalid operation";
		case EXCEPTION_FLT_OVERFLOW:             return "Float overflow";
		case EXCEPTION_FLT_STACK_CHECK:          return "Float stack check";
		case EXCEPTION_FLT_UNDERFLOW:            return "Float underflow";
		case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "Integer divide by zero";
		case EXCEPTION_INT_OVERFLOW:             return "Integer overflow";
		case EXCEPTION_PRIV_INSTRUCTION:         return "Privileged instruction";
		case EXCEPTION_IN_PAGE_ERROR:            return "In page error";
		case EXCEPTION_ILLEGAL_INSTRUCTION:      return "Illegal instruction";
		case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "Noncontinuable exception";
		case EXCEPTION_STACK_OVERFLOW:           return "Stack overflow";
		case EXCEPTION_INVALID_DISPOSITION:      return "Invalid disposition";
		case EXCEPTION_GUARD_PAGE:               return "Guard page";
		case EXCEPTION_INVALID_HANDLE:           return "Invalid handle";
	}
	return "Unknown exception";
}

/** Print out a stacktrace. */
static void Stacktrace(LPEXCEPTION_POINTERS e) {
	PIMAGEHLP_SYMBOL pSym;
	STACKFRAME sf;
	HANDLE process, thread;
	DWORD dwModBase, Disp;
	BOOL more = FALSE;
	int count = 0;
	char modname[MAX_PATH];

	pSym = (PIMAGEHLP_SYMBOL)GlobalAlloc(GMEM_FIXED, 16384);

	ZeroMemory(&sf, sizeof(sf));
	sf.AddrPC.Offset = e->ContextRecord->Eip;
	sf.AddrStack.Offset = e->ContextRecord->Esp;
	sf.AddrFrame.Offset = e->ContextRecord->Ebp;
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrStack.Mode = AddrModeFlat;
	sf.AddrFrame.Mode = AddrModeFlat;

	process = GetCurrentProcess();
	thread = GetCurrentThread();

	while(1) {
		more = StackWalk(
			IMAGE_FILE_MACHINE_I386, // TODO: fix this for 64 bit windows?
			process,
			thread,
			&sf,
			e->ContextRecord,
			NULL,
			SymFunctionTableAccess,
			SymGetModuleBase,
			NULL
		);
		if(!more || sf.AddrFrame.Offset == 0) {
			break;
		}

		dwModBase = SymGetModuleBase(process, sf.AddrPC.Offset);

		if(dwModBase) {
			GetModuleFileName((HINSTANCE)dwModBase, modname, MAX_PATH);
		} else {
			strcpy(modname, "Unknown");
		}

		pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
		pSym->MaxNameLength = MAX_PATH;

		if(SymGetSymFromAddr(process, sf.AddrPC.Offset, &Disp, pSym)) {
			// This is the code path taken on VC if debugging syms are found.
			PRINT("(%d) %s(%s+%#0x) [0x%08X]\n", count, modname, pSym->Name, Disp, sf.AddrPC.Offset);
		} else {
			// This is the code path taken on MinGW, and VC if no debugging syms are found.
			PRINT("(%d) %s [0x%08X]\n", count, modname, sf.AddrPC.Offset);
		}
		++count;
	}
	GlobalFree(pSym);
}

/** Callback for SymEnumerateModules */
#if _MSC_VER >= 1500
static BOOL CALLBACK EnumModules(PCSTR moduleName, ULONG baseOfDll, PVOID userContext)
{
	PRINT("0x%08x\t%s\n", baseOfDll, moduleName);
	return TRUE;
}
#else
static BOOL CALLBACK EnumModules(LPSTR moduleName, DWORD baseOfDll, PVOID userContext)
{
	PRINT("0x%08x\t%s\n", baseOfDll, moduleName);
	return TRUE;
}
#endif

/** Called by windows if an exception happens. */
static LONG CALLBACK ExceptionHandler(LPEXCEPTION_POINTERS e)
{
	// Prologue.
	logOutput.RemoveAllSubscribers();
#ifdef USE_GML
	PRINT("Spring %s MT (%d threads) has crashed.", SpringVersion::GetFull().c_str(), gmlThreadCount);
#else
	PRINT("Spring %s has crashed.", SpringVersion::GetFull().c_str());
#endif
	// Initialize IMAGEHLP.DLL.
	SymInitialize(GetCurrentProcess(), ".", TRUE);

	// Record exception info.
	PRINT("Exception: %s (0x%08x)\n", ExceptionName(e->ExceptionRecord->ExceptionCode), e->ExceptionRecord->ExceptionCode);
	PRINT("Exception Address: 0x%08x\n", e->ExceptionRecord->ExceptionAddress);

	// Record list of loaded DLLs.
	PRINT("DLL information:\n");
	SymEnumerateModules(GetCurrentProcess(), EnumModules, NULL);

	// Record stacktrace.
	PRINT("Stacktrace:\n");
	Stacktrace(e);

	// Unintialize IMAGEHLP.DLL
	SymCleanup(GetCurrentProcess());

	// Cleanup.
	SDL_Quit();
	logOutput.End();  // Stop writing to log.
	// FIXME: update closing of demo to new netcode

	// Inform user.
	char dir[MAX_PATH], msg[MAX_PATH+200];
	GetCurrentDirectory(sizeof(dir) - 1, dir);
	SNPRINTF(msg, sizeof(msg),
		"Spring has crashed.\n\n"
		"A stacktrace has been written to:\n"
		"%s\\infolog.txt", dir);
	MessageBox(NULL, msg, "Spring: Unhandled exception", 0);

	// this seems to silently close the application
	return EXCEPTION_EXECUTE_HANDLER;

	// this triggers the microsoft "application has crashed" error dialog
	//return EXCEPTION_CONTINUE_SEARCH;

	// in practice, 100% CPU usage but no continuation of execution
	// (tested segmentation fault and division by zero)
	//return EXCEPTION_CONTINUE_EXECUTION;
}

/** Install crash handler. */
void Install()
{
	SetUnhandledExceptionFilter(ExceptionHandler);
	signal(SIGABRT, SigAbrtHandler);
}

/** Uninstall crash handler. */
void Remove()
{
	SetUnhandledExceptionFilter(NULL);
	signal(SIGABRT, SIG_DFL);
}

};
};
