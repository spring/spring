/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BF_GROUND_TEXTURES_H_
#define _BF_GROUND_TEXTURES_H_

#include "Rendering/GL/PBO.h"

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

	const int bigSquareSize;
	const int numBigTexX;
	const int numBigTexY;

	int* textureOffsets;

	struct GroundSquare {
		int texLevel;
		GLuint texture;
		unsigned int lastUsed;
	};

	GroundSquare* squares;

	int* tileMap;
	int tileSize;
	char* tiles;
	int tileMapXSize;
	int tileMapYSize;

	float* heightMaxes;
	float* heightMins;
	float* stretchFactors;

	//! use Pixel Buffer Objects for async. uploading (DMA)
	PBO pbo;

	float anisotropy;

	inline bool TexSquareInView(int, int);
};

#endif // _BF_GROUND_TEXTURES_H_
