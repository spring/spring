/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BASE_GROUND_TEXTURES_H_
#define _BASE_GROUND_TEXTURES_H_

class CBaseGroundTextures {
public:
	virtual ~CBaseGroundTextures() {}
	virtual void DrawUpdate(void) {}
	virtual bool SetSquareLuaTexture(int x, int y, int textureID) { return false; }
	virtual void BindSquareTexture(int x, int y) {}
};

#endif
