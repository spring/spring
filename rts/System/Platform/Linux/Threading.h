/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _THREADING_LINUX_H_
#define _THREADING_LINUX_H_

#include <string>
#include <boost/cstdint.hpp>

namespace Threading {
   uint64_t SetThreadAffinityMask(uint64_t processorMask);
   size_t NumProcessors();
};

#endif // _THREADING_LINUX_H_
