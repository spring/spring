/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_EXIT_CODE_H
#define SPRING_EXIT_CODE_H

namespace spring {
	enum {
		EXIT_CODE_FAILURE = -2,
		EXIT_CODE_DESYNC  = -1,
		EXIT_CODE_SUCCESS =  0,
		EXIT_CODE_TIMEOUT =  1,
		EXIT_CODE_FORCED  =  2,
	};

	// only here for validation tests
	extern int exitCode;
};

#endif

