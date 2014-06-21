#include "Threading.h"
#include "System/Log/ILog.h"

#include <errno.h>
#include <string.h>

namespace Threading {
	size_t NumProcessors() {
		static size_t numProcessors;

		if (numProcessors == 0) {
			numProcessors = (size_t) sysconf(_SC_NPROCESSORS_CONF);
		}

		return numProcessors;
	}

	uint64_t SetThreadAffinityMask(uint64_t processorMask) {
		cpu_set_t set;

		uint64_t previousProcessorMask = 0;
		{
			int ret = sched_getaffinity(0, sizeof(set), &set);
			if(ret!=0) {
				LOG("[%s] sched_getaffinity error: %s (%d)", __FUNCTION__, strerror(errno), errno);
				// We still can continue executing, will just
				// not return any usefull mask
				CPU_ZERO(&set);
			}

			for (size_t processor = 0;
			     processor < NumProcessors(); processor++) {
				if (CPU_ISSET_S(processor, sizeof(set), &set))
					previousProcessorMask |=
					    ((uint64_t) 1) << processor;
			}
		}

		CPU_ZERO(&set);
		for (size_t processor = 0; processor < NumProcessors();
		     processor++) {
			if ((processorMask >> processor) & 1)
				CPU_SET(processor, &set);
		}

		int ret = sched_setaffinity(0, sizeof(set), &set);
		// (The process gets migrated immediately by the setaffinity call)
		if(ret!=0) {
			LOG("[%s] sched_setaffinity error: %s (%d)", __FUNCTION__, strerror(errno), errno);
			// We still can continue executing, but we run as before
			CPU_ZERO(&set);
		}

		return previousProcessorMask;
	}

}
