/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBFILE_H
#define _CPPWRAPPER_STUBFILE_H

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
class StubFile : public File {

protected:
	virtual ~StubFile();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	/**
	 * Return -1 when the file does not exist
	 */
	// @Override
	virtual int GetSize(const char* fileName);
	/**
	 * Returns false when file does not exist, or the buffer is too small
	 */
	// @Override
	virtual bool GetContent(const char* fileName, void* buffer, int bufferLen);
}; // class StubFile

}  // namespace springai

#endif // _CPPWRAPPER_STUBFILE_H

