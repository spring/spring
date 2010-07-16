/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BRANCH_PREDICTION_H
#define BRANCH_PREDICTION_H


#ifdef __GNUC__

#define likely(x)	__builtin_expect((x),1)
#define unlikely(x)	__builtin_expect((x),0)

#else

#define likely(x)	(x)
#define unlikely(x)	(x)

#endif


#endif /* BRANCH_PREDICTION_H */
