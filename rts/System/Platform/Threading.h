/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _THREADING_H_
#define _THREADING_H_

#include <boost/thread.hpp>

namespace Threading {
	void SetMainThreadID();
	bool IsMainThread();
};

#endif // _THREADING_H_
