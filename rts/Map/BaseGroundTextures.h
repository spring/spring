/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BASE_GROUND_TEXTURES_H_
#define _BASE_GROUND_TEXTURES_H_

class CBaseGroundTextures {
public:
	virtual ~CBaseGroundTextures() {}
	virtual void DrawUpdate() {}
	virtual bool SetSquareLuaTexture(int texSquareX, int texSquareY, int texID) { return false; }
	virtual bool GetSquareLuaTexture(int texSquareX, int texSquareY, int texID, int texSizeX, int texSizeY, int texMipLevel) { return false; }
	virtual void BindSquareTexture(int x, int y) {}
};

#endif
