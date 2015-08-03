/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_ABSTRACTLOG_H
#define _CPPWRAPPER_ABSTRACTLOG_H

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
class AbstractLog : public Log {

protected:
	virtual ~AbstractLog();
	// @Override
public:
	virtual int CompareTo(const Log& other);
	// @Override
public:
	virtual bool Equals(const Log& other);
	// @Override
public:
	virtual int HashCode();
	// @Override
public:
	virtual std::string ToString();
}; // class AbstractLog

}  // namespace springai

#endif // _CPPWRAPPER_ABSTRACTLOG_H

