/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_PATHDRAWER_H
#define _CPPWRAPPER_PATHDRAWER_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class PathDrawer {

public:
	virtual ~PathDrawer(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual void Start(const springai::AIFloat3& pos, const springai::AIColor& color, short alpha) = 0;

public:
	virtual void Finish(bool iAmUseless) = 0;

public:
	virtual void DrawLine(const springai::AIFloat3& endPos, const springai::AIColor& color, short alpha) = 0;

public:
	virtual void DrawLineAndCommandIcon(Command* cmd, const springai::AIFloat3& endPos, const springai::AIColor& color, short alpha) = 0;

public:
	virtual void DrawIcon(Command* cmd) = 0;

public:
	virtual void Suspend(const springai::AIFloat3& endPos, const springai::AIColor& color, short alpha) = 0;

public:
	virtual void Restart(bool sameColor) = 0;

}; // class PathDrawer

}  // namespace springai

#endif // _CPPWRAPPER_PATHDRAWER_H

