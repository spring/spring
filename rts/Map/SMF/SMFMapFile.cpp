/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SMFMapFile.h"
#include "Map/ReadMap.h"
#include "System/Exceptions.h"
#include "System/Platform/byteorder.h"

#include <cassert>
#include <cstring>


static bool CheckHeader(const SMFHeader& h)
{
	if (h.version != 1)
		return false;
	if (h.tilesize != 32)
		return false;
	if (h.texelPerSquare != 8)
		return false;
	if (h.squareSize != 8)
		return false;

	return (std::strcmp(h.magic, "spring map file") == 0);
}


void CSMFMapFile::Open(const std::string& mapFileName)
{
	char buf[512] = {0};
	const char* fmts[] = {"[%s] could not open \"%s\"", "[%s] corrupt header for \"%s\" (v=%d ts=%d tps=%d ss=%d)"};

	memset(&header, 0, sizeof(header));
	memset(&featureHeader, 0, sizeof(featureHeader));

	ifs.Open(mapFileName);

	if (!ifs.FileExists()) {
		snprintf(buf, sizeof(buf), fmts[0], __func__, mapFileName.c_str());
		throw content_error(buf);
	}

	ReadMapHeader(header, ifs);

	if (CheckHeader(header))
		return;

	snprintf(buf, sizeof(buf), fmts[1], __func__, mapFileName.c_str(), header.version, header.tilesize, header.texelPerSquare, header.squareSize);
	throw content_error(buf);
}

void CSMFMapFile::Close()
{
	ifs.Close();

	featureTypes.clear();

	featureFileOffset = 0;
}


void CSMFMapFile::ReadMinimap(void* data)
{
	ifs.Seek(header.minimapPtr);
	ifs.Read(data, MINIMAP_SIZE);
}

int CSMFMapFile::ReadMinimap(std::vector<std::uint8_t>& data, unsigned miplevel)
{
	int offset = 0;
	int mipsize = 1024;

	for (unsigned i = 0, n = MINIMAP_NUM_MIPMAP; i < std::min(n, miplevel); i++) {
		const int size = ((mipsize + 3) / 4) * ((mipsize + 3) / 4) * 8;
		offset += size;
		mipsize >>= 1;
	}

	const int size = ((mipsize + 3) / 4) * ((mipsize + 3) / 4) * 8;
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

		if (sHeightMap != nullptr) { sHeightMap[y] = h; }
		if (uHeightMap != nullptr) { uHeightMap[y] = h; }
	}

	delete[] temphm;
}


void CSMFMapFile::ReadFeatureInfo()
{
	ifs.Seek(header.featurePtr);
	ReadMapFeatureHeader(featureHeader, ifs);

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
		ReadMapFeatureStruct(ffs, ifs);

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


void CSMFMapFile::GetInfoMapSize(const std::string& name, MapBitmapInfo* info) const
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


bool CSMFMapFile::ReadInfoMap(const std::string& name, void* data)
{
	if (name == "height") {
		ReadHeightmap((unsigned short*)data);
		return true;
	}
	else if (name == "grass") {
		return ReadGrassMap(data);
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


bool CSMFMapFile::ReadGrassMap(void *data)
{
	ifs.Seek(sizeof(SMFHeader));

	for (int a = 0; a < header.numExtraHeaders; ++a) {
		int size;
		int type;

		ifs.Read(&size, 4);
		ifs.Read(&type, 4);

		swabDWordInPlace(size);
		swabDWordInPlace(type);

		if (type == MEH_Vegetation) {
			int pos;
			ifs.Read(&pos, 4);
			swabDWordInPlace(pos);
			ifs.Seek(pos);
			ifs.Read(data, header.mapx / 4 * header.mapy / 4);
			/* char; no swabbing. */
			return true; //we arent interested in other extensions anyway
		} else {
			// assumes we can use data as scratch memory
			assert(size - 8 <= header.mapx / 4 * header.mapy / 4);
			ifs.Read(data, size - 8);
		}
	}
	return false;
}


/// read a float from file (endian aware)
static float ReadFloat(CFileHandler& file)
{
	float __tmpfloat = 0.0f;
	file.Read(&__tmpfloat, sizeof(float));
	return swabFloat(__tmpfloat);
}

/// read an int from file (endian aware)
static int ReadInt(CFileHandler& file)
{
	unsigned int __tmpdw = 0;
	file.Read(&__tmpdw, sizeof(unsigned int));
	return (int)swabDWord(__tmpdw);
}


/// Read SMFHeader head from file
void CSMFMapFile::ReadMapHeader(SMFHeader& head, CFileHandler& file)
{
	file.Read(head.magic, sizeof(head.magic));

	head.version = ReadInt(file);
	head.mapid = ReadInt(file);
	head.mapx = ReadInt(file);
	head.mapy = ReadInt(file);
	head.squareSize = ReadInt(file);
	head.texelPerSquare = ReadInt(file);
	head.tilesize = ReadInt(file);
	head.minHeight = ReadFloat(file);
	head.maxHeight = ReadFloat(file);
	head.heightmapPtr = ReadInt(file);
	head.typeMapPtr = ReadInt(file);
	head.tilesPtr = ReadInt(file);
	head.minimapPtr = ReadInt(file);
	head.metalmapPtr = ReadInt(file);
	head.featurePtr = ReadInt(file);
	head.numExtraHeaders = ReadInt(file);
}

/// Read MapFeatureHeader head from file
void CSMFMapFile::ReadMapFeatureHeader(MapFeatureHeader& head, CFileHandler& file)
{
	head.numFeatureType = ReadInt(file);
	head.numFeatures = ReadInt(file);
}

/// Read MapFeatureStruct head from file
void CSMFMapFile::ReadMapFeatureStruct(MapFeatureStruct& head, CFileHandler& file)
{
	head.featureType = ReadInt(file);
	head.xpos = ReadFloat(file);
	head.ypos = ReadFloat(file);
	head.zpos = ReadFloat(file);
	head.rotation = ReadFloat(file);
	head.relativeSize = ReadFloat(file);
}

/// Read MapTileHeader head from file
void CSMFMapFile::ReadMapTileHeader(MapTileHeader& head, CFileHandler& file)
{
	head.numTileFiles = ReadInt(file);
	head.numTiles = ReadInt(file);
}

/// Read TileFileHeader head from file src
void CSMFMapFile::ReadMapTileFileHeader(TileFileHeader& head, CFileHandler& file)
{
	file.Read(&head.magic, sizeof(head.magic));
	head.version = ReadInt(file);
	head.numTiles = ReadInt(file);
	head.tileSize = ReadInt(file);
	head.compressionType = ReadInt(file);
}

