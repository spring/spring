// BFGroundTextures.h
///////////////////////////////////////////////////////////////////////////

#ifndef __BF_GROUND_TEXTURES_H__
#define __BF_GROUND_TEXTURES_H__

#include "Rendering/GL/myGL.h"

class CFileHandler;
class CSmfReadMap;

class CBFGroundTextures
{
public:
	CBFGroundTextures(CSmfReadMap* srm);
	~CBFGroundTextures(void);
	void SetTexture(int x, int y);
	void DrawUpdate(void);
	void LoadSquare(int x, int y, int level);

protected:
	CSmfReadMap* map;

	int bigSquareSize;
	int numBigTexX;
	int numBigTexY;

	int* textureOffsets;

	struct GroundSquare {
		int texLevel;
		GLuint texture;
		int lastUsed;
	};

	GroundSquare* squares;

	int* tileMap;
	int tileSize;
	char* tiles;
	int tileMapXSize;
	int tileMapYSize;

	//! use Pixel Buffer Objects for async. uploading (DMA)?
	bool usePBO;
	GLuint pboIDs[10];
	int currentPBO;

	float anisotropy;

	inline bool TexSquareInView(int, int);
};

#endif // __BF_GROUND_TEXTURES_H__
