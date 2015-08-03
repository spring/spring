/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPGRAPHDRAWER_H
#define _CPPWRAPPER_WRAPPGRAPHDRAWER_H

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
class WrappGraphDrawer : public GraphDrawer {

private:
	int skirmishAIId;

	WrappGraphDrawer(int skirmishAIId);
	virtual ~WrappGraphDrawer();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static GraphDrawer* GetInstance(int skirmishAIId);

public:
	// @Override
	virtual bool IsEnabled();

public:
	// @Override
	virtual void SetPosition(float x, float y);

public:
	// @Override
	virtual void SetSize(float w, float h);

public:
	// @Override
	virtual springai::GraphLine* GetGraphLine();
}; // class WrappGraphDrawer

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPGRAPHDRAWER_H

