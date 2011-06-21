/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMFMAPFILE_H
#define SMFMAPFILE_H

#include "FileSystem/FileHandler.h"
#include "mapfile.h"
#include <string>
#include <vector>

struct MapFeatureInfo;
struct MapBitmapInfo;

class CSmfMapFile
{
public:

	CSmfMapFile(const std::string& mapFileName);

	void ReadMinimap(void* data);
	/// @return mipsize
	int ReadMinimap(std::vector<boost::uint8_t>& data, unsigned miplevel);
	void ReadHeightmap(unsigned short* heightmap);
	void ReadHeightmap(float* heightmap, float base, float mod);
	void ReadFeatureInfo();
	void ReadFeatureInfo(MapFeatureInfo* f);
	void GetInfoMapSize(const std::string& name, MapBitmapInfo*) const;
	bool ReadInfoMap(const std::string& name, void* data);

	int GetNumFeatures()     const { return featureHeader.numFeatures; }
	int GetNumFeatureTypes() const { return featureHeader.numFeatureType; }

	const char* GetFeatureTypeName(int typeID) const;

	const SMFHeader& GetHeader() const { return header; }

	// todo: do not use, just here for backward compatibility with SMFGroundTextures.cpp
	CFileHandler* GetFileHandler() { return &ifs; }

private:

	void ReadGrassMap(void* data);

	SMFHeader header;
	CFileHandler ifs;

	MapFeatureHeader featureHeader;
	std::vector<std::string> featureTypes;
	int featureFileOffset;
};

#endif // SMFMAPFILE_H
