/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SMFMapFile.h"
#include "Map/ReadMap.h"
#include "System/Exceptions.h"

#include <cassert>
#include <cstring>

using std::string;


CSMFMapFile::CSMFMapFile(const string& mapFileName)
	: ifs(mapFileName), featureFileOffset(0)
{
	memset(&header, 0, sizeof(header));
	memset(&featureHeader, 0, sizeof(featureHeader));

	if (!ifs.FileExists())
		throw content_error("Couldn't open map file " + mapFileName);

	READPTR_MAPHEADER(header, (&ifs));

	if (strcmp(header.magic, "spring map file") != 0 ||
	    header.version != 1 || header.tilesize != 32 ||
	    header.texelPerSquare != 8 || header.squareSize != 8)
		throw content_error("Incorrect map file " + mapFileName);
}


void CSMFMapFile::ReadMinimap(void* data)
{
	ifs.Seek(header.minimapPtr);
	ifs.Read(data, MINIMAP_SIZE);
}

int CSMFMapFile::ReadMinimap(std::vector<boost::uint8_t>& data, unsigned miplevel)
{
	int offset=0;
	int mipsize = 1024;
	for (unsigned i = 0; i < std::min((unsigned)MINIMAP_NUM_MIPMAP, miplevel); i++)
	{
		const int size = ((mipsize+3)/4)*((mipsize+3)/4)*8;
		offset += size;
		mipsize >>= 1;
	}

	const int size = ((mipsize+3)/4)*((mipsize+3)/4)*8;
	data.resize(size);
	
	ifs.Seek(header.minimapPtr + offset);
	ifs.Read(&data[0], size);
	return mipsize;
}


// used only by ReadInfoMap (for unitsync)
void CSMFMapFile::ReadHeightmap(unsigned short* heightmap)
{
	const int hmx = header.mapx + 1;
	const int hmy = header.mapy + 1;

	ifs.Seek(header.heightmapPtr);
	ifs.Read(heightmap, hmx * hmy * sizeof(short));

	for (int y = 0; y < hmx * hmy; ++y) {
		swabWordInPlace(heightmap[y]);
	}
}


void CSMFMapFile::ReadHeightmap(float* sHeightMap, float* uHeightMap, float base, float mod)
{
	const int hmx = header.mapx + 1;
	const int hmy = header.mapy + 1;
	unsigned short* temphm = new unsigned short[hmx * hmy];

	ifs.Seek(header.heightmapPtr);
	ifs.Read(temphm, hmx * hmy * 2);

	for (int y = 0; y < hmx * hmy; ++y) {
		const float h = base + swabWord(temphm[y]) * mod;

		if (sHeightMap != NULL) { sHeightMap[y] = h; }
		if (uHeightMap != NULL) { uHeightMap[y] = h; }
	}

	delete[] temphm;
}


void CSMFMapFile::ReadFeatureInfo()
{
	ifs.Seek(header.featurePtr);
	READ_MAPFEATUREHEADER(featureHeader, (&ifs));

	featureTypes.resize(featureHeader.numFeatureType);

	for(int a = 0; a < featureHeader.numFeatureType; ++a) {
		char c;
		ifs.Read(&c, 1);
		while (c) {
			featureTypes[a] += c;
			ifs.Read(&c, 1);
		}
	}
	featureFileOffset = ifs.GetPos();
}


void CSMFMapFile::ReadFeatureInfo(MapFeatureInfo* f)
{
	assert(featureFileOffset != 0);
	ifs.Seek(featureFileOffset);
	for(int a = 0; a < featureHeader.numFeatures; ++a) {
		MapFeatureStruct ffs;
		READ_MAPFEATURESTRUCT(ffs, (&ifs));

		f[a].featureType = ffs.featureType;
		f[a].pos = float3(ffs.xpos, ffs.ypos, ffs.zpos);
		f[a].rotation = ffs.rotation;
	}
}


const char* CSMFMapFile::GetFeatureTypeName(int typeID) const
{
	assert(typeID >= 0 && typeID < featureHeader.numFeatureType);
	return featureTypes[typeID].c_str();
}


void CSMFMapFile::GetInfoMapSize(const string& name, MapBitmapInfo* info) const
{
	if (name == "height") {
		*info = MapBitmapInfo(header.mapx + 1, header.mapy + 1);
	}
	else if (name == "grass") {
		*info = MapBitmapInfo(header.mapx / 4, header.mapy / 4);
	}
	else if (name == "metal") {
		*info = MapBitmapInfo(header.mapx / 2, header.mapy / 2);
	}
	else if (name == "type") {
		*info = MapBitmapInfo(header.mapx / 2, header.mapy / 2);
	}
	else {
		*info = MapBitmapInfo(0, 0);
	}
}


bool CSMFMapFile::ReadInfoMap(const string& name, void* data)
{
	if (name == "height") {
		ReadHeightmap((unsigned short*)data);
		return true;
	}
	else if (name == "grass") {
		ReadGrassMap(data);
		return true;
	}
	else if(name == "metal") {
		ifs.Seek(header.metalmapPtr);
		ifs.Read(data, header.mapx / 2 * header.mapy / 2);
		return true;
	}
	else if(name == "type") {
		ifs.Seek(header.typeMapPtr);
		ifs.Read(data, header.mapx / 2 * header.mapy / 2);
		return true;
	}
	return false;
}


void CSMFMapFile::ReadGrassMap(void *data)
{
	ifs.Seek(sizeof(SMFHeader));

	for (int a = 0; a < header.numExtraHeaders; ++a) {
		int size;
		ifs.Read(&size, 4);
		swabDWordInPlace(size);
		int type;
		ifs.Read(&type, 4);
		swabDWordInPlace(type);
		if (type == MEH_Vegetation) {
			int pos;
			ifs.Read(&pos, 4);
			swabDWordInPlace(pos);
			ifs.Seek(pos);
			ifs.Read(data, header.mapx / 4 * header.mapy / 4);
			/* char; no swabbing. */
			break; //we arent interested in other extensions anyway
		}
		else {
			// assumes we can use data as scratch memory
			assert(size - 8 <= header.mapx / 4 * header.mapy / 4);
			ifs.Read(data, size - 8);
		}
	}
}
