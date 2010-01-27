#ifndef __FARTEXTURE_HANDLER_H__
#define __FARTEXTURE_HANDLER_H__

#include <vector>
#include "Vec2.h"
#include "GL/myGL.h"
#include "Rendering/GL/FBO.h"

class CCamera;
struct S3DModel;
class float3;
class CVertexArray;

/**
 * @brief Cheap unit lodding using imposters.
 */
class CFarTextureHandler
{
public:
	CFarTextureHandler();
	~CFarTextureHandler();
	void CreateFarTexture(S3DModel* model);
	void CreateFarTextures();
	GLuint GetTextureID() const { return farTexture; }
	float2 GetTextureCoords(const int& farTextureNum, const int& orientation);
	void DrawFarTexture(const CCamera*, const S3DModel*, const float3&, float, short, CVertexArray*);

	int texSizeX;
	int texSizeY;

	static const int iconSizeX = 32;
	static const int iconSizeY = 32;
	static const int numOrientations = 8;

private:
	int2 GetTextureCoordsInt(const int& farTextureNum, const int& orientation);
	void ReallyCreateFarTexture(S3DModel* model);

	FBO fbo;
	GLuint farTexture;
	int usedFarTextures;
	std::vector<S3DModel*> pending;
};

extern CFarTextureHandler* farTextureHandler;

#endif // __FARTEXTURE_HANDLER_H__
