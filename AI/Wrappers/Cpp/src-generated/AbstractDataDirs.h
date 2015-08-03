/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_ABSTRACTDATADIRS_H
#define _CPPWRAPPER_ABSTRACTDATADIRS_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "DataDirs.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class AbstractDataDirs : public DataDirs {

protected:
	virtual ~AbstractDataDirs();
	// @Override
public:
	virtual int CompareTo(const DataDirs& other);
	// @Override
public:
	virtual bool Equals(const DataDirs& other);
	// @Override
public:
	virtual int HashCode();
	// @Override
public:
	virtual std::string ToString();
}; // class AbstractDataDirs

}  // namespace springai

#endif // _CPPWRAPPER_ABSTRACTDATADIRS_H

