/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_ABSTRACTFILE_H
#define _CPPWRAPPER_ABSTRACTFILE_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "File.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class AbstractFile : public File {

protected:
	virtual ~AbstractFile();
	// @Override
public:
	virtual int CompareTo(const File& other);
	// @Override
public:
	virtual bool Equals(const File& other);
	// @Override
public:
	virtual int HashCode();
	// @Override
public:
	virtual std::string ToString();
}; // class AbstractFile

}  // namespace springai

#endif // _CPPWRAPPER_ABSTRACTFILE_H

