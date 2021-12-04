/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MapGenerator.h"
#include "Map/SMF/SMFFormat.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "System/FileSystem/Archives/VirtualArchive.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Exceptions.h"
#include "System/StringUtil.h"
#include "System/FileSystem/VFSHandler.h"

#include <cstring> // strcpy,memset
#include <sstream>

void CMapGenerator::Generate()
{
	// create archive for map
	CVirtualArchive* archive = virtualArchiveFactory->AddArchive(setup->mapName);

	// create arrays that can be filled by top class
	const int2 gridSize = GetGridSize();

	heightMap.resize((gridSize.x + 1) * (gridSize.y + 1));
	metalMap.resize((gridSize.x + 1) * (gridSize.y + 1));

	// generate map and fill archive files
	GenerateMap();
	GenerateSMF(archive->GetFilePtr(archive->AddFile("maps/generated.smf")));
	GenerateMapInfo(archive->GetFilePtr(archive->AddFile("mapinfo.lua")));
	GenerateSMT(archive->GetFilePtr(archive->AddFile("maps/generated.smt")));

	// add archive to VFS
	archiveScanner->ScanArchive(setup->mapName + "." + virtualArchiveFactory->GetDefaultExtension());

	// write to disk for testing
	// archive->WriteToFile();
}

void CMapGenerator::AppendToBuffer(CVirtualFile* file, const void* data, int size)
{
	file->buffer.insert(file->buffer.end(), (std::uint8_t*)data, (std::uint8_t*)data + size);
}

void CMapGenerator::SetToBuffer(CVirtualFile* file, const void* data, int size, int position)
{
	std::copy((std::uint8_t*)data, (std::uint8_t*)data + size, file->buffer.begin() + position);
}

void CMapGenerator::GenerateSMF(CVirtualFile* fileSMF)
{
	SMFHeader smfHeader;
	MapTileHeader smfTile;
	MapFeatureHeader smfFeature;

	//--- Make SMFHeader ---
	std::strcpy(smfHeader.magic, "spring map file");
	smfHeader.version = 1;
	smfHeader.mapid = 0x524d4746 ^ (int)setup->mapSeed;

	//Set settings
	smfHeader.mapx = GetGridSize().x;
	smfHeader.mapy = GetGridSize().y;
	smfHeader.squareSize = 8;
	smfHeader.texelPerSquare = 8;
	smfHeader.tilesize = 32;
	smfHeader.minHeight = -100;
	smfHeader.maxHeight = 0x1000;

	constexpr int32_t numSmallTiles = 1; //2087; //32 * 32 * (mapSize.x  / 2) * (mapSize.y / 2);
	constexpr char smtFileName[] = "generated.smt";

	//--- Extra headers ---
	ExtraHeader vegHeader;
	vegHeader.type = MEH_Vegetation;
	vegHeader.size = sizeof(int);

	smfHeader.numExtraHeaders =  1;

	//Make buffers for each map
	const int32_t heightmapDimensions = (smfHeader.mapx + 1) * (smfHeader.mapy + 1);
	const int32_t typemapDimensions = (smfHeader.mapx / 2) * (smfHeader.mapy / 2);
	const int32_t metalmapDimensions = (smfHeader.mapx / 2) * (smfHeader.mapy / 2);
	const int32_t tilemapDimensions =  (smfHeader.mapx * smfHeader.mapy) / 16;
	const int32_t vegmapDimensions = (smfHeader.mapx / 4) * (smfHeader.mapy / 4);

	const int32_t heightmapSize = heightmapDimensions * sizeof(int16_t);
	const int32_t typemapSize = typemapDimensions * sizeof(uint8_t);
	const int32_t metalmapSize = metalmapDimensions * sizeof(uint8_t);
	const int32_t tilemapSize = tilemapDimensions * sizeof(int32_t);
	const int32_t tilemapTotalSize = sizeof(MapTileHeader) + sizeof(numSmallTiles) + sizeof(smtFileName) + tilemapSize;
	const int32_t vegmapSize = vegmapDimensions * sizeof(uint8_t);

	constexpr int32_t vegmapOffset = sizeof(smfHeader) + sizeof(vegHeader) + sizeof(int32_t);

	std::vector<int16_t> heightmapPtr(heightmapDimensions);
	std::vector<uint8_t> typemapPtr(typemapDimensions);
	std::vector<uint8_t> metalmapPtr(metalmapDimensions);
	std::vector<int32_t> tilemapPtr(tilemapDimensions);
	std::vector<uint8_t> vegmapPtr(vegmapDimensions);

	//--- Set offsets, increment each member with the previous one ---
	smfHeader.heightmapPtr = vegmapOffset + vegmapSize;
	smfHeader.typeMapPtr = smfHeader.heightmapPtr + heightmapSize;
	smfHeader.tilesPtr = smfHeader.typeMapPtr + typemapSize;
	smfHeader.minimapPtr = 0; //smfHeader.tilesPtr + sizeof(MapTileHeader);
	smfHeader.metalmapPtr = smfHeader.tilesPtr + tilemapTotalSize;  //smfHeader.minimapPtr + minimapSize;
	smfHeader.featurePtr = smfHeader.metalmapPtr + metalmapSize;

	//--- Make MapTileHeader ---
	smfTile.numTileFiles = 1;
	smfTile.numTiles = numSmallTiles;

	//--- Make MapFeatureHeader ---
	smfFeature.numFeatures = 0;
	smfFeature.numFeatureType = 0;

	//--- Update Ptrs and write to buffer ---
	std::memset(vegmapPtr.data(), 0, vegmapSize);

	const float heightMin = smfHeader.minHeight;
	const float heightMax = smfHeader.maxHeight;
	const float heightMul = 65535.0f / (smfHeader.maxHeight - smfHeader.minHeight);

	for (int x = 0; x < heightmapDimensions; x++) {
		heightmapPtr[x] = int16_t(Clamp(heightMap[x], heightMin, heightMax) - heightMin) * heightMul;
	}

	std::memset(typemapPtr.data(), 0, typemapSize);

	/*for (u32 x = 0; x < smfHeader.mapx; x++) {
		for (u32 y = 0; y < smfHeader.mapy; y++) {
			u32 index = tilemapPtr[]
		}
	}*/

	std::memset(tilemapPtr.data(), 0, tilemapSize);
	std::memset(metalmapPtr.data(), 0, metalmapSize);

	//--- Write to final buffer ---
	AppendToBuffer(fileSMF, smfHeader);

	AppendToBuffer(fileSMF, vegHeader);
	AppendToBuffer(fileSMF, vegmapOffset);
	AppendToBuffer(fileSMF, vegmapPtr.data(), vegmapSize);

	AppendToBuffer(fileSMF, heightmapPtr.data(), heightmapSize);
	AppendToBuffer(fileSMF, typemapPtr.data(), typemapSize);

	AppendToBuffer(fileSMF, smfTile);
	AppendToBuffer(fileSMF, numSmallTiles);
	AppendToBuffer(fileSMF, smtFileName, sizeof(smtFileName));
	AppendToBuffer(fileSMF, tilemapPtr.data(), tilemapSize);

	AppendToBuffer(fileSMF, metalmapPtr.data(), metalmapSize);
	AppendToBuffer(fileSMF, smfFeature);
}

void CMapGenerator::GenerateMapInfo(CVirtualFile* fileMapInfo)
{
	//Open template mapinfo.lua
	const std::string luaTemplate = "mapgenerator/mapinfo_template.lua";
	CFileHandler fh(luaTemplate, SPRING_VFS_PWD_ALL);
	if (!fh.FileExists())
		throw content_error("Error generating map: " + luaTemplate + " not found");

	std::string luaInfo;
	fh.LoadStringData(luaInfo);

	//Make info to put in mapinfo
	std::stringstream ss;
	std::string startPosString;
	const std::vector<int2>& startPositions = GetStartPositions();
	for (size_t x = 0; x < startPositions.size(); x++) {
		ss << "[" << x << "] = {startPos = {x = " << startPositions[x].x << ", z = " << startPositions[x].y << "}},";
	}
	startPosString = ss.str();

	//Replace tags in mapinfo.lua
	luaInfo = StringReplace(luaInfo, "${NAME}", setup->mapName);
	luaInfo = StringReplace(luaInfo, "${DESCRIPTION}", GetMapDescription());
	luaInfo = StringReplace(luaInfo, "${START_POSITIONS}", startPosString);

	//Copy to filebuffer
	fileMapInfo->buffer.assign(luaInfo.begin(), luaInfo.end());
}

void CMapGenerator::GenerateSMT(CVirtualFile* fileSMT)
{
	constexpr int32_t tileSize = 32;
	constexpr int32_t tileBPP = 3;

	//--- Make TileFileHeader ---
	TileFileHeader smtHeader;
	std::strcpy(smtHeader.magic, "spring tilefile");
	smtHeader.version = 1;
	smtHeader.numTiles = 1; //32 * 32 * (generator->GetMapSize().x * 32) * (generator->GetMapSize().y * 32);
	smtHeader.tileSize = tileSize;
	smtHeader.compressionType = 1;

	int32_t tilePos = 0;
	uint8_t tileData[tileSize * tileSize * tileBPP];
	int8_t tileDataDXT[SMALL_TILE_SIZE];

	for (int32_t x = 0; x < tileSize; x++) {
		for (int32_t y = 0; y < tileSize; y++) {
			tileData[tilePos + 0] = 0;
			tileData[tilePos + 1] = 0xFF;
			tileData[tilePos + 2] = 0;
			tilePos += tileBPP;
		}
	}

	glClearErrors("MapGen", __func__, globalRendering->glDebugErrors);
	GLuint tileTex;
	glGenTextures(1, &tileTex);
	glBindTexture(GL_TEXTURE_2D, tileTex);

	// HL GLEW does not know GL_COMPRESSED_RGB_S3TC_DXT1
	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, tileSize, tileSize, 0, GL_RGB, GL_UNSIGNED_BYTE, tileData);
	glGenerateMipmap(GL_TEXTURE_2D);

	int32_t dxtImageOffset =   0;
	int32_t dxtImageSize   = 512;
	int32_t writePosition  =   0;

	for (int32_t x = 0; x < 4; x++) {
		glGetCompressedTexImage(GL_TEXTURE_2D, x, tileDataDXT + dxtImageOffset);

		dxtImageOffset += dxtImageSize;
		dxtImageSize /= 4;
	}

	glDeleteTextures(1, &tileTex);

	if (glGetError() != GL_NO_ERROR)
		throw content_error("Error generating map - texture generation not supported");

	fileSMT->buffer.resize(sizeof(TileFileHeader));

	std::memcpy(&fileSMT->buffer[writePosition], &smtHeader, sizeof(smtHeader));
	writePosition += sizeof(smtHeader);

	fileSMT->buffer.resize(fileSMT->buffer.size() + smtHeader.numTiles * SMALL_TILE_SIZE);

	for (int32_t x = 0; x < smtHeader.numTiles; x++) {
		std::memcpy(&fileSMT->buffer[writePosition], tileDataDXT, SMALL_TILE_SIZE);
		writePosition += SMALL_TILE_SIZE;
	}
}
