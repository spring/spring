/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBPATHDRAWER_H
#define _CPPWRAPPER_STUBPATHDRAWER_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "PathDrawer.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubPathDrawer : public PathDrawer {

protected:
	virtual ~StubPathDrawer();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	// @Override
	virtual void Start(const springai::AIFloat3& pos, const springai::AIColor& color, short alpha);
	// @Override
	virtual void Finish(bool iAmUseless);
	// @Override
	virtual void DrawLine(const springai::AIFloat3& endPos, const springai::AIColor& color, short alpha);
	// @Override
	virtual void DrawLineAndCommandIcon(Command* cmd, const springai::AIFloat3& endPos, const springai::AIColor& color, short alpha);
	// @Override
	virtual void DrawIcon(Command* cmd);
	// @Override
	virtual void Suspend(const springai::AIFloat3& endPos, const springai::AIColor& color, short alpha);
	// @Override
	virtual void Restart(bool sameColor);
}; // class StubPathDrawer

}  // namespace springai

#endif // _CPPWRAPPER_STUBPATHDRAWER_H

