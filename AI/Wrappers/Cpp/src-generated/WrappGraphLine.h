/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPGRAPHLINE_H
#define _CPPWRAPPER_WRAPPGRAPHLINE_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "GraphLine.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappGraphLine : public GraphLine {

private:
	int skirmishAIId;

	WrappGraphLine(int skirmishAIId);
	virtual ~WrappGraphLine();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static GraphLine* GetInstance(int skirmishAIId);

public:
	// @Override
	virtual void AddPoint(int lineId, float x, float y);

public:
	// @Override
	virtual void DeletePoints(int lineId, int numPoints);

public:
	// @Override
	virtual void SetColor(int lineId, const springai::AIColor& color);

public:
	// @Override
	virtual void SetLabel(int lineId, const char* label);
}; // class WrappGraphLine

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPGRAPHLINE_H

