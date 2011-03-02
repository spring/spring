/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _THREADING_H_
#define _THREADING_H_

#include <boost/thread.hpp>
#ifdef WIN32
	#include <windows.h>
#else
	#include <pthread.h>
#endif


namespace Threading {
#ifdef WIN32
	typedef HANDLE NativeThreadHandle;
#else
	typedef pthread_t NativeThreadHandle;
#endif

#ifdef WIN32
	typedef DWORD NativeThreadId;
#else
	typedef pthread_t NativeThreadId;
#endif

	struct Error {
		Error(const std::string& _caption, const std::string& _message, const unsigned int _flags) : caption(_caption), message(_message), flags(_flags) {};
		std::string caption;
		std::string message;
		unsigned int flags;
	};

	NativeThreadHandle _GetCurrentThread();
	NativeThreadId _GetCurrentThreadId();
	void SetMainThread(boost::thread* mt = NULL);
	void SetLoadingThread(boost::thread* lt);
	boost::thread* GetMainThread();
	boost::thread* GetLoadingThread();
	void SetThreadError(const Error& err);
	Error* GetThreadError();
	bool IsMainThread();
	bool IsLoadingThread();
};

#endif // _THREADING_H_
