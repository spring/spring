/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBLINE_H
#define _CPPWRAPPER_STUBLINE_H

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
class StubLine : public Line {

protected:
	virtual ~StubLine();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int lineId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLineId(int lineId);
	// @Override
	virtual int GetLineId() const;
private:
	springai::AIFloat3 firstPosition;/* = springai::AIFloat3::NULL_VALUE;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFirstPosition(springai::AIFloat3 firstPosition);
	// @Override
	virtual springai::AIFloat3 GetFirstPosition();
private:
	springai::AIFloat3 secondPosition;/* = springai::AIFloat3::NULL_VALUE;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSecondPosition(springai::AIFloat3 secondPosition);
	// @Override
	virtual springai::AIFloat3 GetSecondPosition();
private:
	springai::AIColor color;/* = springai::AIColor::NULL_VALUE;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetColor(springai::AIColor color);
	// @Override
	virtual springai::AIColor GetColor();
}; // class StubLine

}  // namespace springai

#endif // _CPPWRAPPER_STUBLINE_H

