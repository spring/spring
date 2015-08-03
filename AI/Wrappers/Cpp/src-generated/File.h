/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_FILE_H
#define _CPPWRAPPER_FILE_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class File {

public:
	virtual ~File(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * Return -1 when the file does not exist
	 */
public:
	virtual int GetSize(const char* fileName) = 0;

	/**
	 * Returns false when file does not exist, or the buffer is too small
	 */
public:
	virtual bool GetContent(const char* fileName, void* buffer, int bufferLen) = 0;

}; // class File

}  // namespace springai

#endif // _CPPWRAPPER_FILE_H

