#include "StdAfx.h"

#include "Map/SMF/BFGroundTextures.h"

#include <cmath>
#include <cstdlib>
#include <cstdio>

#include "Map/SMF/mapfile.h"
#include "Map/SMF/SmfReadMap.h"
#include "Map/MapInfo.h"
#include "Game/Camera.h"
#include "Game/Game.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/errorhandler.h"
#include "System/TimeProfiler.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"
#include "System/mmgr.h"
#include "System/FastMath.h"

using std::sprintf;
using std::string;
using std::max;
using std::min;

CBFGroundTextures::CBFGroundTextures(CSmfReadMap* rm) :
	bigSquareSize(128),
	numBigTexX(gs->mapx / bigSquareSize),
	numBigTexY(gs->mapy / bigSquareSize)
{
	usePBO = false;
	if (GLEW_EXT_pixel_buffer_object && rm->usePBO) {
		glGenBuffers(10, pboIDs);
		currentPBO = 0;
		usePBO = true;
	}

	// todo: refactor: put reading code in CSmfFile and keep errorhandling/progress reporting here..
	map = rm;
	CFileHandler* ifs = rm->GetFile().GetFileHandler();
	const SMFHeader* header = &map->GetFile().GetHeader();

	ifs->Seek(header->tilesPtr);
	tileSize = header->tilesize;

	MapTileHeader tileHeader;
	READPTR_MAPTILEHEADER(tileHeader, ifs);

	tileMap = new int[(header->mapx * header->mapy) / 16];
	tiles = new char[tileHeader.numTiles * SMALL_TILE_SIZE];
	int curTile = 0;


	char loadMsg[128] = {0};
	const CMapInfo::smf_t& smf = mapInfo->smf;
	bool smtHeaderOverride = false;

	if (!smf.smtFileNames.empty()) {
		if (smf.smtFileNames.size() != tileHeader.numTileFiles) {
			logOutput.Print(
				"[CBFGroundTextures] mismatched number of .smt file "
				"references between map's .smd ("_STPF_") and header (%d);"
				" ignoring .smd overrides",
				smf.smtFileNames.size(), tileHeader.numTileFiles
			);
		} else {
			smtHeaderOverride = true;
		}
	}


	for (int a = 0; a < tileHeader.numTileFiles; ++a) {
		int numSmallTiles;
		ifs->Read(&numSmallTiles, 4);
		numSmallTiles = swabdword(numSmallTiles);


		std::string tileFileName, smtFileName;

		// eat the zero-terminated name
		while (true) {
			char ch = 0;
			ifs->Read(&ch, 1);

			if (ch == 0) {
				break;
			}

			smtFileName += ch;
		}

		if (!smtHeaderOverride) {
			tileFileName = "maps/" + smtFileName;
		} else {
			tileFileName = "maps/" + smf.smtFileNames[a];
		}


		logOutput.Print("Loading .smt tile-file \"%s\"", tileFileName.c_str());
		SNPRINTF(loadMsg, 127, "Loading %d tiles from file %d/%d", numSmallTiles, a+1, tileHeader.numTileFiles);
		PrintLoadMsg(loadMsg);


		CFileHandler tileFile(tileFileName);

		if (!tileFile.FileExists()) {
			logOutput.Print(
				"[CBFGroundTextures] could not find .smt tile-file "
				"\"%s\" (all %d missing tiles will be colored red)",
				tileFileName.c_str(), numSmallTiles
			);
			memset(&tiles[curTile * SMALL_TILE_SIZE], 0xaa, numSmallTiles * SMALL_TILE_SIZE);
			curTile += numSmallTiles;
			continue;
		}


		PrintLoadMsg("Reading tiles");

		TileFileHeader tfh;
		READ_TILEFILEHEADER(tfh, tileFile);

		if (strcmp(tfh.magic, "spring tilefile") != 0 || tfh.version != 1 || tfh.tileSize != 32 || tfh.compressionType != 1) {
			char t[500];
			sprintf(t, "[CBFGroundTextures] file \"%s\" does not match .smt format", tileFileName.c_str());
			handleerror(0, t, "Error reading tile-file", 0);
			exit(0);
		}

		for (int b = 0; b < numSmallTiles; ++b) {
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

	squares = new GroundSquare[numBigTexX * numBigTexY];

	for (int y = 0; y < numBigTexY; ++y) {
		for (int x = 0; x < numBigTexX; ++x) {
			GroundSquare* square = &squares[y * numBigTexX + x];
			square->lastUsed = 1;
			LoadSquare(x, y, 3);
		}
	}


	ScopedOnceTimer timer("generating MipMaps");
	const int nb = numBigTexX * numBigTexY;
	heightMaxes = new float[nb];
	heightMins = new float[nb];
	stretchFactors = new float[nb];
	const float * hdata= map->mipHeightmap[1];
	const int mx=header->mapx/2;
	const int nbx=numBigTexX;

	for (int y = 0; y < numBigTexY; ++y) {
		for (int x = 0; x < numBigTexX; ++x) {
			heightMaxes[y*numBigTexX+x]	=-100000;
			heightMins[y*numBigTexX+x]	=100000;
			stretchFactors[y*numBigTexX+x]=0;
			for (int x2=x*64+1;x2<(x+1)*64-1;x2++){ //64 is the mipped heightmap square size
				for (int y2=y*64+1;y2<(y+1)*64-1;y2++){ //we leave out the borders on sampling because it is easier to do the Sobel kernel convolution
					heightMaxes[y*nbx+x] = max( hdata[y2*mx+x2] , heightMaxes[y*nbx+x]	);
					heightMins[y*nbx+x] =  min( hdata[y2*mx+x2] , heightMins[y*nbx+x]	);
					float gx =	-1 * hdata[(y2-1) * mx + x2-1] + //Gx sobel kernel
								-2 * hdata[(y2  ) * mx + x2-1] +
								-1 * hdata[(y2+1) * mx + x2-1] +
								 1 * hdata[(y2-1) * mx + x2+1] +
								 2 * hdata[(y2  ) * mx + x2+1] +
								 1 * hdata[(y2+1) * mx + x2+1] ;
					gx = fabs(gx);

					float gy =	-1 * hdata[(y2+1) * mx + x2-1] + //Gy sobel kernel
								-2 * hdata[(y2+1) * mx + x2  ] +
								-1 * hdata[(y2+1) * mx + x2+1] +
								 1 * hdata[(y2-1) * mx + x2-1] +
								 2 * hdata[(y2-1) * mx + x2  ] +
								 1 * hdata[(y2-1) * mx + x2+1] ;
					gy = fabs(gy);

					float g = (gx+gy)/64; //linear sum, no need for fancy sqrt
					g *= g;
					/*square to amplify large stretches of height.
					in fact, this should probably be different,
					as g of 64 (8*(1+2+1+1+2+1) would mean a 45 degree angle (which is what I think is streched),
					we should divide by 64 before squarification to supress lower values*/ 
					stretchFactors[y*nbx+x] += g;

				}
			}
			stretchFactors[y*nbx+x]++;
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

	delete[] heightMaxes;
	delete[] heightMins;
	delete[] stretchFactors;
}


void CBFGroundTextures::SetTexture(int x, int y)
{
	GroundSquare* square = &squares[y * numBigTexX + x];
	glBindTexture(GL_TEXTURE_2D, square->texture);

	if (game->GetDrawMode() == CGame::gameNormalDraw) {
		square->lastUsed = gu->drawFrame;
	}
}

inline bool CBFGroundTextures::TexSquareInView(int btx, int bty) {
	const float* heightData = map->GetHeightmap();
	static const int heightDataX = gs->mapx + 1;
	static const int bigTexW = (gs->mapx << 3) / numBigTexX;
	static const int bigTexH = (gs->mapy << 3) / numBigTexY;
	static const float bigTexSquareRadius = fastmath::apxsqrt(float(bigTexW * bigTexW + bigTexH * bigTexH));

	const int x = btx * bigTexW + (bigTexW >> 1);
	const int y = bty * bigTexH + (bigTexH >> 1);
	const int idx = (y >> 3) * heightDataX + (x >> 3);
	const float3 bigTexSquarePos(x, heightData[idx], y);

	return (cam2->InView(bigTexSquarePos, bigTexSquareRadius));
}

void CBFGroundTextures::DrawUpdate(void)
{
	float diag = fastmath::apxsqrt(gu->viewSizeX*gu->viewSizeX + gu->viewSizeY*gu->viewSizeY); //screen diagonal number of pixels
	for (int y = 0; y < numBigTexY; ++y) {
		float dy = cam2->pos.z - y * bigSquareSize * SQUARE_SIZE - (SQUARE_SIZE << 6);
		dy = max(0.0f, float(fabs(dy) - (SQUARE_SIZE << 6)));

		for (int x = 0; x < numBigTexX; ++x) {
			GroundSquare* square = &squares[y * numBigTexX + x];

			if (!TexSquareInView(x, y)) {
				if ((square->texLevel < 3) && (gu->drawFrame - square->lastUsed > 120)) {
					// `unload` texture (= load lowest mipmap)
					// if the square wasn't visible for 120 vframes
					glDeleteTextures(1, &square->texture);
					LoadSquare(x, y, 3);
				}
				continue;
			}

			float dx = cam2->pos.x - x * bigSquareSize * SQUARE_SIZE - (SQUARE_SIZE << 6);
			dx = max(0.0f, float(fabs(dx) - (SQUARE_SIZE << 6)));
			float dz = max( cam2->pos.y - (heightMaxes[y * numBigTexX + x] + heightMins[y * numBigTexX + x])/2 ,0.0f);
			float dist = fastmath::apxsqrt(dx * dx + dy * dy + dz * dz);

			// so, we shall work under the following assumptions:
			// the minimum mip level is the closest ceiling mip level that we can use based on distance, FOV and tile size.
			// we can increase this mip level IF the stretch factor requires us to do so.
			// for simplicitys sake we will approximate tile size with a sphere of 512 elmos radius- which is =~ a sqrt2*1024 =~ 1400 diag pixels diameter sphere.
			// half fov is 45 degs, for default ta and most other camera modes
			int wantedLevel =0;
			float dh=heightMaxes[y * numBigTexX + x] - heightMins[y * numBigTexX + x];
			float sp=0; //screenpixels
			if (dh > 1024) // this means that is the heightmap chunk is taller than it is wide, then we use the tallness metric instead of the width for calculating the size of it on screen.
				sp = (dh)*(diag/2)/dist; //dist and viewsize based number (screenpixels).
			else
				sp = 1024*(diag/2)/dist;

			if (sp>513)
				wantedLevel=0;
			else if (sp > 257)
				wantedLevel=1;
			else if (sp > 129)
				wantedLevel=2;
			else
				wantedLevel=3;

			if (stretchFactors[y*numBigTexX+x]>16000 && wantedLevel>0) //16k is an approximation of the sobel sum required to have a heightmap that has double the texture area than a flat square.
				wantedLevel--;

			if (wantedLevel > 3)
				wantedLevel = 3;

			if (square->texLevel != wantedLevel) {
				glDeleteTextures(1, &square->texture);
				LoadSquare(x, y, wantedLevel);
			}
		}
	}
}

int tileoffset[] = {0, 512, 640, 672};

void CBFGroundTextures::LoadSquare(int x, int y, int level)
{
	int size = 1024 >> level;

	GLint* buf = NULL;
	bool usedPBO = false;

	if (usePBO) {
		if (currentPBO > 9) currentPBO = 0;
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIDs[currentPBO++]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, size * size / 2, 0, GL_STREAM_DRAW);

		//! map the buffer object into client's memory
		buf = (GLint*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
		usedPBO = true;
	}

	if (buf == NULL) {
		buf = (GLint*)(new GLubyte[size * size / 2]);
		usedPBO = false;
	}

	GroundSquare* square = &squares[y * numBigTexX + x];
	square->texLevel = level;
	int numblocks = 8 >> level;

	for (int y1 = 0; y1 < 32; y1++) {
		for (int x1 = 0; x1 < 32; x1++) {
			GLint* tile = (GLint*)&tiles[tileMap[(x1 + x * 32) + (y1 + y * 32) * tileMapXSize] * SMALL_TILE_SIZE + tileoffset[level]];

			const int doff = x1 * numblocks + y1 * numblocks * numblocks * 32;
			for (int yt = 0; yt < numblocks; yt++) {
				GLint* sbuf = &tile[yt * numblocks * 2];
				GLint* dbuf = &buf[(doff + yt * numblocks * 32) * 2];
				memcpy(dbuf, sbuf, numblocks * 2 * sizeof(GLint));
			}
		}
	}

	glGenTextures(1, &square->texture);
	glBindTexture(GL_TEXTURE_2D, square->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	if (GLEW_EXT_texture_edge_clamp) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	if (map->anisotropy != 0.0f)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, map->anisotropy);
	if (level<2) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 1);
	} else {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 0.5f);
	}

	if (usedPBO) {
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, size, size, 0, size * size / 2, 0);
		if (!gu->atiHacks)
			glBufferData(GL_PIXEL_UNPACK_BUFFER, 0, 0, GL_STREAM_DRAW); //discard old content
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	} else {
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, size, size, 0, size * size / 2, buf);
		delete[] buf;
	}
}
