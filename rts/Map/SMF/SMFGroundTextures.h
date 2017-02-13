/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SMF_GROUND_TEXTURES_H_
#define _SMF_GROUND_TEXTURES_H_

#include <vector>

#include "Map/BaseGroundTextures.h"
#include "Rendering/GL/PBO.h"

class CSMFMapFile;
class CSMFReadMap;

class CSMFGroundTextures: public CBaseGroundTextures
{
public:
	CSMFGroundTextures(CSMFReadMap* rm);

	void DrawUpdate();
	bool SetSquareLuaTexture(int texSquareX, int texSquareY, int texID);
	bool GetSquareLuaTexture(int texSquareX, int texSquareY, int texID, int texSizeX, int texSizeY, int texMipLevel);
	void BindSquareTexture(int texSquareX, int texSquareY);

protected:
	void LoadTiles(CSMFMapFile& file);
	void LoadSquareTextures(const int mipLevel);
	void ConvolveHeightMap(const int mapWidth, const int mipLevel);
	bool RecompressTilesIfNeeded();
	void ExtractSquareTiles(const int texSquareX, const int texSquareY, const int mipLevel, GLint* tileBuf) const;
	void LoadSquareTexture(int x, int y, int level);

	inline bool TexSquareInView(int, int) const;

	CSMFReadMap* smfMap;

private:
	struct GroundSquare {
		enum {
			RAW_TEX_IDX = 0,
			LUA_TEX_IDX = 1,
		};

		GroundSquare(): textureIDs{0, 0}, texMipLevel(0), texDrawFrame(1) {}
		~GroundSquare();

		bool HasLuaTexture() const { return (textureIDs[LUA_TEX_IDX] != 0); }

		void SetRawTexture(unsigned int id) { textureIDs[RAW_TEX_IDX] = id; }
		void SetLuaTexture(unsigned int id) { textureIDs[LUA_TEX_IDX] = id; }
		void SetMipLevel(unsigned int l) { texMipLevel = l; }
		void SetDrawFrame(unsigned int f) { texDrawFrame = f; }

		unsigned int* GetTextureIDPtr() { return &textureIDs[RAW_TEX_IDX]; }
		unsigned int GetTextureID() const { return textureIDs[HasLuaTexture()]; }
		unsigned int GetMipLevel() const { return texMipLevel; }
		unsigned int GetDrawFrame() const { return texDrawFrame; }

	private:
		unsigned int textureIDs[2];
		unsigned int texMipLevel;
		unsigned int texDrawFrame;
	};

	std::vector<GroundSquare> squares;

	std::vector<int> tileMap;
	std::vector<char> tiles;

	// FIXME? these are not updated at runtime
	std::vector<float> heightMaxima;
	std::vector<float> heightMinima;
	std::vector<float> stretchFactors;

	// use Pixel Buffer Objects for async. uploading (DMA)
	PBO pbo;

	int tileTexFormat;
};

#endif // _BF_GROUND_TEXTURES_H_
