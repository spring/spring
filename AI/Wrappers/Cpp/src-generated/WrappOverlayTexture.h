/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPOVERLAYTEXTURE_H
#define _CPPWRAPPER_WRAPPOVERLAYTEXTURE_H

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
class WrappOverlayTexture : public OverlayTexture {

private:
	int skirmishAIId;
	int overlayTextureId;

	WrappOverlayTexture(int skirmishAIId, int overlayTextureId);
	virtual ~WrappOverlayTexture();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetOverlayTextureId() const;
public:
	static OverlayTexture* GetInstance(int skirmishAIId, int overlayTextureId);

public:
	// @Override
	virtual void Update(const float* texData, int x, int y, int w, int h);

public:
	// @Override
	virtual void Remove();

public:
	// @Override
	virtual void SetPosition(float x, float y);

public:
	// @Override
	virtual void SetSize(float w, float h);

public:
	// @Override
	virtual void SetLabel(const char* label);
}; // class WrappOverlayTexture

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPOVERLAYTEXTURE_H

