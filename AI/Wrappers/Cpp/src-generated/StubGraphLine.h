/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBGRAPHLINE_H
#define _CPPWRAPPER_STUBGRAPHLINE_H

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
class StubGraphLine : public GraphLine {

protected:
	virtual ~StubGraphLine();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	// @Override
	virtual void AddPoint(int lineId, float x, float y);
	// @Override
	virtual void DeletePoints(int lineId, int numPoints);
	// @Override
	virtual void SetColor(int lineId, const springai::AIColor& color);
	// @Override
	virtual void SetLabel(int lineId, const char* label);
}; // class StubGraphLine

}  // namespace springai

#endif // _CPPWRAPPER_STUBGRAPHLINE_H

