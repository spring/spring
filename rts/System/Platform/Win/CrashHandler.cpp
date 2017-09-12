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
#include "seh.h"
#include "System/StringUtil.h"
#include "System/SafeCStrings.h"
#include "Game/GameVersion.h"

#include <new> // set_new_handler
#include <chrono>
#include <thread>


#define BUFFER_SIZE 1024
#define LOG_RAW_LINE(level, fmt, ...) do {                                   \
	fprintf((level >= LOG_LEVEL_ERROR)? stderr: stdout, fmt, ##__VA_ARGS__); \
	fprintf((level >= LOG_LEVEL_ERROR)? stderr: stdout, "\n"              ); \
	fprintf(logFile, fmt, ##__VA_ARGS__);                                    \
	fprintf(logFile, "\n"              );                                    \
} while (false)
// #define LOG_RAW_LINE(level, fmt, ...) LOG_I(level, fmt, ##__VA_ARGS__);

namespace CrashHandler {

static std::function<void(HANDLE, DWORD*)> suspendFunc = [](HANDLE thr, DWORD* ret) { *ret = SuspendThread(thr); };
static std::thread suspendThrd;


CRITICAL_SECTION stackLock;
bool imageHelpInitialised = false;

static int stackLockInit() { InitializeCriticalSection(&stackLock); return 0; }
const int dummyStackLock = stackLockInit();

// 1MB buffer (1K lines of 1K chars each) should be enough for any trace
static char traceBuffer[BUFFER_SIZE * BUFFER_SIZE];

static const char* aiLibWarning = "This stacktrace indicates a problem with a skirmish AI.";
static const char* glLibWarning =
	"This stacktrace indicates a problem with your graphics card driver. "
	"Please try upgrading it (specifically recommended is the latest version) "
	"but make sure to use a driver removal utility first.";

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
	DWORD suspendCntr = -2;

	const bool wdThread = (hThread != INVALID_HANDLE_VALUE);

	bool aiLibFound = false;
	bool glLibFound = false;

	int numFrames = 0;
	char modName[MAX_PATH];

	ZeroMemory(&frame, sizeof(frame));
	ZeroMemory(&context, sizeof(CONTEXT));
	memset(modName, 0, sizeof(modName));
	memset(traceBuffer, 0, sizeof(traceBuffer));
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
		context.ContextFlags = CONTEXT_FULL;

		assert(Threading::IsWatchDogThread());
		assert(!CompareObjectHandles(hThread, cThread));

		if (hThread != cThread) {
			LOG_RAW_LINE(logLevel, "\t[attempting to suspend thread]");

			// FIXME:
			//   SuspendThread? occasionally seems to hang the hang-detector
			//   (which obviously never passes its own handle to Stacktrace)
			//   risk an allocator deadlock and make the suspend call from a
			//   helper thread
			suspendThrd = std::move(std::thread(suspendFunc, hThread, &suspendCntr));

			for (int i = 0; i < 50; i++) {
				if (suspendCntr != -2) {
					LOG_RAW_LINE(logLevel, "\t[SuspendThread returned %lu after %d iterations]", suspendCntr, i);
					break;
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			LOG_RAW_LINE(logLevel, "\t[thread %s with count %lu]", ((suspendCntr == -2)? "deadlocked": "suspended or still running"), suspendCntr);
		} else {
			// should never happen
			LOG_RAW_LINE(logLevel, "\t[attempted to suspend hang-detector thread]");
			return;
		}

		// if still -2 after 50 sleeps, assume a deadlock and try to get the context anyway
		if (suspendCntr == -1) {
			LOG_RAW_LINE(logLevel, "\t[failed to suspend thread]");
			suspendThrd.join();
			return;
		}

		if (GetThreadContext(hThread, &context) == 0) {
			LOG_RAW_LINE(logLevel, "\t[failed to get thread context]");
			ResumeThread(hThread);
			suspendThrd.join();
			return;
		}


		#if 0
		// reached when watchdog triggers, suspend thread (it might be in an infinite loop)
		if (SuspendThread(hThread) == DWORD(-1)) {
			LOG_RAW_LINE(logLevel, "\t[failed to suspend thread]");
			return;
		}

		if (GetThreadContext(hThread, &context) == 0) {
			LOG_RAW_LINE(logLevel, "\t[failed to get thread context]");
			ResumeThread(hThread);
			return;
		}
		#endif
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

		const void* pc = reinterpret_cast<const void*>(frame.AddrPC.Offset);
		const void* sp = reinterpret_cast<const void*>(frame.AddrStack.Offset);
		const void* fp = reinterpret_cast<const void*>(frame.AddrFrame.Offset);

		// log initial context
		LOG_RAW_LINE(logLevel, "\t[ProgCtr=%p StackPtr=%p FramePtr=%p]", pc, sp, fp);
	}


	while (StackWalk64(machineType, process, thread, &frame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
		#if 0
		if (frame.AddrFrame.Offset == 0)
			break;
		#endif
		if (numFrames >= BUFFER_SIZE)
			break;


		if ((dwModBase = SymGetModuleBase64(process, frame.AddrPC.Offset)) != 0) {
			GetModuleFileName((HINSTANCE) dwModBase, modName, MAX_PATH);
		} else {
			strcpy(modName, "Unknown");
		}


#ifdef _MSC_VER
		const int SYMLENGTH = 4096;
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

			SNPRINTF(traceBuffer + numFrames * BUFFER_SIZE, BUFFER_SIZE, addrFmts[0], numFrames , line.FileName ? line.FileName : "<unknown>", line.LineNumber, pSym->Name, frame.AddrPC.Offset);
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

			SNPRINTF(traceBuffer + numFrames * BUFFER_SIZE, BUFFER_SIZE, addrFmts[1], numFrames, modName, dwModAddr);
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

	for (int i = 0; i < numFrames; ++i) {
		LOG_RAW_LINE(logLevel, "%s", traceBuffer + i * BUFFER_SIZE);
	}

	if (suspendThrd.joinable())
		suspendThrd.join();
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
	logFile = fopen((logOutput.GetFilePath()).c_str(), "a");

	// Record list of loaded DLLs.
	LOG_RAW_LINE(logLevel, "DLL information:");
	SymEnumerateModules(GetCurrentProcess(), (PSYM_ENUMMODULES_CALLBACK)EnumModules, nullptr);
}

void CleanupStacktrace(const int logLevel) {
	LOG_CLEANUP();
	// Uninitialize IMAGEHLP.DLL
	SymCleanup(GetCurrentProcess());
	imageHelpInitialised = false;

	fclose(logFile);
	LeaveCriticalSection(&stackLock);
}

void OutputStacktrace() {
	LOG_RAW_LINE(LOG_LEVEL_ERROR, "Error handler invoked for Spring %s.", (SpringVersion::GetFull()).c_str());

	PrepareStacktrace();
	Stacktrace(nullptr, nullptr, INVALID_HANDLE_VALUE, LOG_LEVEL_ERROR);
	CleanupStacktrace();
}



void NewHandler() {
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
	// prologue
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
	// pre-allocate stack
	suspendThrd = std::move(std::thread([]() { return 0; }));
	suspendThrd.join();

	SetUnhandledExceptionFilter(ExceptionHandler);
	signal(SIGABRT, SigAbrtHandler);
	std::set_new_handler(NewHandler);
}

/** Uninstall crash handler. */
void Remove()
{
	SetUnhandledExceptionFilter(nullptr);
	signal(SIGABRT, SIG_DFL);
	std::set_new_handler(nullptr);

	// prevent a std::terminate if we never suspended
	if (suspendThrd.joinable())
		suspendThrd.join();
}

}; // namespace CrashHandler

