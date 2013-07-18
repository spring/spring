/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <cmath>
#include <cstdlib>
#include <cstdio>
#if defined(USE_LIBSQUISH) && !defined(HEADLESS)
	#include "lib/squish/squish.h"
	#include "lib/rg-etc1/rg_etc1.h"
#endif

#include "SMFGroundTextures.h"
#include "SMFFormat.h"
#include "SMFReadMap.h"
#include "Rendering/GL/PBO.h"
#include "Rendering/GlobalRendering.h"
#include "Map/MapInfo.h"
#include "Game/Camera.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/LoadScreen.h"
#include "System/Exceptions.h"
#include "System/FastMath.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/ThreadPool.h"

using std::sprintf;

#define LOG_SECTION_SMF_GROUND_TEXTURES "CSMFGroundTextures"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_SMF_GROUND_TEXTURES)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_SMF_GROUND_TEXTURES


CSMFGroundTextures::CSMFGroundTextures(CSMFReadMap* rm): smfMap(rm)
{
	// TODO refactor: put reading code in CSMFFile and keep error-handling/progress reporting here
	      CSMFMapFile& file = smfMap->GetFile();
	const SMFHeader& header = file.GetHeader();
	const std::string smfDir = FileSystem::GetDirectory(gameSetup->MapFile());
	const CMapInfo::smf_t& smf = mapInfo->smf;

	assert(gs->mapx == header.mapx);
	assert(gs->mapy == header.mapy);

	CFileHandler* ifs = file.GetFileHandler();
	ifs->Seek(header.tilesPtr);

	MapTileHeader tileHeader;
	READPTR_MAPTILEHEADER(tileHeader, ifs);

	tileMap.resize(smfMap->tileCount);
	tiles.resize(tileHeader.numTiles * SMALL_TILE_SIZE);
	squares.resize(smfMap->numBigTexX * smfMap->numBigTexY);

	bool smtHeaderOverride = false;
	int curTile = 0;

	if (!smf.smtFileNames.empty()) {
		if (smf.smtFileNames.size() != tileHeader.numTileFiles) {
			LOG_L(L_WARNING,
					"mismatched number of .smt file "
					"references between the map's .smd (" _STPF_ ")"
					" and header (%d); ignoring .smd overrides",
					smf.smtFileNames.size(), tileHeader.numTileFiles);
		} else {
			smtHeaderOverride = true;
		}
	}


	loadscreen->SetLoadMessage("Loading Tile Files");

	for (int a = 0; a < tileHeader.numTileFiles; ++a) {
		int numSmallTiles;
		ifs->Read(&numSmallTiles, sizeof(int));
		swabDWordInPlace(numSmallTiles);


		std::string smtFilePath, smtFileName;

		//! eat the zero-terminated name
		while (true) {
			char ch = 0;
			ifs->Read(&ch, sizeof(char));

			if (ch == 0) {
				break;
			}

			smtFileName += ch;
		}

		if (!smtHeaderOverride) {
			smtFilePath = smfDir + smtFileName;
		} else {
			smtFilePath = smfDir + smf.smtFileNames[a];
		}

		CFileHandler tileFile(smtFilePath);

		if (!tileFile.FileExists()) {
			//! try absolute path
			if (!smtHeaderOverride) {
				smtFilePath = smtFileName;
			} else {
				smtFilePath = smf.smtFileNames[a];
			}
			tileFile.Open(smtFilePath);
		}

		if (!tileFile.FileExists()) {
			LOG_L(L_WARNING,
					"could not find .smt tile-file "
					"\"%s\" (all %d missing tiles will be colored red)",
					smtFilePath.c_str(), numSmallTiles);
			memset(&tiles[curTile * SMALL_TILE_SIZE], 0xaa, numSmallTiles * SMALL_TILE_SIZE);
			curTile += numSmallTiles;
			continue;
		}

		TileFileHeader tfh;
		READ_TILEFILEHEADER(tfh, tileFile);

		if (strcmp(tfh.magic, "spring tilefile") != 0 || tfh.version != 1 || tfh.tileSize != 32 || tfh.compressionType != 1) {
			char t[500];
			sprintf(t, "[CSMFGroundTextures] file \"%s\" does not match .smt format", smtFilePath.c_str());
			throw content_error(t);
		}

		for (int b = 0; b < numSmallTiles; ++b) {
			tileFile.Read(&tiles[curTile * SMALL_TILE_SIZE], SMALL_TILE_SIZE);
			curTile++;
		}
	}

	loadscreen->SetLoadMessage("Reading Tile Map");
	{
		ifs->Read(&tileMap[0], smfMap->tileCount * sizeof(int));

		for (int i = 0; i < smfMap->tileCount; i++) {
			swabDWordInPlace(tileMap[i]);
		}
	}

	texformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;

#if defined(USE_LIBSQUISH) && !defined(HEADLESS) && defined(GLEW_ARB_ES3_compatibility)
	// Not all FOSS drivers support S3TC, use ETC1 for those
	const bool useETC1 = (!GLEW_EXT_texture_compression_s3tc) && GLEW_ARB_ES3_compatibility;
	if (useETC1) {
		// note 1: Mesa should support this
		// note 2: Nvidia supports ETC but preprocesses the texture (on the CPU) each upload = slow -> makes no sense to add it as another map compression format
		// note 3: for both DXT1 & ETC1/2 blocksize is 8 bytes per 4x4 pixel block -> perfect for us :)

		loadscreen->SetLoadMessage("Recompress MapTiles with ETC1");

		texformat = GL_COMPRESSED_RGB8_ETC2; // ETC2 is backward compatible with ETC1! GLEW doesn't have the ETC1 extension :<

		const int numTiles = tiles.size() / 8;

		rg_etc1::pack_etc1_block_init();
		rg_etc1::etc1_pack_params pack_params;
		pack_params.m_quality = rg_etc1::cLowQuality; // must be low, all others take _ages_ to process

		for_mt(0, numTiles, [&](const int i) {
			squish::u8 rgba[64]; // 4x4 pixels * 4 * 1byte channels = 64byte
			squish::Decompress(rgba, &tiles[i * 8], squish::kDxt1);
			rg_etc1::pack_etc1_block(&tiles[i * 8], (const unsigned int*)rgba, pack_params);
		});
	}
#endif

	loadscreen->SetLoadMessage("Loading Square Textures");

	for (int y = 0; y < smfMap->numBigTexY; ++y) {
		for (int x = 0; x < smfMap->numBigTexX; ++x) {
			GroundSquare* square = &squares[y * smfMap->numBigTexX + x];
			square->texLevel       = 0;
			square->textureID      = 0;
			square->lastBoundFrame = 1;
			square->luaTexture     = false;

			LoadSquareTexture(x, y, 3);
		}
	}


	ScopedOnceTimer timer("CSMFGroundTextures::ConvolveHeightMap");

	const float* hdata = readmap->GetMIPHeightMapSynced(1);
	const int mx = header.mapx / 2;
	const int nbx = smfMap->numBigTexX;
	const int nb = smfMap->numBigTexX * smfMap->numBigTexY;

	// 64 is the heightmap square-size at MIP level 1 (bigSquareSize >> 1)
	static const int mipSquareSize = 64;

	heightMaxima.resize(nb, readmap->currMinHeight);
	heightMinima.resize(nb, readmap->currMaxHeight);
	stretchFactors.resize(nb, 0.0f);

	for (int y = 0; y < smfMap->numBigTexY; ++y) {
		for (int x = 0; x < smfMap->numBigTexX; ++x) {

			// NOTE: we leave out the borders on sampling because it is easier to do the Sobel kernel convolution
			for (int x2 = x * mipSquareSize + 1; x2 < (x + 1) * mipSquareSize - 1; x2++) {
				for (int y2 = y * mipSquareSize + 1; y2 < (y + 1) * mipSquareSize - 1; y2++) {
					heightMaxima[y * nbx + x] = std::max( hdata[y2 * mx + x2], heightMaxima[y * nbx + x]);
					heightMinima[y * nbx + x] = std::min( hdata[y2 * mx + x2], heightMinima[y * nbx + x]);

					// Gx sobel kernel
					const float gx =
						-1.0f * hdata[(y2-1) * mx + x2-1] +
						-2.0f * hdata[(y2  ) * mx + x2-1] +
						-1.0f * hdata[(y2+1) * mx + x2-1] +
						 1.0f * hdata[(y2-1) * mx + x2+1] +
						 2.0f * hdata[(y2  ) * mx + x2+1] +
						 1.0f * hdata[(y2+1) * mx + x2+1];
					// Gy sobel kernel
					const float gy =
						-1.0f * hdata[(y2+1) * mx + x2-1] +
						-2.0f * hdata[(y2+1) * mx + x2  ] +
						-1.0f * hdata[(y2+1) * mx + x2+1] +
						 1.0f * hdata[(y2-1) * mx + x2-1] +
						 2.0f * hdata[(y2-1) * mx + x2  ] +
						 1.0f * hdata[(y2-1) * mx + x2+1];

					// linear sum, no need for fancy sqrt
					const float g = (math::fabs(gx) + math::fabs(gy)) / mipSquareSize;

					/*
					 * square g to amplify large stretches of height.
					 * in fact, this should probably be different, as
					 * g of 64 (8*(1+2+1+1+2+1) would mean a 45 degree
					 * angle (which is what I think is stretched), we
					 * should divide by 64 before squarification to
					 * suppress lower values
					 */
					stretchFactors[y * nbx + x] += (g * g);
				}
			}

			stretchFactors[y * nbx + x] += 1.0f;
		}
	}
}

CSMFGroundTextures::~CSMFGroundTextures()
{
	for (int i = 0; i < smfMap->numBigTexX * smfMap->numBigTexY; ++i) {
		if (!squares[i].luaTexture) {
			glDeleteTextures(1, &squares[i].textureID);
		}
	}
}


inline bool CSMFGroundTextures::TexSquareInView(int btx, int bty) const
{
	static const float* hm = readmap->GetCornerHeightMapUnsynced();

	static const float bigTexSquareRadius = fastmath::apxsqrt(
		smfMap->bigTexSize * smfMap->bigTexSize +
		smfMap->bigTexSize * smfMap->bigTexSize
	);

	const int x = btx * smfMap->bigTexSize + (smfMap->bigTexSize >> 1);
	const int y = bty * smfMap->bigTexSize + (smfMap->bigTexSize >> 1);
	const int idx = (y >> 3) * smfMap->heightMapSizeX + (x >> 3);
	const float3 bigTexSquarePos(x, hm[idx], y);

	return (cam2->InView(bigTexSquarePos, bigTexSquareRadius));
}

void CSMFGroundTextures::DrawUpdate()
{
	// screen-diagonal number of pixels
	const float vsxSq = globalRendering->viewSizeX * globalRendering->viewSizeX;
	const float vsySq = globalRendering->viewSizeY * globalRendering->viewSizeY;
	const float vdiag = fastmath::apxsqrt(vsxSq + vsySq);

	for (int y = 0; y < smfMap->numBigTexY; ++y) {
		float dz = cam2->GetPos().z - (y * smfMap->bigSquareSize * SQUARE_SIZE);
		dz -= (SQUARE_SIZE << 6);
		dz = std::max(0.0f, float(math::fabs(dz) - (SQUARE_SIZE << 6)));

		for (int x = 0; x < smfMap->numBigTexX; ++x) {
			GroundSquare* square = &squares[y * smfMap->numBigTexX + x];

			if (square->luaTexture) {
				// no deletion or mip-level selection
				continue;
			}

			if (!TexSquareInView(x, y)) {
				if ((square->texLevel < 3) && (globalRendering->drawFrame - square->lastBoundFrame > 120)) {
					// `unload` texture (load lowest mip-map) if
					// the square wasn't visible for 120 vframes
					glDeleteTextures(1, &square->textureID);
					LoadSquareTexture(x, y, 3);
				}
				continue;
			}

			float dx = cam2->GetPos().x - (x * smfMap->bigSquareSize * SQUARE_SIZE);
			dx -= (SQUARE_SIZE << 6);
			dx = std::max(0.0f, float(math::fabs(dx) - (SQUARE_SIZE << 6)));

			const float hAvg =
				(heightMaxima[y * smfMap->numBigTexX + x] +
				 heightMinima[y * smfMap->numBigTexX + x]) / 2.0f;
			const float dy = std::max(cam2->GetPos().y - hAvg, 0.0f);
			const float dist = fastmath::apxsqrt(dx * dx + dy * dy + dz * dz);

			// we work under the following assumptions:
			//    the minimum mip level is the closest ceiling mip level that we can use
			//    based on distance, FOV and tile size; we can increase this mip level IF
			//    the stretch factor requires us to do so.
			//
			//    we will approximate tile size with a sphere 512 elmos in radius, which
			//    translates to a diameter of =~ sqrt2 * bigTexSize =~ 1400 pixels
			//
			//    half (vertical) FOV is 45 degs, for default and most other camera modes
			int wantedLevel = 0;
			float heightDiff =
				heightMaxima[y * smfMap->numBigTexX + x] -
				heightMinima[y * smfMap->numBigTexX + x];
			int screenPixels = smfMap->bigTexSize;

			if (dist > 0.0f) {
				if (heightDiff > float(smfMap->bigTexSize)) {
					// this means the heightmap chunk is taller than it is wide,
					// so we use the tallness metric instead for calculating its
					// on-screen size in pixels
					screenPixels = (heightDiff) * (vdiag * 0.5f) / dist;
				} else {
					screenPixels = smfMap->bigTexSize * (vdiag * 0.5f) / dist;
				}
			}

			if (screenPixels > 513)
				wantedLevel = 0;
			else if (screenPixels > 257)
				wantedLevel = 1;
			else if (screenPixels > 129)
				wantedLevel = 2;
			else
				wantedLevel = 3;

			// 16K is an approximation of the Sobel sum required to have a
			// heightmap that has double the texture area of a flat square
			if (stretchFactors[y * smfMap->numBigTexX + x] > 16000 && wantedLevel > 0)
				wantedLevel--;

			if (square->texLevel != wantedLevel) {
				glDeleteTextures(1, &square->textureID);
				LoadSquareTexture(x, y, wantedLevel);
			}
		}
	}
}



bool CSMFGroundTextures::SetSquareLuaTexture(int texSquareX, int texSquareY, int texID) {
	if (texSquareX < 0 || texSquareX >= smfMap->numBigTexX) { return false; }
	if (texSquareY < 0 || texSquareY >= smfMap->numBigTexY) { return false; }

	GroundSquare* square = &squares[texSquareY * smfMap->numBigTexX + texSquareX];

	if (texID != 0) {
		if (!square->luaTexture) {
			// only delete textures managed by us
			glDeleteTextures(1, &square->textureID);
		}

		square->textureID = texID;
		square->luaTexture = true;
	} else {
		if (square->luaTexture) {
			// default texture will be loaded by the next
			// DrawUpdate (when it comes into view again)
			square->textureID = 0;
			square->luaTexture = false;
		}
	}

	return (square->luaTexture);
}

bool CSMFGroundTextures::GetSquareLuaTexture(int texSquareX, int texSquareY, int texID, int texSizeX, int texSizeY, int texMipLevel) {
	if (texSquareX < 0 || texSquareX >= smfMap->numBigTexX) { return false; }
	if (texSquareY < 0 || texSquareY >= smfMap->numBigTexY) { return false; }
	if (texMipLevel < 0 || texMipLevel > 3) { return false; }

	// no point extracting sub-rectangles from compressed data
	if (texSizeX != (smfMap->bigTexSize >> texMipLevel)) { return false; }
	if (texSizeY != (smfMap->bigTexSize >> texMipLevel)) { return false; }

	static const GLenum ttarget = GL_TEXTURE_2D;

	const int mipSqSize = smfMap->bigTexSize >> texMipLevel;
	const int numSqBytes = (mipSqSize * mipSqSize) / 2;

	pbo.Bind();
	pbo.Resize(numSqBytes);
	ExtractSquareTiles(texSquareX, texSquareY, texMipLevel, (GLint*) pbo.MapBuffer());
	pbo.UnmapBuffer();

	glBindTexture(ttarget, texID);
	glCompressedTexImage2D(ttarget, 0, texformat, texSizeX, texSizeY, 0, numSqBytes, pbo.GetPtr());
	glBindTexture(ttarget, 0);

	pbo.Unbind();
	return true;
}



void CSMFGroundTextures::ExtractSquareTiles(
	const int texSquareX,
	const int texSquareY,
	const int mipLevel,
	GLint* tileBuf
) const {
	static const int TILE_MIP_OFFSET[] = {0, 512, 640, 672};
	static const int BLOCK_SIZE = 32;

	const int mipOffset = TILE_MIP_OFFSET[mipLevel];
	const int numBlocks = SQUARE_SIZE >> mipLevel;
	const int tileOffsetX = texSquareX * BLOCK_SIZE;
	const int tileOffsetY = texSquareY * BLOCK_SIZE;

	// extract all 32x32 sub-blocks (tiles) in the 128x128 square
	// (each 32x32 tile covers a (bigSquareSize = 32 * tileScale) x
	// (bigSquareSize = 32 * tileScale) heightmap chunk)
	for (int y1 = 0; y1 < BLOCK_SIZE; y1++) {
		for (int x1 = 0; x1 < BLOCK_SIZE; x1++) {
			const int tileX = tileOffsetX + x1;
			const int tileY = tileOffsetY + y1;
			const int tileIdx = tileMap[tileY * smfMap->tileMapSizeX + tileX];
			const GLint* tile = (GLint*) &tiles[tileIdx * SMALL_TILE_SIZE + mipOffset];

			const int doff = (x1 * numBlocks) + (y1 * numBlocks * numBlocks) * BLOCK_SIZE;

			for (int b = 0; b < numBlocks; b++) {
				const GLint* sbuf = &tile[b * numBlocks * 2];
				      GLint* dbuf = &tileBuf[(doff + b * numBlocks * BLOCK_SIZE) * 2];

				// at MIP level n: ((8 >> n) * 2 * 4) = (64 >> n) bytes for each <b>
				memcpy(dbuf, sbuf, numBlocks * 2 * sizeof(GLint));
			}
		}
	}
}

void CSMFGroundTextures::LoadSquareTexture(int x, int y, int level)
{
	static const GLenum ttarget = GL_TEXTURE_2D;

	const int mipSqSize = smfMap->bigTexSize >> level;
	const int numSqBytes = (mipSqSize * mipSqSize) / 2;

	GroundSquare* square = &squares[y * smfMap->numBigTexX + x];
	square->texLevel = level;

	pbo.Bind();
	pbo.Resize(numSqBytes);
	ExtractSquareTiles(x, y, level, (GLint*) pbo.MapBuffer());
	pbo.UnmapBuffer();

	glGenTextures(1, &square->textureID);
	glBindTexture(ttarget, square->textureID);
	glTexParameteri(ttarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(ttarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	if (GLEW_EXT_texture_edge_clamp) {
		glTexParameteri(ttarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(ttarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	if (smfMap->GetAnisotropy() != 0.0f)
		glTexParameterf(ttarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, smfMap->GetAnisotropy());

	if (level < 2) {
		glTexParameteri(ttarget, GL_TEXTURE_PRIORITY, 1);
	} else {
		glTexParameterf(ttarget, GL_TEXTURE_PRIORITY, 0.5f);
	}

	glCompressedTexImage2D(ttarget, 0, texformat, mipSqSize, mipSqSize, 0, numSqBytes, pbo.GetPtr());
	pbo.Unbind();
}

void CSMFGroundTextures::BindSquareTexture(int texSquareX, int texSquareY)
{
	assert(texSquareX >= 0);
	assert(texSquareY >= 0);
	assert(texSquareX < smfMap->numBigTexX);
	assert(texSquareY < smfMap->numBigTexY);

	GroundSquare* square = &squares[texSquareY * smfMap->numBigTexX + texSquareX];
	glBindTexture(GL_TEXTURE_2D, square->textureID);

	if (game->GetDrawMode() == CGame::gameNormalDraw) {
		square->lastBoundFrame = globalRendering->drawFrame;
	}
}
