/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <windows.h>
#include <process.h>
#include <imagehlp.h>
#include <signal.h>

#include "System/Platform/CrashHandler.h"
#include "System/Platform/errorhandler.h"
#include "System/Log/ILog.h"
#include "System/Log/FileSink.h"
#include "System/Log/LogSinkHandler.h"
#include "System/LogOutput.h"
#include "System/Threading/SpringThreading.h"
#include "seh.h"
#include "System/StringUtil.h"
#include "System/SafeCStrings.h"
#include "Game/GameVersion.h"

#include <new> // set_new_handler
#include <chrono>
#include <vector>


#ifdef _MSC_VER
#define SYMLENGTH 4096
#endif

#define MAX_FRAMES 1024
#define DBG_LOG_RAW_LINE(level, fmt, ...) do {                               \
	fprintf((level >= LOG_LEVEL_ERROR)? stderr: stdout, fmt, ##__VA_ARGS__); \
	fprintf((level >= LOG_LEVEL_ERROR)? stderr: stdout, "\n"              ); \
	fprintf(logFile, fmt, ##__VA_ARGS__);                                    \
	fprintf(logFile, "\n"              );                                    \
} while (false)
#define LOG_RAW_LINE(level, fmt, ...) LOG_I(level, fmt, ##__VA_ARGS__);


namespace CrashHandler {

// printing while the thread is suspended apparently does
// allocations, which in turn cause deadlocks.
// to circumvent this we store all relevant information
// and only print after the thread is resumed
struct StacktraceLine {
	int type;

	char modName[MAX_PATH];
	DWORD dwModAddr;
#ifdef _MSC_VER
	char fileName[MAX_PATH];
	DWORD lineNumber;
	char symName[SYMLENGTH];
	DWORD64 pcOffset;
#endif
};

static std::vector<StacktraceLine> stacktraceLines;


static CRITICAL_SECTION stackLock;
static bool imageHelpInitialised = false;


static const char* aiLibWarning = "This stacktrace indicates a problem with a skirmish AI.";
static const char* glLibWarning =
	"This stacktrace indicates a problem with your graphics card driver, please try "
	"upgrading it. Specifically recommended is the latest version; do not forget to "
	"use a driver removal utility first.";

static const char* addrFmts[2] = {
	"\t(%d) %s:%u %s [0x%08llX]",
	"\t(%d) %s [0x%08lX]"
};
static const char* errFmt =
	"Spring has crashed:\n  %s.\n\n"
	"A stacktrace has been written to:\n  %s";

static FILE* logFile = nullptr;




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

	userSearchPath[0] = '.';
	userSearchPath[1] = '\0';

	// Initialize IMAGEHLP.DLL
	// Note: For some strange reason it doesn't work ~4 times after it was loaded&unloaded the first time.
	for (int i = 0; i < 20; i++) {
		if (SymInitialize(GetCurrentProcess(), userSearchPath, TRUE)) {
			SymSetOptions(SYMOPT_LOAD_LINES);

			return (imageHelpInitialised = true);
		}

		SymCleanup(GetCurrentProcess());
	}

	return false;
}



/** Callback for SymEnumerateModules */
#if _MSC_VER >= 1500
	static BOOL CALLBACK EnumModules(PCSTR moduleName, ULONG baseOfDll, PVOID userContext)
	{
		LOG_RAW_LINE(LOG_LEVEL_ERROR, "0x%08lx\t%s", baseOfDll, moduleName);
		return TRUE;
	}
#else // _MSC_VER >= 1500
	static BOOL CALLBACK EnumModules(LPSTR moduleName, DWORD baseOfDll, PVOID userContext)
	{
		LOG_RAW_LINE(LOG_LEVEL_ERROR, "0x%08lx\t%s", baseOfDll, moduleName);
		return TRUE;
	}
#endif // _MSC_VER >= 1500



/** Print out a stacktrace. */
inline static void StacktraceInline(const char* threadName, LPEXCEPTION_POINTERS e, HANDLE hThread = INVALID_HANDLE_VALUE, const int logLevel = LOG_LEVEL_ERROR)
{
	STACKFRAME64 frame;
	CONTEXT context;
	HANDLE thread = hThread;

	const HANDLE process = GetCurrentProcess();
	const HANDLE cThread = GetCurrentThread();

	DWORD64 dwModBase =  0;
	DWORD64 dwModAddr =  0;
	DWORD machineType =  0;

	const bool wdThread = (hThread != INVALID_HANDLE_VALUE);

	bool aiLibFound = false;
	bool glLibFound = false;

	int numFrames = 0;
	char modName[MAX_PATH];

	const void* initialPC;
	const void* initialSP;
	const void* initialFP;

	ZeroMemory(&frame, sizeof(frame));
	ZeroMemory(&context, sizeof(CONTEXT));
	memset(modName, 0, sizeof(modName));
	memset(stacktraceLines.data(), 0, stacktraceLines.size() * sizeof(StacktraceLine));
	assert(logFile != nullptr);

	// NOTE: this line is parsed by the stacktrans script
	if (threadName != nullptr) {
		LOG_RAW_LINE(logLevel, "Stacktrace (%s) for Spring %s:", threadName, (SpringVersion::GetFull()).c_str());
	} else {
		LOG_RAW_LINE(logLevel, "Stacktrace for Spring %s:", (SpringVersion::GetFull()).c_str());
	}

	if (e != nullptr) {
		// reached when an exception occurs
		context = *e->ContextRecord;
		thread = cThread;
	} else if (wdThread) {
		// reached when watchdog triggers, suspend thread (it might be in an infinite loop)
		context.ContextFlags = CONTEXT_FULL;

		assert(Threading::IsWatchDogThread());
		// assert(!CompareObjectHandles(hThread, cThread));

		if (hThread == cThread) {
			// should never happen
			LOG_RAW_LINE(logLevel, "\t[attempted to suspend hang-detector thread]");
			return;
		}

		if (SuspendThread(hThread) == -1) {
			LOG_RAW_LINE(logLevel, "\t[failed to suspend thread]");
			return;
		}

		if (GetThreadContext(hThread, &context) == 0) {
			ResumeThread(hThread);
			LOG_RAW_LINE(logLevel, "\t[failed to get thread context]");
			return;
		}
	} else {
		// fallback; get context directly from CPU-registers
#ifdef _M_IX86
		context.ContextFlags = CONTEXT_CONTROL;

#ifdef _MSC_VER
		// MSVC
		__asm {
			call func;
			func: pop eax;
			mov [context.Eip], eax;
			mov [context.Ebp], ebp;
			mov [context.Esp], esp;
		}
#else
		// GCC
		DWORD eip, esp, ebp;
		__asm__ __volatile__ ("call func; func: pop %%eax; mov %%eax, %0;" : "=m" (eip) : : "%eax" );
		__asm__ __volatile__ ("mov %%ebp, %0;" : "=m" (ebp) : : );
		__asm__ __volatile__ ("mov %%esp, %0;" : "=m" (esp) : : );

		context.Eip = eip;
		context.Ebp = ebp;
		context.Esp = esp;
#endif

#else
		RtlCaptureContext(&context);
#endif
		thread = cThread;
	}


	{
		// retrieve program-counter and starting stack-frame address
		#ifdef _M_IX86
		machineType = IMAGE_FILE_MACHINE_I386;
		frame.AddrPC.Offset = context.Eip;
		frame.AddrStack.Offset = context.Esp;
		frame.AddrFrame.Offset = context.Ebp;
		#elif _M_X64
		machineType = IMAGE_FILE_MACHINE_AMD64;
		frame.AddrPC.Offset = context.Rip;
		frame.AddrStack.Offset = context.Rsp;
		frame.AddrFrame.Offset = context.Rsp;
		#else
		#error "CrashHandler: Unsupported platform"
		#endif

		frame.AddrPC.Mode = AddrModeFlat;
		frame.AddrStack.Mode = AddrModeFlat;
		frame.AddrFrame.Mode = AddrModeFlat;

		initialPC = reinterpret_cast<const void*>(frame.AddrPC.Offset);
		initialSP = reinterpret_cast<const void*>(frame.AddrStack.Offset);
		initialFP = reinterpret_cast<const void*>(frame.AddrFrame.Offset);
	}


	while (StackWalk64(machineType, process, thread, &frame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
		#if 0
		if (frame.AddrFrame.Offset == 0)
			break;
		#endif
		if (numFrames >= MAX_FRAMES)
			break;


		if ((dwModBase = SymGetModuleBase64(process, frame.AddrPC.Offset)) != 0) {
			GetModuleFileName((HINSTANCE) dwModBase, modName, MAX_PATH);
		} else {
			strcpy(modName, "Unknown");
		}


		StacktraceLine& stl = stacktraceLines[numFrames];

#ifdef _MSC_VER
		char symbuf[sizeof(SYMBOL_INFO) + SYMLENGTH];

		PSYMBOL_INFO pSym = reinterpret_cast<SYMBOL_INFO*>(symbuf);
		pSym->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSym->MaxNameLen = SYMLENGTH;

		// check if we have symbols, only works on VC (mingw doesn't have a compatible file format)
		if (SymFromAddr(process, frame.AddrPC.Offset, nullptr, pSym)) {
			IMAGEHLP_LINE64 line = {0};
			line.SizeOfStruct = sizeof(line);

			DWORD displacement;
			SymGetLineFromAddr64(GetCurrentProcess(), frame.AddrPC.Offset, &displacement, &line);

			stl.type = 0;
			strncpy(stl.fileName, line.FileName ? line.FileName : "<unknown>", MAX_PATH);
			stl.lineNumber = line.LineNumber;
			strncpy(stl.symName, pSym->Name, SYMLENGTH);
			stl.pcOffset = frame.AddrPC.Offset;
		} else
#endif
		{
			// this is the code path taken on MinGW, and MSVC if no debugging syms are found
			// for the .exe we need the absolute address while for DLLs we need the module's
			// internal/relative address
			if (strstr(modName, ".exe") != nullptr) {
				dwModAddr = frame.AddrPC.Offset;
			} else {
				dwModAddr = frame.AddrPC.Offset - dwModBase;
			}

			stl.type = 1;
			strncpy(stl.modName, modName, MAX_PATH);
			stl.dwModAddr = dwModAddr;
		}

		{
			aiLibFound |= (strstr(modName, "SkirmishAI.dll") != nullptr);
		}
		{
			// OpenGL lib names (ATI): "atioglxx.dll" "atioglx2.dll"
			glLibFound |= (strstr(modName, "atiogl") != nullptr);
			// OpenGL lib names (Nvidia): "nvoglnt.dll" "nvoglv32.dll" "nvoglv64.dll" (last one is a guess)
			glLibFound |= (strstr(modName, "nvogl") != nullptr);
			// OpenGL lib names (Intel): "ig4dev32.dll" "ig4dev64.dll" "ig4icd32.dll"
			glLibFound |= (strstr(modName, "ig4") != nullptr);
			// OpenGL lib names (Intel)
			glLibFound |= (strstr(modName, "ig75icd32.dll") != nullptr);
		}

		++numFrames;
	}


	if (wdThread)
		ResumeThread(hThread);

	if (aiLibFound)
		LOG_RAW_LINE(logLevel, "%s", aiLibWarning);
	if (glLibFound)
		LOG_RAW_LINE(logLevel, "%s", glLibWarning);

	// log initial context
	LOG_RAW_LINE(logLevel, "\t[ProgCtr=%p StackPtr=%p FramePtr=%p]", initialPC, initialSP, initialFP);

	for (int i = 0; i < numFrames; ++i) {
		const StacktraceLine& stl = stacktraceLines[i];
		switch (stl.type) {
#ifdef _MSC_VER
			case 0: {
				LOG_RAW_LINE(logLevel, addrFmts[stl.type], i, stl.fileName, stl.lineNumber, stl.symName, stl.pcOffset);
			} break;
#endif
			case 1: {
				LOG_RAW_LINE(logLevel, addrFmts[stl.type], i, stl.modName, stl.dwModAddr);
			} break;
			default: {
				assert(false);
			} break;
		}
	}
}


// internally called, lock should already be held
static void Stacktrace(const char* threadName, LPEXCEPTION_POINTERS e, HANDLE hThread, const int logLevel) {
	StacktraceInline(threadName, e, hThread, logLevel);
}

// externally called
void Stacktrace(Threading::NativeThreadHandle thread, const std::string& threadName, const int logLevel)
{
	// caller has to Prepare
	// EnterCriticalSection(&stackLock);
	StacktraceInline(threadName.c_str(), nullptr, thread, logLevel);
	// caller has to Cleanup
	// LeaveCriticalSection(&stackLock);
}



void PrepareStacktrace(const int logLevel) {
	EnterCriticalSection(&stackLock);
	InitImageHlpDll();

	// sidestep any kind of hidden allocation which might cause a deadlock
	// this does mean the "[f=123456] Error:" prefixes will not be present
	logFile = log_file_getLogFileStream((logOutput.GetFilePath()).c_str());

	// Record list of loaded DLLs.
	LOG_RAW_LINE(logLevel, "DLL information:");
	SymEnumerateModules(GetCurrentProcess(), (PSYM_ENUMMODULES_CALLBACK)EnumModules, nullptr);
}

void CleanupStacktrace(const int logLevel) {
	LOG_CLEANUP();

	// Uninitialize IMAGEHLP.DLL
	SymCleanup(GetCurrentProcess());
	imageHelpInitialised = false;

	LeaveCriticalSection(&stackLock);
}

void OutputStacktrace() {
	LOG_RAW_LINE(LOG_LEVEL_ERROR, "Error handler invoked for Spring %s.", (SpringVersion::GetFull()).c_str());

	PrepareStacktrace();
	Stacktrace(nullptr, nullptr, INVALID_HANDLE_VALUE, LOG_LEVEL_ERROR);
	CleanupStacktrace();
}



void NewHandler() {
	std::set_new_handler(nullptr); // prevent recursion; OST or EMB might perform hidden allocs
	LOG_RAW_LINE(LOG_LEVEL_ERROR, "Failed to allocate memory"); // make sure this ends up in the log also

	OutputStacktrace();
	ErrorMessageBox("Failed to allocate memory", "Spring: Fatal Error", MBF_OK | MBF_CRASH);
}

static void SigAbrtHandler(int signal)
{
	LOG_RAW_LINE(LOG_LEVEL_ERROR, "Spring received an ABORT signal");

	OutputStacktrace();
	ErrorMessageBox("Abort / abnormal termination", "Spring: Fatal Error", MBF_OK | MBF_CRASH);
}




/** Called by windows if an exception happens. */
LONG CALLBACK ExceptionHandler(LPEXCEPTION_POINTERS e)
{
	// prologue; disable registered sinks (info-console, ...)
	logSinkHandler.SetSinking(false);
	LOG_RAW_LINE(LOG_LEVEL_ERROR, "Spring %s has crashed.", (SpringVersion::GetFull()).c_str());
	PrepareStacktrace();

	const char* errStr = ExceptionName(e->ExceptionRecord->ExceptionCode);
	char errBuf[2048];

	LOG_RAW_LINE(LOG_LEVEL_ERROR, "Exception: %s (0x%08lx)", errStr, e->ExceptionRecord->ExceptionCode);
	LOG_RAW_LINE(LOG_LEVEL_ERROR, "Exception Address: 0x%p", (PVOID) e->ExceptionRecord->ExceptionAddress);

	// print trace inline: avoids modifying the stack which might confuse
	// StackWalk when using the context record passed to ExceptionHandler
	StacktraceInline(nullptr, e);
	CleanupStacktrace();

	// only the first crash is of any real interest
	CrashHandler::Remove();

	// inform user about the exception
	SNPRINTF(errBuf, sizeof(errBuf), errFmt, errStr, (logOutput.GetFilePath()).c_str());
	ErrorMessageBox(errBuf, "Spring: Unhandled exception", MBF_OK | MBF_CRASH); //calls exit()!

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
	InitializeCriticalSection(&stackLock);

	SetUnhandledExceptionFilter(ExceptionHandler);
	signal(SIGABRT, SigAbrtHandler);
	std::set_new_handler(NewHandler);

	// pre-allocate since doing so after a bad_alloc exception can fail
	// NB: MAX_FRAMES * sizeof(StacktraceLine) is too big for the stack
	stacktraceLines.resize(MAX_FRAMES);
}

/** Uninstall crash handler. */
void Remove()
{
	SetUnhandledExceptionFilter(nullptr);
	signal(SIGABRT, SIG_DFL);
	std::set_new_handler(nullptr);

	DeleteCriticalSection(&stackLock);
}

}; // namespace CrashHandler

