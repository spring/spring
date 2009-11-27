#ifndef __FARTEXTURE_HANDLER_H__
#define __FARTEXTURE_HANDLER_H__

#include <vector>
#include "Vec2.h"
#include "GL/myGL.h"
#include "Rendering/GL/FBO.h"

struct S3DModel;

/**
 * @brief Cheap unit lodding using imposters.
 */
class CFartextureHandler
{
public:
	CFartextureHandler();
	~CFartextureHandler();
	void CreateFarTexture(S3DModel* model);
	void CreateFarTextures();
	GLuint GetTextureID() const { return farTexture; }
	float2 GetTextureCoords(const int& farTextureNum, const int& orientation);

	int texSizeX;
	int texSizeY;

	static const int iconSizeX = 64;
	static const int iconSizeY = 64;
	static const int numOrientations = 8;

private:
	int2 GetTextureCoordsInt(const int& farTextureNum, const int& orientation);
	void ReallyCreateFarTexture(S3DModel* model);

	FBO fbo;
	GLuint farTexture;
	int usedFarTextures;
	std::vector<S3DModel*> pending;
};

extern CFartextureHandler* fartextureHandler;

#endif // __FARTEXTURE_HANDLER_H__
