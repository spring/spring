/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPFILE_H
#define _CPPWRAPPER_WRAPPFILE_H

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
class WrappFile : public File {

private:
	int skirmishAIId;

	WrappFile(int skirmishAIId);
	virtual ~WrappFile();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static File* GetInstance(int skirmishAIId);

	/**
	 * Return -1 when the file does not exist
	 */
public:
	// @Override
	virtual int GetSize(const char* fileName);

	/**
	 * Returns false when file does not exist, or the buffer is too small
	 */
public:
	// @Override
	virtual bool GetContent(const char* fileName, void* buffer, int bufferLen);
}; // class WrappFile

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPFILE_H

