#include "StdAfx.h"
#include "BFGroundTextures.h"
#include "FileSystem/FileHandler.h"
#include "Rendering/GL/myGL.h"
#include "Game/Camera.h"
#include "LogOutput.h"
#include "Rendering/GL/myGL.h"
#include "mapfile.h"
#include "Platform/errorhandler.h"
#include "SmfReadMap.h"
#include "mmgr.h"
#include "FastMath.h"

using std::string;
using std::max;

CBFGroundTextures::CBFGroundTextures(CSmfReadMap* rm) :
	bigSquareSize(128),
	numBigTexX(gs->mapx / bigSquareSize),
	numBigTexY(gs->mapy / bigSquareSize)
{
	usePBO = false;
	if (GLEW_EXT_pixel_buffer_object && rm->usePBO) {
		glGenBuffers(10, pboIDs);
		currentPBO=0;
		usePBO = true;
	}

	CFileHandler* ifs = rm->ifs;
	map = rm;

	SMFHeader* header = &map->header;
	ifs->Seek(header->tilesPtr);

	tileSize = header->tilesize;

	MapTileHeader tileHeader;
	READPTR_MAPTILEHEADER(tileHeader, ifs);

	tileMap = SAFE_NEW int[(header->mapx * header->mapy) / 16];
	tiles = SAFE_NEW char[tileHeader.numTiles * SMALL_TILE_SIZE];
	int curTile = 0;

	for (int a = 0; a < tileHeader.numTileFiles; ++a) {
		PrintLoadMsg("Loading tile file");

		int size;
		ifs->Read(&size, 4);
		size = swabdword(size);
		string name;

		while (true) {
			char ch;
			ifs->Read(&ch, 1);
			/* char, no swab */
			if (ch == 0)
				break;

			name += ch;
		}

		name = string("maps/") + name;
		CFileHandler tileFile(name);

		if (!tileFile.FileExists()) {
			logOutput.Print("Couldnt find tile file %s", name.c_str());
			memset(&tiles[curTile * SMALL_TILE_SIZE], 0xaa, size * SMALL_TILE_SIZE);
			curTile += size;
			continue;
		}

		PrintLoadMsg("Reading tiles");

		TileFileHeader tfh;
		READ_TILEFILEHEADER(tfh, tileFile);

		if (strcmp(tfh.magic, "spring tilefile") != 0 || tfh.version != 1 || tfh.tileSize != 32 || tfh.compressionType != 1) {
			char t[500];
			sprintf(t,"Error couldnt open tile file %s", name.c_str());
			handleerror(0, t, "Error when reading tile file", 0);
			exit(0);
		}

		for (int b = 0; b < size; ++b) {
			tileFile.Read(&tiles[curTile * SMALL_TILE_SIZE], SMALL_TILE_SIZE);
			curTile++;
		}
	}

	PrintLoadMsg("Reading tile map");

	int count = (header->mapx * header->mapy) / 16;
	ifs->Read(tileMap, count * sizeof(int));

	for (int i = 0; i < count; i++) {
		tileMap[i] = swabdword(tileMap[i]);
	}

	tileMapXSize = header->mapx / 4;
	tileMapYSize = header->mapy / 4;

	squares = SAFE_NEW GroundSquare[numBigTexX * numBigTexY];

	for (int y = 0; y < numBigTexY; ++y) {
		for (int x = 0; x < numBigTexX; ++x) {
			GroundSquare* square = &squares[y * numBigTexX + x];
			square->texLevel = 1;
			square->lastUsed = -100;
			LoadSquare(x, y, 2);
		}
	}
}

CBFGroundTextures::~CBFGroundTextures(void)
{
	for (int i = 0; i < numBigTexX * numBigTexY; ++i) {
		glDeleteTextures(1, &squares[i].texture);
	}

	delete[] squares;
	delete[] tileMap;
	delete[] tiles;

	if (usePBO) {
		glDeleteBuffers(10,pboIDs);
	}
}


void CBFGroundTextures::SetTexture(int x, int y)
{
	GroundSquare* square = &squares[y * numBigTexX + x];
	glBindTexture(GL_TEXTURE_2D, square->texture);
	square->lastUsed = gs->frameNum;
}

inline bool CBFGroundTextures::TexSquareInView(int btx, int bty) {
	const float* heightData = map->GetHeightmap();
	static const int heightDataX = gs->mapx + 1;
	static const int bigTexW = (gs->mapx << 3) / numBigTexX;
	static const int bigTexH = (gs->mapy << 3) / numBigTexY;
	static const float bigTexSquareRadius = fastmath::sqrt(float(bigTexW * bigTexW + bigTexH * bigTexH));

	const int x = btx * bigTexW + (bigTexW >> 1);
	const int y = bty * bigTexH + (bigTexH >> 1);
	const int idx = (y >> 3) * heightDataX + (x >> 3);
	const float3 bigTexSquarePos(x, heightData[idx], y);

	return (cam2->InView(bigTexSquarePos, bigTexSquareRadius));
}

void CBFGroundTextures::DrawUpdate(void)
{
	for (int y = 0; y < numBigTexY; ++y) {
		float dy = cam2->pos.z - y * bigSquareSize * SQUARE_SIZE - (SQUARE_SIZE << 6);
		dy = max(0.0f, float(fabs(dy) - (SQUARE_SIZE << 6)));

		for (int x = 0; x < numBigTexX; ++x) {
			if (!TexSquareInView(x, y)) {
				// no need to update this square's
				// texture if we can't even see it
				continue;
			}

			GroundSquare* square = &squares[y * numBigTexX + x];

			float dx = cam2->pos.x - x * bigSquareSize * SQUARE_SIZE - (SQUARE_SIZE << 6);
			dx = max(0.0f, float(fabs(dx) - (SQUARE_SIZE << 6)));
			float dist = fastmath::sqrt(dx * dx + dy * dy);

			if (square->lastUsed < gs->frameNum - 60)
				dist = 8000;

			float wantedLevel = dist / 1000;

			if (wantedLevel > 2.5f)
				wantedLevel = 2.5f;
			if (wantedLevel < square->texLevel - 1)
				wantedLevel = square->texLevel - 1;

			if (square->texLevel != (int) wantedLevel) {
				glDeleteTextures(1, &square->texture);
				LoadSquare(x, y, (int) wantedLevel);
			}
		}
	}
}

int tileoffset[] = {0, 512, 640, 672};

void CBFGroundTextures::LoadSquare(int x, int y, int level)
{
	int size = 1024 >> level;

	GLubyte* buf=NULL; bool usedPBO=false;

	if (usePBO) {
		if (currentPBO > 9) currentPBO=0;
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIDs[currentPBO++]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, size * size / 2, 0, GL_STREAM_DRAW);

		//map the buffer object into client's memory
		buf = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
		usedPBO = true;
	}

	if(buf==NULL) {
		buf = SAFE_NEW GLubyte[size * size / 2];
		usedPBO = false;
	}

	GroundSquare* square = &squares[y * numBigTexX + x];
	square->texLevel = level;

	int numblocks = 8 / (1 << level);

	for (int y1 = 0; y1 < 32; y1++) {
		for (int x1 = 0; x1 < 32; x1++) {
			char* tile = &tiles[tileMap[(x1 + x * 32) + (y1 + y * 32) * tileMapXSize] * SMALL_TILE_SIZE + tileoffset[level]];

			for (int yt = 0; yt < numblocks; yt++) {
				for (int xt = 0; xt < numblocks; xt++) {
					GLint* sbuf = (GLint*)&tile[(xt + yt * numblocks) * 8];
					GLint* dbuf = (GLint*)&buf[(x1 * numblocks + xt + (y1 * numblocks + yt) * (numblocks * 32)) * 8];

					//copy 2x 4 bytes at once
					dbuf[0] = sbuf[0];
					dbuf[1] = sbuf[1];
				}
			}
		}
	}

	glGenTextures(1, &square->texture);
	glBindTexture(GL_TEXTURE_2D, square->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	if (GLEW_EXT_texture_edge_clamp) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	if (map->anisotropy != 0.0f)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, map->anisotropy);

	if (usedPBO) {
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, size, size, 0, size * size / 2, 0);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, 0, 0, GL_STREAM_DRAW); //free it
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}else{
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, size, size, 0, size * size / 2, buf);
		delete[] buf;
	}
}
