#ifndef __FARTEXTURE_HANDLER_H__
#define __FARTEXTURE_HANDLER_H__

#include "archdef.h"

struct S3DOModel;

class CFartextureHandler
{
public:
	unsigned int farTexture;

	CFartextureHandler(void);
	~CFartextureHandler(void);
	void CreateFarTexture(S3DOModel *model);

private:
	unsigned char* farTextureMem;
	int usedFarTextures;
};

extern CFartextureHandler* fartextureHandler;

#endif // __FARTEXTURE_HANDLER_H__
