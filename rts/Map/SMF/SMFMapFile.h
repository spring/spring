/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SMF_MAP_FILE_H
#define _SMF_MAP_FILE_H

#include "System/FileSystem/FileHandler.h"
#include "SMFFormat.h"

#include <string>
#include <vector>

struct MapFeatureInfo;
struct MapBitmapInfo;

class CSMFMapFile
{
public:
	CSMFMapFile(                              ): ifs("", "") {                    } // defer Open
	CSMFMapFile(const std::string& mapFileName): ifs("", "") { Open(mapFileName); } // unitsync
	~CSMFMapFile() { Close(); }

	void Open(const std::string& mapFileName);
	void Close();

	void ReadMinimap(void* data);
	/// @return mip size
	int ReadMinimap(std::vector<std::uint8_t>& data, unsigned miplevel);
	void ReadHeightmap(unsigned short* heightmap);
	void ReadHeightmap(float* sHeightMap, float* uHeightMap, float base, float mod);
	void ReadFeatureInfo();
	void ReadFeatureInfo(MapFeatureInfo* f);
	void GetInfoMapSize(const char* name, MapBitmapInfo*) const;
	bool ReadInfoMap(const char* name, void* data);

	int GetNumFeatures()     const { return featureHeader.numFeatures; }
	int GetNumFeatureTypes() const { return featureHeader.numFeatureType; }

	const char* GetFeatureTypeName(int typeID) const;

	const SMFHeader& GetHeader() const { return header; }

	/**
	 * @deprecated do not use, just here for backward compatibility
	 *   with SMFGroundTextures.cpp
	 */
	CFileHandler* GetFileHandler() { return &ifs; }

	static void ReadMapTileHeader(MapTileHeader& head, CFileHandler& file);
	static void ReadMapTileFileHeader(TileFileHeader& head, CFileHandler& file);

private:
	bool ReadGrassMap(void* data);
	void ReadMapHeader(SMFHeader& head, CFileHandler& file);
	void ReadMapFeatureHeader(MapFeatureHeader& head, CFileHandler& file);
	void ReadMapFeatureStruct(MapFeatureStruct& head, CFileHandler& file);

	CFileHandler ifs;

	SMFHeader header;
	MapFeatureHeader featureHeader;

	char featureTypes[16384][32];

	int featureFileOffset = 0;
};

#endif // _SMF_MAP_FILE_H
