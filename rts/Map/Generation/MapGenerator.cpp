/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MapGenerator.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Exceptions.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/Archives/VirtualArchive.h"
#include "Map/SMF/SMFFormat.h"
#include "Game/LoadScreen.h"
#include "Rendering/GL/myGL.h"

#include <boost/algorithm/string.hpp>
#include <fstream>

CMapGenerator::CMapGenerator(const CGameSetup* setup) : setup(setup)
{
}

CMapGenerator::~CMapGenerator()
{
}

void CMapGenerator::Generate()
{
	//loadscreen->SetLoadMessage("Generating map");

	//Create archive for map
	std::string mapArchivePath = setup->mapName + "." + virtualArchiveFactory->GetDefaultExtension();
	CVirtualArchive* archive = virtualArchiveFactory->AddArchive(setup->mapName);

	//Create arrays that can be filled by top class
	int2 gridSize = GetGridSize();
	const int dimensions = (gridSize.x + 1) * (gridSize.y + 1);
	heightMap.resize(dimensions);
	metalMap.resize(dimensions);

	//Generate map and fill archive files
	GenerateMap();
	GenerateSMF(archive);
	GenerateMapInfo(archive);
	GenerateSMT(archive);

	//Add archive to vfs
	archiveScanner->ScanArchive(mapArchivePath);

	//Write to disk for testing
	//archive->WriteToFile();
}

void CMapGenerator::AppendToBuffer(CVirtualFile* file, const void* data, int size)
{
	file->buffer.insert(file->buffer.end(), (boost::uint8_t*)data, (boost::uint8_t*)data + size);
}

void CMapGenerator::SetToBuffer(CVirtualFile* file, const void* data, int size, int position)
{
	std::copy((boost::uint8_t*)data, (boost::uint8_t*)data + size, file->buffer.begin() + position);
}

void CMapGenerator::GenerateSMF(CVirtualArchive* archive)
{
	CVirtualFile* fileSMF = archive->AddFile("maps/generated.smf");

	//FileBuffer b;

	SMFHeader smfHeader;
	MapTileHeader smfTile;
	MapFeatureHeader smfFeature;

	//--- Make SMFHeader ---
	strcpy(smfHeader.magic, "spring map file");
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

	const int numSmallTiles = 1; //2087; //32 * 32 * (mapSize.x  / 2) * (mapSize.y / 2);
	const char smtFileName[] = "generated.smt";

	//--- Extra headers ---
	ExtraHeader vegHeader;
	vegHeader.type = MEH_Vegetation;
	vegHeader.size = sizeof(int);

	smfHeader.numExtraHeaders =  1;

	//Make buffers for each map
	int heightmapDimensions = (smfHeader.mapx + 1) * (smfHeader.mapy + 1);
	int typemapDimensions = (smfHeader.mapx / 2) * (smfHeader.mapy / 2);
	int metalmapDimensions = (smfHeader.mapx / 2) * (smfHeader.mapy / 2);
	int tilemapDimensions =  (smfHeader.mapx * smfHeader.mapy) / 16;
	int vegmapDimensions = (smfHeader.mapx / 4) * (smfHeader.mapy / 4);

	int heightmapSize = heightmapDimensions * sizeof(short);
	int typemapSize = typemapDimensions * sizeof(unsigned char);
	int metalmapSize = metalmapDimensions * sizeof(unsigned char);
	int tilemapSize = tilemapDimensions * sizeof(int);
	int tilemapTotalSize = sizeof(MapTileHeader) + sizeof(numSmallTiles) + sizeof(smtFileName) + tilemapSize;
	int vegmapSize = vegmapDimensions * sizeof(unsigned char);

	short* heightmapPtr = new short[heightmapDimensions];
	unsigned char* typemapPtr = new unsigned char[typemapDimensions];
	unsigned char* metalmapPtr = new unsigned char[metalmapDimensions];
	int* tilemapPtr = new int[tilemapDimensions];
	unsigned char* vegmapPtr = new unsigned char[vegmapDimensions];

	//--- Set offsets, increment each member with the previous one ---
	int vegmapOffset = sizeof(smfHeader) + sizeof(vegHeader) + sizeof(int);
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
	memset(vegmapPtr, 0, vegmapSize);

	float heightMin = smfHeader.minHeight;
	float heightMax = smfHeader.maxHeight;
	float heightMul = (float)0xFFFF / (smfHeader.maxHeight - smfHeader.minHeight);
	for(int x = 0; x < heightmapDimensions; x++)
	{
		float h = heightMap[x];
		if(h < heightMin) h = heightMin;
		if(h > heightMax) h = heightMax;
		h -= heightMin;
		h *= heightMul;
		heightmapPtr[x] = (short)h;
	}

	memset(typemapPtr, 0, typemapSize);

	/*for(u32 x = 0; x < smfHeader.mapx; x++)
	{
		for(u32 y = 0; y < smfHeader.mapy; y++)
		{
			u32 index =
			tilemapPtr[]
		}
	}*/

	memset(tilemapPtr, 0, tilemapSize);

	memset(metalmapPtr, 0, metalmapSize);

	//--- Write to final buffer ---
	//std::vector<boost::uint8_t>& smb = fileSMF->buffer;
	AppendToBuffer(fileSMF, smfHeader);

	AppendToBuffer(fileSMF, vegHeader);
	AppendToBuffer(fileSMF, vegmapOffset);
	AppendToBuffer(fileSMF, vegmapPtr, vegmapSize);

	AppendToBuffer(fileSMF, heightmapPtr, heightmapSize);
	AppendToBuffer(fileSMF, typemapPtr, typemapSize);

	AppendToBuffer(fileSMF, smfTile);
	AppendToBuffer(fileSMF, numSmallTiles);
	AppendToBuffer(fileSMF, smtFileName, sizeof(smtFileName));
	AppendToBuffer(fileSMF, tilemapPtr, tilemapSize);

	AppendToBuffer(fileSMF, metalmapPtr, metalmapSize);
	AppendToBuffer(fileSMF, smfFeature);

	delete[] heightmapPtr;
	delete[] typemapPtr;
	delete[] metalmapPtr;
	delete[] tilemapPtr;
	delete[] vegmapPtr;
}

void CMapGenerator::GenerateMapInfo(CVirtualArchive* archive)
{

	CVirtualFile* fileMapInfo = archive->AddFile("mapinfo.lua");

	//Open template mapinfo.lua
	const std::string luaTemplate = "mapgenerator/mapinfo_template.lua";
	CFileHandler fh(luaTemplate, SPRING_VFS_PWD_ALL);
	if(!fh.FileExists())
	{
		throw content_error("Error generating map: " + luaTemplate + " not found");
	}

	std::string luaInfo;
	fh.LoadStringData(luaInfo);

	//Make info to put in mapinfo
	std::stringstream ss;
	std::string startPosString;
	const std::vector<int2>& startPositions = GetStartPositions();
	for(size_t x = 0; x < startPositions.size(); x++)
	{
		ss << "[" << x << "] = {startPos = {x = " << startPositions[x].x << ", z = " << startPositions[x].y << "}},";
	}
	startPosString = ss.str();

	//Replace tags in mapinfo.lua
	boost::replace_first(luaInfo, "${NAME}", setup->mapName);
	boost::replace_first(luaInfo, "${DESCRIPTION}", GetMapDescription());
	boost::replace_first(luaInfo, "${START_POSITIONS}", startPosString);

	//Copy to filebuffer
	fileMapInfo->buffer.assign(luaInfo.begin(), luaInfo.end());
}

void CMapGenerator::GenerateSMT(CVirtualArchive* archive)
{
	CVirtualFile* fileSMT = archive->AddFile("maps/generated.smt");

	const int tileSize = 32;

	//--- Make TileFileHeader ---
	TileFileHeader smtHeader;
	strcpy(smtHeader.magic, "spring tilefile");
	smtHeader.version = 1;
	smtHeader.numTiles = 1; //32 * 32 * (generator->GetMapSize().x * 32) * (generator->GetMapSize().y * 32);
	smtHeader.tileSize = tileSize;
	smtHeader.compressionType = 1;

	const int bpp = 3;
	int tilePos = 0;
	unsigned char tileData[tileSize * tileSize * bpp];
	for(int x = 0; x < tileSize; x++)
	{
		for(int y = 0; y < tileSize; y++)
		{
			tileData[tilePos] = 0;
			tileData[tilePos + 1] = 0xFF;
			tileData[tilePos + 2] = 0;
			tilePos += bpp;
		}
	}
	glClearErrors();
	GLuint tileTex;
	glGenTextures(1, &tileTex);
	glBindTexture(GL_TEXTURE_2D, tileTex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, tileSize, tileSize, 0, GL_RGB, GL_UNSIGNED_BYTE, tileData);
	glGenerateMipmapEXT(GL_TEXTURE_2D);
	char tileDataDXT[SMALL_TILE_SIZE];

	int dxtImageOffset = 0;
	int dxtImageSize = 512;
	for(int x = 0; x < 4; x++)
	{
		glGetCompressedTexImage(GL_TEXTURE_2D, x, tileDataDXT + dxtImageOffset);
		dxtImageOffset += dxtImageSize;
		dxtImageSize /= 4;
	}

	glDeleteTextures(1, &tileTex);

	GLenum errorcode = glGetError();
	if(errorcode != GL_NO_ERROR)
	{
		throw content_error("Error generating map - texture generation not supported");
	}

	size_t totalSize = sizeof(TileFileHeader);
	fileSMT->buffer.resize(totalSize);

	int writePosition = 0;
	memcpy(&(fileSMT->buffer[writePosition]), &smtHeader, sizeof(smtHeader));
	writePosition += sizeof(smtHeader);

	fileSMT->buffer.resize(fileSMT->buffer.size() + smtHeader.numTiles * SMALL_TILE_SIZE);
	for(int x = 0; x < smtHeader.numTiles; x++)
	{
		memcpy(&(fileSMT->buffer[writePosition]), tileDataDXT, SMALL_TILE_SIZE);
		writePosition += SMALL_TILE_SIZE;
	}
}
