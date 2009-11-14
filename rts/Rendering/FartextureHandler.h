#ifndef __FARTEXTURE_HANDLER_H__
#define __FARTEXTURE_HANDLER_H__

#include <vector>
#include "GL/myGL.h"

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

private:
	void ReallyCreateFarTexture(S3DModel* model);

	GLuint farTexture;
	unsigned char* farTextureMem;
	int usedFarTextures;
	std::vector<S3DModel*> pending;
};

extern CFartextureHandler* fartextureHandler;

#endif // __FARTEXTURE_HANDLER_H__
