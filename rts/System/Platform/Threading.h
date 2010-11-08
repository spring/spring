/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _THREADING_H_
#define _THREADING_H_

#include <boost/thread.hpp>

namespace Threading {
	struct Error {
		Error(const std::string& _caption, const std::string& _message, const unsigned int _flags) : caption(_caption), message(_message), flags(_flags) {};
		std::string caption;
		std::string message;
		unsigned int flags;
	};

	void SetMainThread(boost::thread* mt);
	boost::thread* GetMainThread();
	void SetThreadError(const Error& err);
	Error* GetThreadError();
	bool IsMainThread();
};

#endif // _THREADING_H_
