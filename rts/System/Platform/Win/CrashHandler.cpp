/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "lib/gml/gml.h"
#include <windows.h>
#include <process.h>
#include <imagehlp.h>
#include <signal.h>
#include <boost/thread/thread.hpp>

#include "System/Platform/CrashHandler.h"
#include "System/Platform/errorhandler.h"

#include "ConfigHandler.h"
#include "LogOutput.h"
#include "myTime.h"
#include "NetProtocol.h"
#include "seh.h"
#include "Util.h"
#include "Game/GameVersion.h"

#define BUFFER_SIZE 2048

#if defined(USE_GML) && GML_ENABLE_SIM
extern volatile int gmlMultiThreadSim;
#endif

HANDLE simthread = INVALID_HANDLE_VALUE; // used in gmlsrv.h as well
HANDLE drawthread = INVALID_HANDLE_VALUE;
#define MAX_STACK_DEPTH 4096


namespace CrashHandler {

boost::thread* hangdetectorthread = NULL;
volatile int keepRunning = 1;
// watchdog timers
volatile spring_time simwdt = spring_notime, drawwdt = spring_notime;
spring_time hangTimeout = spring_msecs(0);
volatile bool gameLoading = false;

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

static void initImageHlpDll() {

	char userSearchPath[8];
	STRCPY(userSearchPath, ".");
	// Initialize IMAGEHLP.DLL
	SymInitialize(GetCurrentProcess(), userSearchPath, TRUE);
}

/** Print out a stacktrace. */
static void Stacktrace(LPEXCEPTION_POINTERS e, HANDLE hThread = INVALID_HANDLE_VALUE) {
	PIMAGEHLP_SYMBOL pSym;
	STACKFRAME sf;
	HANDLE process, thread;
	DWORD dwModBase, Disp, dwModAddrToPrint;
	BOOL more = FALSE;
	int count = 0;
	char modname[MAX_PATH];

	pSym = (PIMAGEHLP_SYMBOL)GlobalAlloc(GMEM_FIXED, 16384);

	BOOL suspended = FALSE;
	CONTEXT c;
	if (e) {
		c = *e->ContextRecord;
		thread = GetCurrentThread();
	} else {
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

	// use globalalloc to reduce risk for allocator related deadlock
	char* printstrings = (char*)GlobalAlloc(GMEM_FIXED, 0);

	bool containsOglDll = false;
	while (true) {
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
		if (!more || sf.AddrFrame.Offset == 0 || count > MAX_STACK_DEPTH) {
			break;
		}

		dwModBase = SymGetModuleBase(process, sf.AddrPC.Offset);

		if (dwModBase) {
			GetModuleFileName((HINSTANCE)dwModBase, modname, MAX_PATH);
		} else {
			strcpy(modname, "Unknown");
		}

		pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
		pSym->MaxNameLength = MAX_PATH;

		char* printstringsnew = (char*) GlobalAlloc(GMEM_FIXED, (count + 1) * BUFFER_SIZE);
		memcpy(printstringsnew, printstrings, count * BUFFER_SIZE);
		GlobalFree(printstrings);
		printstrings = printstringsnew;

		if (SymGetSymFromAddr(process, sf.AddrPC.Offset, &Disp, pSym)) {
			// This is the code path taken on VC if debugging syms are found.
			SNPRINTF(printstrings + count * BUFFER_SIZE, BUFFER_SIZE, "(%d) %s(%s+%#0lx) [0x%08lX]", count, modname, pSym->Name, Disp, sf.AddrPC.Offset);
		} else {
			// This is the code path taken on MinGW, and VC if no debugging syms are found.
			if (strstr(modname, ".exe")) {
				// for the .exe, we need the absolute address
				dwModAddrToPrint = sf.AddrPC.Offset;
			} else {
				// for DLLs, we need the module-internal/relative address
				dwModAddrToPrint = sf.AddrPC.Offset - dwModBase;
			}
			SNPRINTF(printstrings + count * BUFFER_SIZE, BUFFER_SIZE, "(%d) %s [0x%08lX]", count, modname, dwModAddrToPrint);
		}

		// OpenGL lib names (ATI): "atioglxx.dll" "atioglx2.dll"
		containsOglDll = containsOglDll || strstr(modname, "atiogl");
		// OpenGL lib names (Nvidia): "nvoglnt.dll" "nvoglv32.dll" "nvoglv64.dll" (last one is a guess)
		containsOglDll = containsOglDll || strstr(modname, "nvogl");
		// OpenGL lib names (Intel): "ig4dev32.dll" "ig4dev64.dll"
		containsOglDll = containsOglDll || strstr(modname, "ig4dev");

		++count;
	}

	if (suspended) {
		ResumeThread(hThread);
	}

	if (containsOglDll) {
		PRINT("This stack trace indicates a problem with your graphic card driver. "
		      "Please try upgrading or downgrading it. "
		      "Specifically recommended is the latest driver, and one that is as old as your graphic card. "
		      "Make sure to use a driver removal utility, before installing other drivers.");
	}

	for (int i = 0; i < count; ++i) {
		PRINT("%s", printstrings + i * BUFFER_SIZE);
	}

	GlobalFree(printstrings);

	GlobalFree(pSym);
}

/** Callback for SymEnumerateModules */
#if _MSC_VER >= 1500
static BOOL CALLBACK EnumModules(PCSTR moduleName, ULONG baseOfDll, PVOID userContext)
{
	PRINT("0x%08lx\t%s", baseOfDll, moduleName);
	return TRUE;
}
#else // _MSC_VER >= 1500
static BOOL CALLBACK EnumModules(LPSTR moduleName, DWORD baseOfDll, PVOID userContext)
{
	PRINT("0x%08lx\t%s", baseOfDll, moduleName);
	return TRUE;
}
#endif // _MSC_VER >= 1500

/** Called by windows if an exception happens. */
static LONG CALLBACK ExceptionHandler(LPEXCEPTION_POINTERS e)
{
	// Prologue.
	logOutput.SetSubscribersEnabled(false);
	PRINT("Spring %s has crashed.", SpringVersion::GetFull().c_str());
#ifdef USE_GML
	PRINT("MT with %d threads.", gmlThreadCount);
#endif
	initImageHlpDll();

	// Record exception info.
	PRINT("Exception: %s (0x%08lx)", ExceptionName(e->ExceptionRecord->ExceptionCode), e->ExceptionRecord->ExceptionCode);
	PRINT("Exception Address: 0x%08lx", (unsigned long int) (PVOID) e->ExceptionRecord->ExceptionAddress);

	// Record list of loaded DLLs.
	PRINT("DLL information:");
	SymEnumerateModules(GetCurrentProcess(), EnumModules, NULL);

	// Record stacktrace.
	PRINT("Stacktrace:");
	Stacktrace(e);

	// Unintialize IMAGEHLP.DLL
	SymCleanup(GetCurrentProcess());

	// Inform user.
	char dir[MAX_PATH], msg[MAX_PATH+200];
	GetCurrentDirectory(sizeof(dir) - 1, dir);
	SNPRINTF(msg, sizeof(msg),
		"Spring has crashed.\n\n"
		"A stacktrace has been written to:\n"
		"%s\\infolog.txt", dir);
	ErrorMessageBox(msg, "Spring: Unhandled exception", 0);

	// this seems to silently close the application
	return EXCEPTION_EXECUTE_HANDLER;

	// this triggers the microsoft "application has crashed" error dialog
	//return EXCEPTION_CONTINUE_SEARCH;

	// in practice, 100% CPU usage but no continuation of execution
	// (tested segmentation fault and division by zero)
	//return EXCEPTION_CONTINUE_EXECUTION;
}

/** Print stack traces for relevant threads. */
void HangHandler(bool simhang)
{
	PRINT("Hang detection triggered %sfor Spring %s.", simhang ? "(in sim thread) " : "", SpringVersion::GetFull().c_str());
#ifdef USE_GML
	PRINT("MT with %d threads.", gmlThreadCount);
#endif

	initImageHlpDll();

	// Record list of loaded DLLs.
	PRINT("DLL information:");
	SymEnumerateModules(GetCurrentProcess(), EnumModules, NULL);


	if (drawthread != INVALID_HANDLE_VALUE) {
		// Record stacktrace.
		PRINT("Stacktrace:");
		Stacktrace(NULL, drawthread);
	}

#if defined(USE_GML) && GML_ENABLE_SIM
	if (gmlMultiThreadSim && simthread != INVALID_HANDLE_VALUE) {
		PRINT("Stacktrace (sim):");
		Stacktrace(NULL, simthread);
	}
#endif

	// Unintialize IMAGEHLP.DLL
	SymCleanup(GetCurrentProcess());

	logOutput.Flush();
}

void HangDetector() {
	while (keepRunning) {
		// increase multiplier during game load to prevent false positives e.g. during pathing
		const int hangTimeMultiplier = CrashHandler::gameLoading ? 3 : 1;
		const spring_time hangtimeout = spring_msecs(spring_tomsecs(hangTimeout) * hangTimeMultiplier);

		spring_time curwdt = spring_gettime();
#if defined(USE_GML) && GML_ENABLE_SIM
		if (gmlMultiThreadSim) {
			spring_time cursimwdt = simwdt;
			if (spring_istime(cursimwdt) && curwdt > cursimwdt && (curwdt - cursimwdt) > hangtimeout) {
				HangHandler(true);
				simwdt = curwdt;
			}
		}
#endif
		spring_time curdrawwdt = drawwdt;
		if (spring_istime(curdrawwdt) && curwdt > curdrawwdt && (curwdt - curdrawwdt) > hangtimeout) {
			HangHandler(false);
			drawwdt = curwdt;
		}

		spring_sleep(spring_secs(1));
	}
}


void InstallHangHandler() {
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
					&drawthread, 0, TRUE, DUPLICATE_SAME_ACCESS);
	int hangTimeoutSecs = configHandler->Get("HangTimeout", 0);
	CrashHandler::gameLoading = false;
	// HangTimeout = -1 to force disable hang detection
	if (hangTimeoutSecs >= 0) {
		if (hangTimeoutSecs == 0)
			hangTimeoutSecs = 10;
		hangTimeout = spring_secs(hangTimeoutSecs);
		hangdetectorthread = new boost::thread(&HangDetector);
	}
	InitializeSEH();
}

void ClearDrawWDT(bool disable) { drawwdt = disable ? spring_notime : spring_gettime(); }
void ClearSimWDT(bool disable) { simwdt = disable ? spring_notime : spring_gettime(); }

void GameLoading(bool loading) { CrashHandler::gameLoading = loading; }

void UninstallHangHandler()
{
	if (hangdetectorthread) {
		keepRunning = 0;
		hangdetectorthread->join();
		delete hangdetectorthread;
	}
	if (drawthread != INVALID_HANDLE_VALUE) {
		CloseHandle(drawthread);
	}
	if (simthread != INVALID_HANDLE_VALUE) {
		CloseHandle(simthread);
	}
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

}; // namespace CrashHandler
