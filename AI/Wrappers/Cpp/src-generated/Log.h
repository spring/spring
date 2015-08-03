/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_LOG_H
#define _CPPWRAPPER_LOG_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Log {

public:
	virtual ~Log(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * This will end up in infolog
	 */
public:
	virtual void DoLog(const char* const msg) = 0;

	/**
	 * Inform the engine of an error that happend in the interface.
	 * @param   msg       error message
	 * @param   severety  from 10 for minor to 0 for fatal
	 * @param   die       if this is set to true, the engine assumes
	 *                    the interface is in an irreparable state, and it will
	 *                    unload it immediately.
	 */
public:
	virtual void Exception(const char* const msg, int severety, bool die) = 0;

}; // class Log

}  // namespace springai

#endif // _CPPWRAPPER_LOG_H

