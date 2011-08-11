/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <windows.h>
#include "seh.h"
#include "System/Platform/CrashHandler.h"

#ifdef _MSC_VER
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

//! defined in System/Platform/Win/CrashHandler.cpp
namespace CrashHandler {
	LONG CALLBACK ExceptionHandler(LPEXCEPTION_POINTERS e);
}

void __cdecl se_translator_function(unsigned int err, struct _EXCEPTION_POINTERS* ep)
{
	char buf[128];
	sprintf(buf,"%s(0x%08x) at 0x%08x",ExceptionName(err),err,ep->ExceptionRecord->ExceptionAddress);
	CrashHandler::ExceptionHandler(ep);
	throw std::exception(buf);
}
#endif

void InitializeSEH()
{
#ifdef _MSC_VER
	_set_se_translator(se_translator_function);
#else
	//! GCC/MinGW cannot handle win32 exceptions
#endif
}
