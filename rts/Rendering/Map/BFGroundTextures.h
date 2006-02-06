// BFGroundTextures.h
///////////////////////////////////////////////////////////////////////////

#ifndef __BF_GROUND_TEXTURES_H__
#define __BF_GROUND_TEXTURES_H__

class CFileHandler;

class CBFGroundTextures
{
public:
	CBFGroundTextures(CFileHandler* ifs);
	~CBFGroundTextures(void);
	void SetTexture(int x, int y);
	void DrawUpdate(void);
	void LoadSquare(int x, int y, int level);

protected:
	int numBigTexX;
	int numBigTexY;

	int* textureOffsets;

	struct GroundSquare{
		int texLevel;
		unsigned int texture;
		int lastUsed;
	};

	GroundSquare* squares;

	//variables controlling background reading of textures
	bool inRead;
	int readProgress;
	int readX;
	int readY;
	GroundSquare* readSquare;
	int readLevel;
	unsigned char* readBuffer;
	unsigned char* readTempLine;

	int *tileMap;
	int tileSize;
	char *tiles;
	int tileMapXSize;
	int tileMapYSize;
};

extern CBFGroundTextures* groundTextures;

#endif // __BF_GROUND_TEXTURES_H__
