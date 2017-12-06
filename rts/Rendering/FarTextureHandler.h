/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#ifndef _FARTEXTURE_HANDLER_H
#define _FARTEXTURE_HANDLER_H

#include <vector>
#include "System/type2.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/RenderDataBufferFwd.hpp"

class CSolidObject;

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
	bool HaveFarIcon(const CSolidObject* obj) const;
	bool CheckResizeAtlas();

	float2 GetTextureCoords(const int farTextureNum, const int orientation) const;
	int2 GetTextureCoordsInt(const int farTextureNum, const int orientation) const;

	void DrawFarTexture(const CSolidObject* obj, GL::RenderDataBufferTN* rdb);
	void CreateFarTexture(const CSolidObject* obj);

private:
	int2 texSize;
	int2 iconSize;

	struct CachedIcon {
		unsigned int farTexNum;

		float2 texScales;
		float3 texOffset;
	};

	std::vector<const CSolidObject*> renderQueue;
	std::vector<const CSolidObject*> createQueue;
	std::vector< std::vector<CachedIcon> > iconCache;

	FBO fbo;

	unsigned int farTextureID;
	unsigned int usedFarTextures;
};

extern CFarTextureHandler* farTextureHandler;

#endif // _FARTEXTURE_HANDLER_H
