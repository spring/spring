/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <cmath>
#include <cstdlib>
#include <cstdio>

#include "SMFGroundTextures.h"
#include "SMFFormat.h"
#include "SMFReadMap.h"
#include "Rendering/GL/PBO.h"
#include "Rendering/GlobalRendering.h"
#include "Map/MapInfo.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/LoadScreen.h"
#include "System/Exceptions.h"
#include "System/FastMath.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Platform/Watchdog.h"
#include "System/Threading/ThreadPool.h" // for_mt

using std::sprintf;

#define LOG_SECTION_SMF_GROUND_TEXTURES "CSMFGroundTextures"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_SMF_GROUND_TEXTURES)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_SMF_GROUND_TEXTURES



std::vector<CSMFGroundTextures::GroundSquare> CSMFGroundTextures::squares;

std::vector<int> CSMFGroundTextures::tileMap;
std::vector<char> CSMFGroundTextures::tiles;

std::vector<float> CSMFGroundTextures::heightMaxima;
std::vector<float> CSMFGroundTextures::heightMinima;
std::vector<float> CSMFGroundTextures::stretchFactors;



CSMFGroundTextures::GroundSquare::~GroundSquare()
{
	glDeleteTextures(1, &textureIDs[RAW_TEX_IDX]);

	textureIDs[RAW_TEX_IDX] = 0;
	textureIDs[LUA_TEX_IDX] = 0;
}



CSMFGroundTextures::CSMFGroundTextures(CSMFReadMap* rm): smfMap(rm)
{
	LoadTiles(smfMap->GetMapFile());
	LoadSquareTextures(0, 3); // preload all levels
	ConvolveHeightMap(mapDims.mapx, 1);
}

CSMFGroundTextures::~CSMFGroundTextures()
{
	pbo.Release();
	glDeleteTextures(1, &tileArrayTex);
}

void CSMFGroundTextures::LoadTiles(CSMFMapFile& file)
{
	loadscreen->SetLoadMessage("Loading Map Tiles");

	CFileHandler* ifs = file.GetFileHandler();
	const SMFHeader& header = file.GetHeader();

	char tmp[512] = {0};

	if ((mapDims.mapx != header.mapx) || (mapDims.mapy != header.mapy)) {
		snprintf(tmp, sizeof(tmp), "[SMFGroundTextures::%s] header.{mapx=%d,mapy=%d} != mapDims.{mapx=%d,mapy=%d}", __func__, header.mapx, header.mapy, mapDims.mapx, mapDims.mapy);
		throw content_error(tmp);
	}

	ifs->Seek(header.tilesPtr);

	MapTileHeader tileHeader;
	CSMFMapFile::ReadMapTileHeader(tileHeader, *ifs);

	if (smfMap->tileCount <= 0) {
		snprintf(tmp, sizeof(tmp), "[SMFGroundTextures::%s] smfMap->tileCount=%d <= 0", __func__, smfMap->tileCount);
		throw content_error(tmp);
	}

	tileMap.clear();
	tileMap.resize(smfMap->tileCount);
	tiles.clear();
	tiles.resize(tileHeader.numTiles * SMALL_TILE_SIZE);
	squares.clear();
	squares.resize(smfMap->numBigTexX * smfMap->numBigTexY);

	bool smtHeaderOverride = false;

	const std::string& smfDir = FileSystem::GetDirectory(gameSetup->MapFileName());
	const CMapInfo::smf_t& smf = mapInfo->smf;

	if (!smf.smtFileNames.empty()) {
		if (!(smtHeaderOverride = (smf.smtFileNames.size() == tileHeader.numTileFiles))) {
			LOG_L(L_WARNING, "[SMFGroundTextures::%s] smtFileNames.size()=" _STPF_ " != tileHeader.numTileFiles=%d", __func__, smf.smtFileNames.size(), tileHeader.numTileFiles);
		}
	}

	for (int a = 0, curTile = 0; a < tileHeader.numTileFiles; ++a) {
		int numSmallTiles = 0;
		char fileNameBuffer[256] = {0};

		ifs->Read(&numSmallTiles, sizeof(int));
		ifs->ReadString(&fileNameBuffer[0], sizeof(char) * (sizeof(fileNameBuffer) - 1));
		swabDWordInPlace(numSmallTiles);

		std::string smtFileName = fileNameBuffer;
		std::string smtFilePath = (!smtHeaderOverride)?
			(smfDir + smtFileName):
			(smfDir + smf.smtFileNames[a]);

		CFileHandler tileFile(smtFilePath);

		// try absolute path
		if (!tileFile.FileExists())
			tileFile.Open(smtFilePath = (!smtHeaderOverride) ? smtFileName : smf.smtFileNames[a]);

		if (!tileFile.FileExists()) {
			LOG_L(L_WARNING,
				"[SMFGroundTextures::%s] could not find .smt tile-file %d (\"%s\"; ALL %d SMALL TILES WILL BE MADE RED)",
				__func__, a, smtFilePath.c_str(), numSmallTiles
			);

			memset(&tiles[curTile * SMALL_TILE_SIZE], 0xaa, numSmallTiles * SMALL_TILE_SIZE);
			curTile += numSmallTiles;
			continue;
		}

		TileFileHeader tfh;
		CSMFMapFile::ReadMapTileFileHeader(tfh, tileFile);

		if (strcmp(tfh.magic, "spring tilefile") != 0 || tfh.version != 1 || tfh.tileSize != 32 || tfh.compressionType != 1) {
			snprintf(
				tmp, sizeof(tmp),
				"[SMFGroundTextures::%s] tile-file %d (path=\"%s\" magic=\"%s\" version=%d tileSize=%d comprType=%d) does not match .smt format",
				__func__, a, smtFilePath.c_str(), tfh.magic, tfh.version, tfh.tileSize, tfh.compressionType
			);
			throw content_error(tmp);
		}

		for (int b = 0; b < numSmallTiles; ++b) {
			tileFile.Read(&tiles[(curTile++) * SMALL_TILE_SIZE], SMALL_TILE_SIZE);
		}
	}

	ifs->Read(&tileMap[0], smfMap->tileCount * sizeof(int));

	for (int i = 0; i < smfMap->tileCount; i++) {
		swabDWordInPlace(tileMap[i]);
	}

	// ETCx is nicer than S3TC (when in hardware) although it lacks alpha support,
	// but recompressing the DXT tiles has little benefit since FOSS drivers also
	// implement S3TC now
	// (recompression quality would have to be low anyway for performance reasons)
	tileTexFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
}

void CSMFGroundTextures::LoadSquareTextures(const int minLevel, const int maxLevel)
{
	loadscreen->SetLoadMessage("Loading Square Textures");


	glGenTextures(1, &tileArrayTex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tileArrayTex);

	const int bts = smfMap->bigTexSize;
	const int ntx = smfMap->numBigTexX;
	const int nty = smfMap->numBigTexY;

	#if 0
	glTexImage3D(
		GL_TEXTURE_2D_ARRAY,
		0,
		tileTexFormat,
		bts,
		bts,
		ntx * nty,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		nullptr
	);
	#else
	glTexStorage3D(
		GL_TEXTURE_2D_ARRAY,
		1 + 3,
		tileTexFormat,
		bts, bts,
		ntx * nty
	);

	{
		#ifndef HEADLESS
		GLint val = 0;
		glGetTexParameteriv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_IMMUTABLE_FORMAT, &val);
		assert(val == 1);
		#endif
	}

	#endif

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR              );
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (smfMap->GetTexAnisotropyLevel(false) != 0.0f)
		glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, smfMap->GetTexAnisotropyLevel(false));

	for (int i = minLevel; i <= maxLevel; i++) {
		for (int y = 0; y < nty; ++y) {
			for (int x = 0; x < ntx; ++x) {
				LoadSquareTexture(x, y, i);
			}
		}
	}

	// prefer baked MIP's, seven levels are overkill
	// glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void CSMFGroundTextures::ConvolveHeightMap(const int mapWidth, const int mipLevel)
{
	ScopedOnceTimer timer("CSMFGroundTextures::ConvolveHeightMap");

	const float* hdata = readMap->GetMIPHeightMapSynced(mipLevel);
	const int mx = mapWidth >> mipLevel;

	const int nbx = smfMap->numBigTexX;
	const int nby = smfMap->numBigTexY;
	const int nb = nbx * nby;

	// 64 is the heightmap square-size at MIP level 1 (bigSquareSize >> 1)
	constexpr int mipSquareSize = 64;

	heightMaxima.clear();
	heightMaxima.resize(nb, readMap->GetCurrMinHeight());
	heightMinima.clear();
	heightMinima.resize(nb, readMap->GetCurrMaxHeight());
	stretchFactors.clear();
	stretchFactors.resize(nb, 0.0f);

	for (int y = 0; y < nby; ++y) {
		for (int x = 0; x < nbx; ++x) {

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


inline bool CSMFGroundTextures::TexSquareInView(int btx, int bty) const
{
	const CCamera* cam = CCameraHandler::GetActiveCamera();
	const float* hm = readMap->GetCornerHeightMapUnsynced();

	static const float bigTexSquareRadius = fastmath::apxsqrt(
		smfMap->bigTexSize * smfMap->bigTexSize +
		smfMap->bigTexSize * smfMap->bigTexSize
	);

	const int x = btx * smfMap->bigTexSize + (smfMap->bigTexSize >> 1);
	const int y = bty * smfMap->bigTexSize + (smfMap->bigTexSize >> 1);
	const int idx = (y >> 3) * smfMap->heightMapSizeX + (x >> 3);

	return (cam->InView({x * 1.0f, hm[idx], y * 1.0f}, bigTexSquareRadius));
}

void CSMFGroundTextures::DrawUpdate()
{
	const CCamera* cam = CCameraHandler::GetActiveCamera();

	const float3& camPos = cam->GetPos();

	// screen-diagonal number of pixels
	const float vsxSq = globalRendering->viewSizeX * globalRendering->viewSizeX;
	const float vsySq = globalRendering->viewSizeY * globalRendering->viewSizeY;
	const float vdiag = fastmath::apxsqrt(vsxSq + vsySq);

	for (int y = 0; y < smfMap->numBigTexY; ++y) {
		float dz = camPos.z - (y * smfMap->bigSquareSize * SQUARE_SIZE);
		dz -= (SQUARE_SIZE << 6);
		dz = std::max(0.0f, float(math::fabs(dz) - (SQUARE_SIZE << 6)));

		for (int x = 0; x < smfMap->numBigTexX; ++x) {
			GroundSquare* square = &squares[y * smfMap->numBigTexX + x];

			// no deletion or mip-level selection
			if (square->HasLuaTexture())
				continue;

			if (!TexSquareInView(x, y)) {
				if ((square->GetMipLevel() < 3) && ((globalRendering->drawFrame - square->GetDrawFrame()) > 120)) {
					// `unload` texture (load lowest mip-map) if
					// the square wasn't visible for 120 vframes
					square->SetMipLevel(3);
				}

				continue;
			}

			float dx = camPos.x - (x * smfMap->bigSquareSize * SQUARE_SIZE);
			dx -= (SQUARE_SIZE << 6);
			dx = std::max(0.0f, float(math::fabs(dx) - (SQUARE_SIZE << 6)));

			const float avgHeight = (heightMaxima[y * smfMap->numBigTexX + x] + heightMinima[y * smfMap->numBigTexX + x]) * 0.5f;
			const float difHeight =  heightMaxima[y * smfMap->numBigTexX + x] - heightMinima[y * smfMap->numBigTexX + x];

			const float dy = std::max(camPos.y - avgHeight, 0.0f);
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
			int screenPixels = smfMap->bigTexSize;
			int wantedLevel = 0;

			if (dist > 0.0f) {
				if (difHeight > float(smfMap->bigTexSize)) {
					// this means the heightmap chunk is taller than it is wide,
					// so we use the tallness metric instead for calculating its
					// on-screen size in pixels
					screenPixels = (difHeight) * (vdiag * 0.5f) / dist;
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

			square->SetMipLevel(wantedLevel);
		}
	}
}



bool CSMFGroundTextures::SetSquareLuaTexture(int texSquareX, int texSquareY, int texID) {
	#if 0
	if (texSquareX < 0 || texSquareX >= smfMap->numBigTexX)
		return false;
	if (texSquareY < 0 || texSquareY >= smfMap->numBigTexY)
		return false;

	GroundSquare* square = &squares[texSquareY * smfMap->numBigTexX + texSquareX];

	if (texID != 0) {
		// free up some memory while the Lua texture is around
		glDeleteTextures(1, square->GetTextureIDPtr());
		square->SetRawTexture(0);
	}

	square->SetLuaTexture(texID);
	return (square->HasLuaTexture());
	#endif
	return false;
}

bool CSMFGroundTextures::GetSquareLuaTexture(int texSquareX, int texSquareY, int texID, int texSizeX, int texSizeY, int texMipLevel) {
	#if 0
	if (texSquareX < 0 || texSquareX >= smfMap->numBigTexX)
		return false;
	if (texSquareY < 0 || texSquareY >= smfMap->numBigTexY)
		return false;
	if (texMipLevel < 0 || texMipLevel > 3)
		return false;

	// no point extracting sub-rectangles from compressed data
	if (texSizeX != (smfMap->bigTexSize >> texMipLevel))
		return false;
	if (texSizeY != (smfMap->bigTexSize >> texMipLevel))
		return false;

	constexpr GLenum ttarget = GL_TEXTURE_2D;
	constexpr GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT;

	const int mipSqSize = smfMap->bigTexSize >> texMipLevel;
	const int numSqBytes = (mipSqSize * mipSqSize) / 2;

	pbo.Bind();
	pbo.New(numSqBytes);
	ExtractSquareTiles(texSquareX, texSquareY, texMipLevel, (GLint*) pbo.MapBuffer(0, pbo.bufSize, access | pbo.mapUnsyncedBit));
	pbo.UnmapBuffer();

	glBindTexture(ttarget, texID);
	glCompressedTexImage2D(ttarget, 0, tileTexFormat, texSizeX, texSizeY, 0, numSqBytes, pbo.GetPtr());
	glBindTexture(ttarget, 0);

	pbo.Invalidate();
	pbo.Unbind();
	#endif
	return false;
}



void CSMFGroundTextures::ExtractSquareTiles(
	const int texSquareX,
	const int texSquareY,
	const int mipLevel,
	GLint* tileBuf
) const {
	if (tileBuf == nullptr)
		return;

	constexpr int TILE_MIP_OFFSET[] = {0, 512, 512+128, 512+128+32};
	constexpr int BLOCK_SIZE = 32;

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
	constexpr GLenum ttarget = GL_TEXTURE_2D_ARRAY;
	constexpr GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT;

	const int mipSqSize = smfMap->bigTexSize >> level;
	const int numSqBytes = (mipSqSize * mipSqSize) / 2;

	GroundSquare* square = &squares[y * smfMap->numBigTexX + x];
	square->SetMipLevel(level);
	assert(!square->HasLuaTexture());


	pbo.Bind();
	pbo.New(numSqBytes);
	ExtractSquareTiles(x, y, level, (GLint*) pbo.MapBuffer(0, pbo.bufSize, access | pbo.mapUnsyncedBit));
	pbo.UnmapBuffer();

	glCompressedTexSubImage3D(
		ttarget,
		level,

		0, 0, y * smfMap->numBigTexX + x, // xoffset, yoffset, zoffset=slice
		mipSqSize, mipSqSize, 1, // width, height, depth

		tileTexFormat,
		numSqBytes,
		pbo.GetPtr()
	);

	pbo.Invalidate();
	pbo.Unbind();
}

void CSMFGroundTextures::BindSquareTextureArray() const { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D_ARRAY, tileArrayTex); }
void CSMFGroundTextures::UnBindSquareTextureArray() const { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D_ARRAY, 0); }

void CSMFGroundTextures::DrawUpdateSquare(int texSquareX, int texSquareY)
{
	assert(texSquareX >= 0);
	assert(texSquareY >= 0);
	assert(texSquareX < smfMap->numBigTexX);
	assert(texSquareY < smfMap->numBigTexY);

	if (game->GetDrawMode() != Game::NormalDraw)
		return;

	squares[texSquareY * smfMap->numBigTexX + texSquareX].SetDrawFrame(globalRendering->drawFrame);
}
