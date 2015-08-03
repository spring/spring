/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBOVERLAYTEXTURE_H
#define _CPPWRAPPER_STUBOVERLAYTEXTURE_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "OverlayTexture.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubOverlayTexture : public OverlayTexture {

protected:
	virtual ~StubOverlayTexture();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int overlayTextureId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetOverlayTextureId(int overlayTextureId);
	// @Override
	virtual int GetOverlayTextureId() const;
	// @Override
	virtual void Update(const float* texData, int x, int y, int w, int h);
	// @Override
	virtual void Remove();
	// @Override
	virtual void SetPosition(float x, float y);
	// @Override
	virtual void SetSize(float w, float h);
	// @Override
	virtual void SetLabel(const char* label);
}; // class StubOverlayTexture

}  // namespace springai

#endif // _CPPWRAPPER_STUBOVERLAYTEXTURE_H

