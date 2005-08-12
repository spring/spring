// BFGroundTextures.h
///////////////////////////////////////////////////////////////////////////

#ifndef __BF_GROUND_TEXTURES_H__
#define __BF_GROUND_TEXTURES_H__

#include <jpeglib.h>

class CFileHandler;

class CBFGroundTextures
{
public:
	CBFGroundTextures(CFileHandler* ifs);
	~CBFGroundTextures(void);
	void SetTexture(int x, int y);

protected:
	int numBigTexX;
	int numBigTexY;

	unsigned char* jpegBuffer;
	int* textureOffsets;

	struct GroundSquare{
		int texLevel;
		unsigned int texture;
		int lastUsed;
	};

	GroundSquare* squares;

	struct jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	
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
public:
	void ReadJpeg(int bufnum, unsigned char* buffer, int xstep);
	void SetJpegMemSource(void* inbuffer,int length);
	void DrawUpdate(void);
	void SetupJpeg(void);
	void LoadSquare(int x, int y, int level);
	void AbortRead(void);
	void ReadSlow(int speed);
};

extern CBFGroundTextures* groundTextures;

#endif // __BF_GROUND_TEXTURES_H__
