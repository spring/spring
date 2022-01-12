/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SMFMapFile.h"
#include "Map/ReadMap.h"
#include "System/Exceptions.h"
#include "System/StringHash.h"
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
	const char* fmts[] = {"[SMFMapFile::%s] could not open \"%s\"", "[SMFMapFile::%s] corrupt header for \"%s\" (v=%d ts=%d tps=%d ss=%d)"};

	memset(&       header, 0, sizeof(       header));
	memset(&featureHeader, 0, sizeof(featureHeader));
	memset( featureTypes , 0, sizeof(featureTypes ));

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

	memset(&       header, 0, sizeof(       header));
	memset(&featureHeader, 0, sizeof(featureHeader));
	memset( featureTypes , 0, sizeof(featureTypes ));

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
		offset += (Square((mipsize + 3) / 4) * 8);
		mipsize >>= 1;
	}

	data.resize(Square((mipsize + 3) / 4) * 8);

	ifs.Seek(header.minimapPtr + offset);
	ifs.Read(data.data(), data.size());
	return mipsize;
}


// used only by ReadInfoMap (for unitsync)
void CSMFMapFile::ReadHeightmap(unsigned short* heightmap)
{
	const int hmx = header.mapx + 1;
	const int hmy = header.mapy + 1;
	const int len = hmx * hmy;

	ifs.Seek(header.heightmapPtr);
	ifs.Read(heightmap, len * sizeof(unsigned short));

	for (int i = 0; i < len; ++i) {
		swabWordInPlace(heightmap[i]);
	}
}


void CSMFMapFile::ReadHeightmap(float* sHeightMap, float* uHeightMap, float base, float mod)
{
	const int hmx = header.mapx + 1;
	const int hmy = header.mapy + 1;
	const int len = hmx * hmy;

	unsigned short word = 0;

	assert(sHeightMap != nullptr);

	if (uHeightMap == nullptr)
		uHeightMap = sHeightMap;

	ifs.Seek(header.heightmapPtr);

	for (int i = 0; i < len; ++i) {
		ifs.Read(&word, sizeof(word));

		sHeightMap[i] = base + swabWord(word) * mod;
		uHeightMap[i] = sHeightMap[i];
	}
}


void CSMFMapFile::ReadFeatureInfo()
{
	ifs.Seek(header.featurePtr);
	ReadMapFeatureHeader(featureHeader, ifs);

	constexpr size_t S = sizeof(featureTypes   );
	constexpr size_t K = sizeof(featureTypes[0]);
	constexpr size_t N = S / K;

	if (featureHeader.numFeatureType > N) {
		snprintf(featureTypes[0], S - 1, "[SMFMapFile::%s] " _STPF_ " excess feature-types defined\n", __func__, featureHeader.numFeatureType - N);
		throw content_error(featureTypes[0]);
	}

	for (int a = 0; a < featureHeader.numFeatureType; ++a) {
		char* featureType = featureTypes[a];

		for (size_t j = 0, n = K - 1; j < n; j++) {
			ifs.Read(&featureType[j], 1);

			if (featureType[j] == 0)
				break;
		}
	}

	featureFileOffset = ifs.GetPos();
}


void CSMFMapFile::ReadFeatureInfo(MapFeatureInfo* f)
{
	assert(featureFileOffset != 0);
	ifs.Seek(featureFileOffset);

	for (int a = 0; a < featureHeader.numFeatures; ++a) {
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
	return featureTypes[typeID];
}


void CSMFMapFile::GetInfoMapSize(const char* name, MapBitmapInfo* info) const
{
	switch (hashString(name)) {
		case hashString("height"): { *info = MapBitmapInfo(header.mapx + 1, header.mapy + 1); } break;
		case hashString("grass" ): { *info = MapBitmapInfo(header.mapx / 4, header.mapy / 4); } break;
		case hashString("metal" ): { *info = MapBitmapInfo(header.mapx / 2, header.mapy / 2); } break;
		case hashString("type"  ): { *info = MapBitmapInfo(header.mapx / 2, header.mapy / 2); } break;
		default                  : { *info = MapBitmapInfo(              0,               0); } break;
	}
}


bool CSMFMapFile::ReadInfoMap(const char* name, void* data)
{
	switch (hashString(name)) {
		case hashString("height"): {
			ReadHeightmap(reinterpret_cast<unsigned short*>(data));
			return true;
		} break;

		case hashString("grass"): {
			return ReadGrassMap(data);
		} break;

		case hashString("metal"): {
			ifs.Seek(header.metalmapPtr);
			ifs.Read(data, header.mapx / 2 * header.mapy / 2);
			return true;
		} break;

		case hashString("type"): {
			ifs.Seek(header.typeMapPtr);
			ifs.Read(data, header.mapx / 2 * header.mapy / 2);
			return true;
		} break;

		default: {
		} break;
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
		}

		// assumes we can use data as scratch memory
		assert((size - 8) <= (header.mapx / 4 * header.mapy / 4));
		ifs.Read(data, size - 8);
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

