/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPLINE_H
#define _CPPWRAPPER_WRAPPLINE_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Line.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappLine : public Line {

private:
	int skirmishAIId;
	int lineId;

	WrappLine(int skirmishAIId, int lineId);
	virtual ~WrappLine();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetLineId() const;
public:
	static Line* GetInstance(int skirmishAIId, int lineId);

public:
	// @Override
	virtual springai::AIFloat3 GetFirstPosition();

public:
	// @Override
	virtual springai::AIFloat3 GetSecondPosition();

public:
	// @Override
	virtual springai::AIColor GetColor();
}; // class WrappLine

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPLINE_H

