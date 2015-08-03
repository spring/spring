/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBGUI_H
#define _CPPWRAPPER_STUBGUI_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Gui.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubGui : public Gui {

protected:
	virtual ~StubGui();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	float viewRange;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetViewRange(float viewRange);
	// @Override
	virtual float GetViewRange();
private:
	float screenX;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetScreenX(float screenX);
	// @Override
	virtual float GetScreenX();
private:
	float screenY;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetScreenY(float screenY);
	// @Override
	virtual float GetScreenY();
private:
	springai::Camera* camera;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCamera(springai::Camera* camera);
	// @Override
	virtual springai::Camera* GetCamera();
}; // class StubGui

}  // namespace springai

#endif // _CPPWRAPPER_STUBGUI_H

