/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#ifndef _FARTEXTURE_HANDLER_H
#define _FARTEXTURE_HANDLER_H

#include <vector>
#include "System/type2.h"
#include "Rendering/GL/FBO.h"

class CSolidObject;
class CVertexArray;

/**
 * @brief Cheap unit lodding using imposters.
 */
class CFarTextureHandler
{
public:
	CFarTextureHandler();
	~CFarTextureHandler();

	void Queue(const CSolidObject* obj);
	void Draw();

private:
	int texSizeX;
	int texSizeY;

	static const int iconSizeX;
	static const int iconSizeY;
	static const int numOrientations;

	std::vector<const CSolidObject*> queuedForRender;
	std::vector< std::vector<int> > cache;

	FBO fbo;
	unsigned int farTextureID;
	unsigned int usedFarTextures;

	bool HaveFarIcon(const CSolidObject* obj) const;
	bool CheckResizeAtlas();
	float2 GetTextureCoords(const int farTextureNum, const int orientation) const;
	void DrawFarTexture(const CSolidObject* obj, CVertexArray*);
	int2 GetTextureCoordsInt(const int farTextureNum, const int orientation) const;
	void CreateFarTexture(const CSolidObject* obj);
};

extern CFarTextureHandler* farTextureHandler;

#endif // _FARTEXTURE_HANDLER_H
