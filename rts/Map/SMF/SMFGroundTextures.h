/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SMF_GROUND_TEXTURES_H_
#define _SMF_GROUND_TEXTURES_H_

#include "Map/BaseGroundTextures.h"
#include "Rendering/GL/PBO.h"

class CSMFMapFile;
class CSMFReadMap;

class CSMFGroundTextures: public CBaseGroundTextures
{
public:
	CSMFGroundTextures(CSMFReadMap* rm);
	~CSMFGroundTextures();

	void DrawUpdate();
	bool SetSquareLuaTexture(int texSquareX, int texSquareY, int texID);
	bool GetSquareLuaTexture(int texSquareX, int texSquareY, int texID, int texSizeX, int texSizeY, int texMipLevel);
	void BindSquareTexture(int texSquareX, int texSquareY);

protected:
	void LoadTiles(CSMFMapFile& file);
	void LoadSquareTextures(const int mipLevel);
	void ConvolveHeightMap(const int mapWidth, const int mipLevel);
	bool RecompressTiles(bool canRecompress);
	void ExtractSquareTiles(const int texSquareX, const int texSquareY, const int mipLevel, GLint* tileBuf) const;
	void LoadSquareTexture(int x, int y, int level);

	inline bool TexSquareInView(int, int) const;

	CSMFReadMap* smfMap;

private:
	struct GroundSquare {
		unsigned int texLevel;
		unsigned int textureID;
		unsigned int lastBoundFrame;
		bool luaTexture;
	};

	std::vector<GroundSquare> squares;

	std::vector<unsigned int> tileMap;
	std::vector<char> tiles;

	// FIXME? these are not updated at runtime
	std::vector<float> heightMaxima;
	std::vector<float> heightMinima;
	std::vector<float> stretchFactors;

	// use Pixel Buffer Objects for async. uploading (DMA)
	PBO pbo;

	int smallTileMipOffset[5] = {0};
	int &smallTileBytes = smallTileMipOffset[4];
	void calcSmallTileBytes(int size, int type);

	int tileTexFormat;
};

#endif // _BF_GROUND_TEXTURES_H_
