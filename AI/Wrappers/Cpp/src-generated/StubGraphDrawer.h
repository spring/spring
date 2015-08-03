/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBGRAPHDRAWER_H
#define _CPPWRAPPER_STUBGRAPHDRAWER_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "GraphDrawer.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubGraphDrawer : public GraphDrawer {

protected:
	virtual ~StubGraphDrawer();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	bool isEnabled;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetEnabled(bool isEnabled);
	// @Override
	virtual bool IsEnabled();
	// @Override
	virtual void SetPosition(float x, float y);
	// @Override
	virtual void SetSize(float w, float h);
private:
	springai::GraphLine* graphLine;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetGraphLine(springai::GraphLine* graphLine);
	// @Override
	virtual springai::GraphLine* GetGraphLine();
}; // class StubGraphDrawer

}  // namespace springai

#endif // _CPPWRAPPER_STUBGRAPHDRAWER_H

