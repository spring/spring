/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SMF_GROUND_TEXTURES_H_
#define _SMF_GROUND_TEXTURES_H_

#include "Map/BaseGroundTextures.h"
#include "Rendering/GL/PBO.h"

class CFileHandler;
class CSmfReadMap;

class CSMFGroundTextures: public CBaseGroundTextures
{
public:
	CSMFGroundTextures(CSmfReadMap*);
	~CSMFGroundTextures(void);

	void DrawUpdate(void);
	bool SetSquareLuaTexture(int x, int y, int textureID);
	void BindSquareTexture(int x, int y);
	void LoadSquareTexture(int x, int y, int level);

protected:
	CSmfReadMap* smfMap;

	const int bigSquareSize;
	const int numBigTexX;
	const int numBigTexY;

	struct GroundSquare {
		unsigned int texLevel;
		unsigned int textureID;
		unsigned int lastBoundFrame;
		bool luaTexture;
	};

	std::vector<GroundSquare> squares;

	std::vector<int> tileMap;
	std::vector<char> tiles;

	int tileSize;
	int tileMapXSize;
	int tileMapYSize;

	// FIXME? these are not updated at runtime
	std::vector<float> heightMaxima;
	std::vector<float> heightMinima;
	std::vector<float> stretchFactors;

	//! use Pixel Buffer Objects for async. uploading (DMA)
	PBO pbo;

	float anisotropy;

	inline bool TexSquareInView(int, int);
};

#endif // _BF_GROUND_TEXTURES_H_
