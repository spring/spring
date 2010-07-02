/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#ifndef __FARTEXTURE_HANDLER_H__
#define __FARTEXTURE_HANDLER_H__

#include <vector>
#include "Vec2.h"
#include "GL/myGL.h"
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
	GLuint GetTextureID() const { return farTexture; }

	void Queue(const CSolidObject* obj);
	void Draw();

private:
	int texSizeX;
	int texSizeY;

	static const int iconSizeX;
	static const int iconSizeY;
	static const int numOrientations;

	GML_VECTOR<const CSolidObject*> queuedForRender;
	std::vector< std::vector<int> > cache;

	FBO fbo;
	GLuint farTexture;
	int usedFarTextures;

	float2 GetTextureCoords(const int& farTextureNum, const int& orientation);
	void DrawFarTexture(const CSolidObject* obj, CVertexArray*);
	int2 GetTextureCoordsInt(const int& farTextureNum, const int& orientation);
	void CreateFarTexture(const CSolidObject* obj);
};

extern CFarTextureHandler* farTextureHandler;

#endif // __FARTEXTURE_HANDLER_H__
