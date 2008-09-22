/* Author: Tobi Vollebregt */

#ifndef SMFMAPFILE_H
#define SMFMAPFILE_H

#include "FileSystem/FileHandler.h"
#include "Map/ReadMap.h"
#include "mapfile.h"
#include <string>
#include <vector>


class CSmfMapFile
{
public:

	CSmfMapFile(const std::string& mapname);

	void ReadMinimap(void* data);
	void ReadHeightmap(unsigned short* heightmap);
	void ReadHeightmap(float* heightmap, float base, float mod);
	void ReadFeatureInfo();
	void ReadFeatureInfo(MapFeatureInfo* f);
	bool ReadInfoMap(const std::string& name, unsigned char* infomap, MapBitmapInfo* bmInfo);

	int GetNumFeatures()     const { return featureHeader.numFeatures; }
	int GetNumFeatureTypes() const { return featureHeader.numFeatureType; }

	const char* GetFeatureType(int typeID) const;

	const SMFHeader& GetHeader() const { return header; }

	// todo: do not use, just here for backward compatibility with BFGroundTextures.cpp
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
