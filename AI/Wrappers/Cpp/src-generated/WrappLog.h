/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPLOG_H
#define _CPPWRAPPER_WRAPPLOG_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Log.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappLog : public Log {

private:
	int skirmishAIId;

	WrappLog(int skirmishAIId);
	virtual ~WrappLog();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static Log* GetInstance(int skirmishAIId);

	/**
	 * This will end up in infolog
	 */
public:
	// @Override
	virtual void DoLog(const char* const msg);

	/**
	 * Inform the engine of an error that happend in the interface.
	 * @param   msg       error message
	 * @param   severety  from 10 for minor to 0 for fatal
	 * @param   die       if this is set to true, the engine assumes
	 *                    the interface is in an irreparable state, and it will
	 *                    unload it immediately.
	 */
public:
	// @Override
	virtual void Exception(const char* const msg, int severety, bool die);
}; // class WrappLog

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPLOG_H

