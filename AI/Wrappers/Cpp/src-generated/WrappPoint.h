/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPPOINT_H
#define _CPPWRAPPER_WRAPPPOINT_H

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
class WrappPoint : public Point {

private:
	int skirmishAIId;
	int pointId;

	WrappPoint(int skirmishAIId, int pointId);
	virtual ~WrappPoint();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetPointId() const;
public:
	static Point* GetInstance(int skirmishAIId, int pointId);

public:
	// @Override
	virtual springai::AIFloat3 GetPosition();

public:
	// @Override
	virtual springai::AIColor GetColor();

public:
	// @Override
	virtual const char* GetLabel();
}; // class WrappPoint

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPPOINT_H

