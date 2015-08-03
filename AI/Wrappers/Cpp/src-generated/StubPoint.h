/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBPOINT_H
#define _CPPWRAPPER_STUBPOINT_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Point.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubPoint : public Point {

protected:
	virtual ~StubPoint();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int pointId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPointId(int pointId);
	// @Override
	virtual int GetPointId() const;
private:
	springai::AIFloat3 position;/* = springai::AIFloat3::NULL_VALUE;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPosition(springai::AIFloat3 position);
	// @Override
	virtual springai::AIFloat3 GetPosition();
private:
	springai::AIColor color;/* = springai::AIColor::NULL_VALUE;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetColor(springai::AIColor color);
	// @Override
	virtual springai::AIColor GetColor();
private:
	const char* label;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLabel(const char* label);
	// @Override
	virtual const char* GetLabel();
}; // class StubPoint

}  // namespace springai

#endif // _CPPWRAPPER_STUBPOINT_H

