#pragma once

struct SM2Header
{
	char magic[8];
	int version;
	int xsize;
	int ysize;
	int tilesize;
	int numtiles;
	int tilesPtr;
	int tilemapPtr;
	int minimapPtr;
	int heightmapPtr;
	int metalmapPtr;
	int featurePtr;
};

#define SMALL_TILE_SIZE 680
#define MINIMAP_SIZE 699048
#define MINIMAP_NUM_MIPMAP 9

struct FeatureHeader 
{
	int numFeatureType;
	int numFeatures;
};

struct FeatureFileStruct
{
	int featureType;
	float xpos;
	float ypos;
	float zpos;

	float rotation;
	float relativeSize;
};