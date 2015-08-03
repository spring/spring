/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPPATHDRAWER_H
#define _CPPWRAPPER_WRAPPPATHDRAWER_H

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
class WrappPathDrawer : public PathDrawer {

private:
	int skirmishAIId;

	WrappPathDrawer(int skirmishAIId);
	virtual ~WrappPathDrawer();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static PathDrawer* GetInstance(int skirmishAIId);

public:
	// @Override
	virtual void Start(const springai::AIFloat3& pos, const springai::AIColor& color, short alpha);

public:
	// @Override
	virtual void Finish(bool iAmUseless);

public:
	// @Override
	virtual void DrawLine(const springai::AIFloat3& endPos, const springai::AIColor& color, short alpha);

public:
	// @Override
	virtual void DrawLineAndCommandIcon(Command* cmd, const springai::AIFloat3& endPos, const springai::AIColor& color, short alpha);

public:
	// @Override
	virtual void DrawIcon(Command* cmd);

public:
	// @Override
	virtual void Suspend(const springai::AIFloat3& endPos, const springai::AIColor& color, short alpha);

public:
	// @Override
	virtual void Restart(bool sameColor);
}; // class WrappPathDrawer

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPPATHDRAWER_H

