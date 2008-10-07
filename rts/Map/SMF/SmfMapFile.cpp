/* Author: Tobi Vollebregt */

#include "StdAfx.h"     // ugh
#include <assert.h>
#include "SmfMapFile.h"
#include "mapfile.h"
#include "mmgr.h"
#include "System/Exceptions.h"

using std::string;


CSmfMapFile::CSmfMapFile(const string& mapname)
	: ifs(string("maps/") + mapname), featureFileOffset(0)
{
	memset(&header, 0, sizeof(header));
	memset(&featureHeader, 0, sizeof(featureHeader));

	if (!ifs.FileExists())
		throw content_error("Couldn't open map file " + mapname);
	READPTR_MAPHEADER(header, (&ifs));

	if (strcmp(header.magic, "spring map file") != 0 ||
	    header.version != 1 || header.tilesize != 32 ||
	    header.texelPerSquare != 8 || header.squareSize != 8)
		throw content_error("Incorrect map file " + mapname);
}


void CSmfMapFile::ReadMinimap(void* data)
{
	ifs.Seek(header.minimapPtr);
	ifs.Read(data, MINIMAP_SIZE);
}


void CSmfMapFile::ReadHeightmap(unsigned short* heightmap)
{
	const int hmx = header.mapx + 1;
	const int hmy = header.mapy + 1;

	ifs.Seek(header.heightmapPtr);
	ifs.Read(heightmap, hmx * hmy * sizeof(short));

	for (int y = 0; y < hmx * hmy; ++y) {
		heightmap[y] = swabword(heightmap[y]);
	}
}


void CSmfMapFile::ReadHeightmap(float* heightmap, float base, float mod)
{
	const int hmx = header.mapx + 1;
	const int hmy = header.mapy + 1;
	unsigned short* temphm = SAFE_NEW unsigned short[hmx * hmy];

	ifs.Seek(header.heightmapPtr);
	ifs.Read(temphm, hmx * hmy * 2);

	for (int y = 0; y < hmx * hmy; ++y) {
		heightmap[y] = base + swabword(temphm[y]) * mod;
	}

	delete[] temphm;
}


void CSmfMapFile::ReadFeatureInfo()
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


void CSmfMapFile::ReadFeatureInfo(MapFeatureInfo* f)
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


const char* CSmfMapFile::GetFeatureType(int typeID) const
{
	assert(typeID >= 0 && typeID < featureHeader.numFeatureType);
	return featureTypes[typeID].c_str();
}


MapBitmapInfo CSmfMapFile::GetInfoMapSize(const string& name) const
{
	if (name == "height") {
		return MapBitmapInfo(header.mapx + 1, header.mapy + 1);
	}
	else if (name == "grass") {
		return MapBitmapInfo(header.mapx / 4, header.mapy / 4);
	}
	else if (name == "metal") {
		return MapBitmapInfo(header.mapx / 2, header.mapy / 2);
	}
	else if (name == "type") {
		return MapBitmapInfo(header.mapx / 2, header.mapy / 2);
	}
	return MapBitmapInfo(0, 0);
}


bool CSmfMapFile::ReadInfoMap(const string& name, void* data)
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


void CSmfMapFile::ReadGrassMap(void *data)
{
	ifs.Seek(sizeof(SMFHeader));

	for (int a = 0; a < header.numExtraHeaders; ++a) {
		int size;
		ifs.Read(&size, 4);
		size = swabdword(size);
		int type;
		ifs.Read(&type, 4);
		type = swabdword(type);
		if (type == MEH_Vegetation) {
			int pos;
			ifs.Read(&pos, 4);
			pos = swabdword(pos);
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
