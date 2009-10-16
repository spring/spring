/* Author: Tobi Vollebregt */

#include "StdAfx.h"
#include "lib/gml/gml.h"
#include <windows.h>
#include <process.h>
#include <imagehlp.h>
#include <signal.h>
#include <SDL.h> // for SDL_Quit
#include "CrashHandler.h"
#include "Game/GameVersion.h"
#include "LogOutput.h"
#include "NetProtocol.h"
#include "Util.h"
#include "ConfigHandler.h"
#include <boost/thread/thread.hpp>

#if defined(USE_GML) && GML_ENABLE_SIM
extern volatile int gmlMultiThreadSim;
#endif

HANDLE simthread = INVALID_HANDLE_VALUE;
HANDLE drawthread = INVALID_HANDLE_VALUE;

namespace CrashHandler {
	namespace Win32 {

boost::thread* hangdetectorthread = NULL;
volatile int keepRunning = 1;
// watchdog timers
unsigned volatile simwdt = 0, drawwdt = 0;
int hangTimeout = 0;

		
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
static void Stacktrace(LPEXCEPTION_POINTERS e, HANDLE hThread = INVALID_HANDLE_VALUE) {
	PIMAGEHLP_SYMBOL pSym;
	STACKFRAME sf;
	HANDLE process, thread;
	DWORD dwModBase, Disp;
	BOOL more = FALSE;
	int count = 0;
	char modname[MAX_PATH];

	pSym = (PIMAGEHLP_SYMBOL)GlobalAlloc(GMEM_FIXED, 16384);

	BOOL suspended = FALSE;
	CONTEXT c;
	if(e) {
		c = *e->ContextRecord;
		thread = GetCurrentThread();
	}
	else {
		// FIXME: The current thread may deadlock with the suspended thread during stack dumping
		SuspendThread(hThread);
		suspended = TRUE;
		memset(&c, 0, sizeof(CONTEXT));
		c.ContextFlags = CONTEXT_FULL;
		// FIXME: This does not work if you want to dump the current thread's stack
		if (!GetThreadContext(hThread, &c)) {
			ResumeThread(hThread);
			return;
		}
		thread = hThread;
	}

	ZeroMemory(&sf, sizeof(sf));
	sf.AddrPC.Offset = c.Eip;
	sf.AddrStack.Offset = c.Esp;
	sf.AddrFrame.Offset = c.Ebp;
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrStack.Mode = AddrModeFlat;
	sf.AddrFrame.Mode = AddrModeFlat;

	process = GetCurrentProcess();

	while(1) {
		more = StackWalk(
			IMAGE_FILE_MACHINE_I386, // TODO: fix this for 64 bit windows?
			process,
			thread,
			&sf,
			&c,
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

	if(suspended)
		ResumeThread(hThread);

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

/** Print stack traces for relevant threads. */
void HangHandler()
{
#ifdef USE_GML
	PRINT("Hang detection triggered for Spring %s MT (%d threads).", SpringVersion::GetFull().c_str(), gmlThreadCount);
#else
	PRINT("Hang detection triggered for Spring %s.", SpringVersion::GetFull().c_str());
#endif
	// Initialize IMAGEHLP.DLL.
	SymInitialize(GetCurrentProcess(), ".", TRUE);

	// Record list of loaded DLLs.
	PRINT("DLL information:\n");
	SymEnumerateModules(GetCurrentProcess(), EnumModules, NULL);


	if(drawthread != INVALID_HANDLE_VALUE) {
		// Record stacktrace.
		PRINT("Stacktrace:\n");
		Stacktrace(NULL, drawthread);
	}

#if defined(USE_GML) && GML_ENABLE_SIM
	if(gmlMultiThreadSim && simthread != INVALID_HANDLE_VALUE) {
		PRINT("Stacktrace (sim):\n");
		Stacktrace(NULL, simthread);
	}
#endif

	// Unintialize IMAGEHLP.DLL
	SymCleanup(GetCurrentProcess());

	logOutput.Flush();
}

void HangDetector() {
	while(keepRunning) {
		unsigned curwdt = SDL_GetTicks();
#if defined(USE_GML) && GML_ENABLE_SIM
		if(gmlMultiThreadSim) {
			unsigned cursimwdt = simwdt;
			if(cursimwdt && (curwdt - cursimwdt) > hangTimeout * 1000) {
				HangHandler();
				simwdt = cursimwdt;
			}
		}
#endif
		unsigned curdrawwdt = drawwdt;
		if(curdrawwdt && (curwdt - curdrawwdt) > hangTimeout * 1000) {
			HangHandler();
			drawwdt = curdrawwdt;
		}
		SDL_Delay(1000);
	}
}

void InstallHangHandler() {
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
					&drawthread, 0, TRUE, DUPLICATE_SAME_ACCESS);
	hangTimeout = configHandler->Get("HangTimeout", 0);
#ifdef USE_GML
	if(hangTimeout == 0) // HangTimeout = -1 to force disable hang detection in MT build
		hangTimeout = 10;
#endif
	if(hangTimeout > 0)
		hangdetectorthread = new boost::thread(&HangDetector);
}

void ClearDrawWDT(bool disable) {
	drawwdt = disable ? 0 : SDL_GetTicks();
}

void ClearSimWDT(bool disable) {
	simwdt = disable ? 0 : SDL_GetTicks();
}

void UninstallHangHandler() {
	if(hangdetectorthread) {
		keepRunning = 0;
		hangdetectorthread->join();
		delete hangdetectorthread;
	}
	if(drawthread != INVALID_HANDLE_VALUE)
		CloseHandle(drawthread);
	if(simthread != INVALID_HANDLE_VALUE)
		CloseHandle(simthread);
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
