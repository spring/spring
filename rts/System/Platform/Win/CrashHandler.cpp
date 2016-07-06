/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <windows.h>
#include <process.h>
#include <imagehlp.h>
#include <signal.h>

#include "System/Platform/CrashHandler.h"
#include "System/Platform/errorhandler.h"
#include "System/Log/ILog.h"
#include "System/Log/LogSinkHandler.h"
#include "System/LogOutput.h"
#include "Net/Protocol/NetProtocol.h"
#include "seh.h"
#include "System/Util.h"
#include "System/SafeCStrings.h"
#include "Game/GameVersion.h"
#include <new>


#define BUFFER_SIZE 2048
#define MAX_STACK_DEPTH 4096

namespace CrashHandler {

CRITICAL_SECTION stackLock;
bool imageHelpInitialised = false;
int stackLockInit() { InitializeCriticalSection(&stackLock); return 0; }
int dummyStackLock = stackLockInit();

static void SigAbrtHandler(int signal)
{
	// cause an exception if on windows
	LOG_L(L_ERROR, "Spring received an ABORT signal");

	OutputStacktrace();

	ErrorMessageBox("Abort / abnormal termination", "Spring: Fatal Error", MBF_OK | MBF_CRASH);
}

/** Convert exception code to human readable string. */
static const char* ExceptionName(DWORD exceptionCode)
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


bool InitImageHlpDll()
{
	if (imageHelpInitialised)
		return true;

	char userSearchPath[8];
	STRCPY_T(userSearchPath, 8, ".");
	// Initialize IMAGEHLP.DLL
	// Note: For some strange reason it doesn't work ~4 times after it was loaded&unloaded the first time.
	int i = 0;
	do {
		if (SymInitialize(GetCurrentProcess(), userSearchPath, TRUE)) {
			SymSetOptions(SYMOPT_LOAD_LINES);

			imageHelpInitialised = true;
			return true;
		}
		SymCleanup(GetCurrentProcess());
		i++;
	} while (i<20);
	return false;
}


/** Callback for SymEnumerateModules */
#if _MSC_VER >= 1500
	static BOOL CALLBACK EnumModules(PCSTR moduleName, ULONG baseOfDll, PVOID userContext)
	{
		LOG_L(L_ERROR, "0x%08lx\t%s", baseOfDll, moduleName);
		return TRUE;
	}
#else // _MSC_VER >= 1500
	static BOOL CALLBACK EnumModules(LPSTR moduleName, DWORD baseOfDll, PVOID userContext)
	{
		LOG_L(L_ERROR, "0x%08lx\t%s", baseOfDll, moduleName);
		return TRUE;
	}
#endif // _MSC_VER >= 1500

static DWORD __stdcall AllocTest(void *param) {
	GlobalFree(GlobalAlloc(GMEM_FIXED, 16384));
	return 0;
}

/** Print out a stacktrace. */
inline static void StacktraceInline(const char *threadName, LPEXCEPTION_POINTERS e, HANDLE hThread = INVALID_HANDLE_VALUE, const int logLevel = LOG_LEVEL_ERROR)
{
	STACKFRAME64 sf;
	HANDLE process, thread;
	DWORD64 dwModBase;
	DWORD dwModAddrToPrint;
	BOOL more = FALSE;
	int count = 0;
	char modname[MAX_PATH];

	process = GetCurrentProcess();

	if (threadName != NULL) {
		LOG_I(logLevel, "Stacktrace (%s) for Spring %s:", threadName, (SpringVersion::GetFull()).c_str());
	} else {
		LOG_I(logLevel, "Stacktrace for Spring %s:", (SpringVersion::GetFull()).c_str());
	}

	bool suspended = false;
	CONTEXT c;
	if (e) {
		c = *e->ContextRecord;
		thread = GetCurrentThread();
	} else if (hThread != INVALID_HANDLE_VALUE) {
		for (int allocIter = 0; ; ++allocIter) {
			HANDLE allocThread = CreateThread(NULL, 0, &AllocTest, NULL, CREATE_SUSPENDED, NULL);
			SuspendThread(hThread);
			ResumeThread(allocThread);
			if (WaitForSingleObject(allocThread, 10) == WAIT_OBJECT_0) {
				CloseHandle(allocThread);
				break;
			}
			ResumeThread(hThread);
			if (WaitForSingleObject(allocThread, 10) != WAIT_OBJECT_0)
				TerminateThread(allocThread, 0);
			CloseHandle(allocThread);
			if (allocIter < 10)
				continue;
			LOG_I(logLevel, "Stacktrace failed, allocator deadlock");
			return;
		}

		suspended = true;
		memset(&c, 0, sizeof(CONTEXT));
		c.ContextFlags = CONTEXT_FULL;

		if (!GetThreadContext(hThread, &c)) {
			ResumeThread(hThread);
			LOG_I(logLevel, "Stacktrace failed, failed to get context");
			return;
		}
		thread = hThread;
	}
	else {
#ifdef _M_IX86
		ZeroMemory( &c, sizeof( CONTEXT ) );
		c.ContextFlags = CONTEXT_CONTROL;
#ifdef _MSC_VER
		__asm
		{
			call func;
			func: pop eax;
			mov [c.Eip], eax;
			mov [c.Ebp], ebp;
			mov [c.Esp], esp;
		}
#else
		DWORD eip, esp, ebp;
		__asm__ __volatile__ ("call func; func: pop %%eax; mov %%eax, %0;" : "=m" (eip) : : "%eax" );
		__asm__ __volatile__ ("mov %%ebp, %0;" : "=m" (ebp) : : );
		__asm__ __volatile__ ("mov %%esp, %0;" : "=m" (esp) : : );
		c.Eip=eip;
		c.Ebp=ebp;
		c.Esp=esp;
#endif
#else
		RtlCaptureContext( &c );
#endif
		thread = GetCurrentThread();
	}

	DWORD MachineType = 0;
	ZeroMemory(&sf, sizeof(sf));
#ifdef _M_IX86
	MachineType = IMAGE_FILE_MACHINE_I386;
	sf.AddrPC.Offset = c.Eip;
	sf.AddrStack.Offset = c.Esp;
	sf.AddrFrame.Offset = c.Ebp;
#elif _M_X64
	MachineType = IMAGE_FILE_MACHINE_AMD64;
	sf.AddrPC.Offset = c.Rip;
	sf.AddrStack.Offset = c.Rsp;
	sf.AddrFrame.Offset = c.Rsp;
#else
	#error "CrashHandler: Unsupported platform"
#endif
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrStack.Mode = AddrModeFlat;
	sf.AddrFrame.Mode = AddrModeFlat;

	// use globalalloc to reduce risk for allocator related deadlock
	char* printstrings = (char*)GlobalAlloc(GMEM_FIXED, 0);

	bool containsOglDll = false;
	while (true) {
		more = StackWalk64(
			MachineType,
			process,
			thread,
			&sf,
			&c,
			NULL,
			SymFunctionTableAccess64,
			SymGetModuleBase64,
			NULL
		);
		if (!more || /*sf.AddrFrame.Offset == 0 ||*/ count > MAX_STACK_DEPTH) {
			break;
		}

		dwModBase = SymGetModuleBase64(process, sf.AddrPC.Offset);

		if (dwModBase) {
			GetModuleFileName((HINSTANCE)dwModBase, modname, MAX_PATH);
		} else {
			strcpy(modname, "Unknown");
		}

		char* printstringsnew = (char*) GlobalAlloc(GMEM_FIXED, (count + 1) * BUFFER_SIZE);
		memcpy(printstringsnew, printstrings, count * BUFFER_SIZE);
		GlobalFree(printstrings);
		printstrings = printstringsnew;

#ifdef _MSC_VER
		const int SYMLENGTH = 4096;
		char symbuf[sizeof(SYMBOL_INFO) + SYMLENGTH];
		PSYMBOL_INFO pSym = reinterpret_cast<SYMBOL_INFO*>(symbuf);

		pSym->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSym->MaxNameLen = SYMLENGTH;

		// Check if we have symbols, only works on VC (mingw doesn't have a compatible file format)
		if (SymFromAddr(process, sf.AddrPC.Offset, nullptr, pSym)) {
			IMAGEHLP_LINE64 line = { 0 };
			line.SizeOfStruct = sizeof(line);

			DWORD displacement;
			SymGetLineFromAddr64(GetCurrentProcess(), sf.AddrPC.Offset, &displacement, &line);

			SNPRINTF(printstrings + count * BUFFER_SIZE, BUFFER_SIZE, "(%d) %s:%u %s [0x%08llX]", count , line.FileName ? line.FileName : "<unknown>", line.LineNumber, pSym->Name, sf.AddrPC.Offset);
		} else 
#endif
		{
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
		// OpenGL lib names (Intel): "ig4dev32.dll" "ig4dev64.dll" "ig4icd32.dll"
		containsOglDll = containsOglDll || strstr(modname, "ig4");

		++count;
	}

	if (suspended) {
		ResumeThread(hThread);
	}

	if (containsOglDll) {
		LOG_I(logLevel, "This stack trace indicates a problem with your graphic card driver. "
		      "Please try upgrading or downgrading it. "
		      "Specifically recommended is the latest driver, and one that is as old as your graphic card. "
		      "Make sure to use a driver removal utility, before installing other drivers.");
	}

	for (int i = 0; i < count; ++i) {
		LOG_I(logLevel, "%s", printstrings + i * BUFFER_SIZE);
	}

	GlobalFree(printstrings);
}

static void Stacktrace(const char *threadName, LPEXCEPTION_POINTERS e, HANDLE hThread = INVALID_HANDLE_VALUE, const int logLevel = LOG_LEVEL_ERROR) {
	StacktraceInline(threadName, e, hThread, logLevel);
}


void Stacktrace(Threading::NativeThreadHandle thread, const std::string& threadName, const int logLevel)
{
	Stacktrace(threadName.c_str(), NULL, thread, logLevel);
}

void PrepareStacktrace(const int logLevel) {
	EnterCriticalSection( &stackLock );

	InitImageHlpDll();

	// Record list of loaded DLLs.
	LOG_I(logLevel, "DLL information:");
	SymEnumerateModules(GetCurrentProcess(), (PSYM_ENUMMODULES_CALLBACK)EnumModules, NULL);
}

void CleanupStacktrace(const int logLevel) {
	LOG_CLEANUP();
	// Unintialize IMAGEHLP.DLL
	SymCleanup(GetCurrentProcess());
	imageHelpInitialised = false;

	LeaveCriticalSection( &stackLock );
}

void OutputStacktrace() {
	LOG_L(L_ERROR, "Error handler invoked for Spring %s.", (SpringVersion::GetFull()).c_str());

	PrepareStacktrace();

	Stacktrace(NULL, NULL);

	CleanupStacktrace();
}

void NewHandler() {
	LOG_L(L_ERROR, "Failed to allocate memory"); // make sure this ends up in the log also

	OutputStacktrace();

	ErrorMessageBox("Failed to allocate memory", "Spring: Fatal Error", MBF_OK | MBF_CRASH);
}

/** Called by windows if an exception happens. */
LONG CALLBACK ExceptionHandler(LPEXCEPTION_POINTERS e)
{
	// Prologue.
	logSinkHandler.SetSinking(false);
	LOG_L(L_ERROR, "Spring %s has crashed.", (SpringVersion::GetFull()).c_str());
	PrepareStacktrace();

	const std::string error(ExceptionName(e->ExceptionRecord->ExceptionCode));

	// Print exception info.
	LOG_L(L_ERROR, "Exception: %s (0x%08lx)", error.c_str(), e->ExceptionRecord->ExceptionCode);
	LOG_L(L_ERROR, "Exception Address: 0x%p", (PVOID) e->ExceptionRecord->ExceptionAddress);

	// Print stacktrace.
	StacktraceInline(NULL, e); // inline: avoid modifying the stack, it might confuse StackWalk when using the context record passed to ExceptionHandler

	CleanupStacktrace();

	// Only the first crash is of any real interest
	CrashHandler::Remove();

	// Inform user.
	std::ostringstream buf;
	buf << "Spring has crashed:\n"
		<< "  " << error << ".\n\n"
		<< "A stacktrace has been written to:\n"
		<< "  " << logOutput.GetFilePath();
	ErrorMessageBox(buf.str(), "Spring: Unhandled exception", MBF_OK | MBF_CRASH); //calls exit()!

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
	std::set_new_handler(NewHandler);
}


/** Uninstall crash handler. */
void Remove()
{
	SetUnhandledExceptionFilter(NULL);
	signal(SIGABRT, SIG_DFL);
	std::set_new_handler(NULL);
}

}; // namespace CrashHandler
