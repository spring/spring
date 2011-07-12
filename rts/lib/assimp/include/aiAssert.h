/** @file aiAssert.h
 */
#ifndef AI_DEBUG_H_INC
#define AI_DEBUG_H_INC

#ifdef _DEBUG  
#	include <assert.h>
#	define	ai_assert(expression) assert(expression)
#else
#	define	ai_assert(expression)
#endif


#endif
