#ifndef __MAPFILE_H
#define __MAPFILE_H

#include "byteorder.h"

#define SMALL_TILE_SIZE 680
#define MINIMAP_SIZE 699048
#define MINIMAP_NUM_MIPMAP 9

/*
map file (.smf) layout is like this

MapHeader

ExtraHeader
ExtraHeader
.
.
.

Chunk of data pointed to by header or extra headers
Chunk of data pointed to by header or extra headers
.
.
.
*/

struct MapHeader {
	char magic[16]; //"spring map file\0"
	int version;		//must be 1 for now
	int mapid;			//sort of a checksum of the file

	int mapx;				//must be divisible by 128
	int mapy;				//must be divisible by 128
	int squareSize;	//distance between vertices. must be 8
	int texelPerSquare;		//number of texels per square, must be 8 for now
	int tilesize;		//number of texels in a tile, must be 32 for now
	float minHeight;		//height value that 0 in the heightmap corresponds to	
	float maxHeight;		//height value that 0xffff in the heightmap corresponds to

	int heightmapPtr;		//file offset to elevation data (short int[(mapy+1)*(mapx+1)])
	int typeMapPtr;			//file offset to typedata (unsigned char[mapy/2 * mapx/2])
	int tilesPtr;				//file offset to tile data (see MapTileHeader)
	int minimapPtr;			//file offset to minimap (always 1024*1024 dxt1 compresed data with 9 mipmap sublevels)
	int metalmapPtr;		//file offset to metalmap (unsigned char[mapx/2 * mapy/2])
	int featurePtr;			//file offset to feature data	(see MapFeatureHeader)

	int numExtraHeaders;		//numbers of extra headers following main header
};

#define READPTR_MAPHEADER(mh,srcptr)			\
do {							\
	unsigned int __tmpdw;				\
	float __tmpfloat;				\
	(srcptr)->Read((mh).magic,sizeof((mh).magic));	\
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mh).version = (int)swabdword(__tmpdw);		\
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mh).mapid = (int)swabdword(__tmpdw);		\
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mh).mapx = (int)swabdword(__tmpdw);		\
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mh).mapy = (int)swabdword(__tmpdw);		\
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mh).squareSize = (int)swabdword(__tmpdw);	\
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mh).texelPerSquare = (int)swabdword(__tmpdw);	\
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mh).tilesize = (int)swabdword(__tmpdw);	\
	(srcptr)->Read(&__tmpfloat,sizeof(float));	\
	(mh).minHeight = swabfloat(__tmpfloat);		\
	(srcptr)->Read(&__tmpfloat,sizeof(float));	\
	(mh).maxHeight = swabfloat(__tmpfloat);		\
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mh).heightmapPtr = (int)swabdword(__tmpdw);	\
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mh).typeMapPtr = (int)swabdword(__tmpdw);	\
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mh).tilesPtr = (int)swabdword(__tmpdw);	\
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mh).minimapPtr = (int)swabdword(__tmpdw);	\
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mh).metalmapPtr = (int)swabdword(__tmpdw);	\
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mh).featurePtr = (int)swabdword(__tmpdw);	\
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mh).numExtraHeaders = (int)swabdword(__tmpdw);	\
} while (0)
	

//start of every extra header must look like this, then comes data specific for header type
struct ExtraHeader {
	int size;			//size of extra header
	int type;
};


//defined types for extra headers

#define MEH_None 0
//not sure why this one should be used

#define MEH_Vegetation 1
//this extension contains a offset to an unsigned char[mapx/4 * mapy/4] array that defines ground vegetation, if its missing there is no ground vegetation
//0=none
//1=grass
//rest undefined so far


//some structures used in the chunks of data later in the file

struct MapTileHeader
{
	int numTileFiles;
	int numTiles;
};

#define READ_MAPTILEHEADER(mth,src)			\
do {							\
	unsigned int __tmpdw;				\
	(src).Read(&__tmpdw,sizeof(unsigned int));	\
	(mth).numTileFiles = swabdword(__tmpdw);	\
	(src).Read(&__tmpdw,sizeof(unsigned int));	\
	(mth).numTiles = swabdword(__tmpdw);		\
} while (0)

#define READPTR_MAPTILEHEADER(mth,src)			\
do {							\
	unsigned int __tmpdw;				\
	(src)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mth).numTileFiles = swabdword(__tmpdw);	\
	(src)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mth).numTiles = swabdword(__tmpdw);		\
} while (0)

//this is followed by numTileFiles file definition where each file definition is an int followed by a zero terminated file name
//each file defines as many tiles the int indicates with the following files starting where the last one ended
//so if there is 2 files with 100 tiles each the first defines 0-99 and the second 100-199
//after this followes an int[mapx*texelPerSquare/tileSize * mapy*texelPerSquare/tileSize] which is indexes to the defined tiles


struct MapFeatureHeader 
{
	int numFeatureType;
	int numFeatures;
};

#define READ_MAPFEATUREHEADER(mfh,src)			\
do {							\
	unsigned int __tmpdw;				\
	(src)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mfh).numFeatureType = (int)swabdword(__tmpdw);	\
	(src)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mfh).numFeatures = (int)swabdword(__tmpdw);	\
} while (0)

//this is followed by numFeatureType zero terminated strings indicating the names of the features in the map
//then follow numFeatures MapFeatureStructs

struct MapFeatureStruct
{
	int featureType;	//index to one of the strings above
	float xpos;
	float ypos;
	float zpos;

	float rotation;
	float relativeSize;		//not used at the moment keep 1
};

#define READ_MAPFEATURESTRUCT(mfs,src)			\
do {							\
	unsigned int __tmpdw;				\
	float __tmpfloat;				\
	(src)->Read(&__tmpdw,sizeof(unsigned int));	\
	(mfs).featureType = (int)swabdword(__tmpdw);	\
	(src)->Read(&__tmpfloat,sizeof(float));		\
	(mfs).xpos = swabfloat(__tmpfloat);		\
	(src)->Read(&__tmpfloat,sizeof(float));		\
	(mfs).ypos = swabfloat(__tmpfloat);		\
	(src)->Read(&__tmpfloat,sizeof(float));		\
	(mfs).zpos = swabfloat(__tmpfloat);		\
	(src)->Read(&__tmpfloat,sizeof(float));		\
	(mfs).rotation = swabfloat(__tmpfloat);		\
	(src)->Read(&__tmpfloat,sizeof(float));		\
	(mfs).relativeSize = swabfloat(__tmpfloat);	\
} while (0)

/*
map texture tile file (.smt) layout is like this

TileFileHeader

Tiles
.
.
.
*/

struct TileFileHeader
{
	char magic[16];  //"spring tilefile\0"
	int version;		//must be 1 for now

	int numTiles;			//total number of tiles in this file
	int tileSize;			//must be 32 for now
	int compressionType;	//must be 1=dxt1 for now
};

#define READ_TILEFILEHEADER(tfh,src)			\
do {							\
	unsigned int __tmpdw;				\
	(src).Read(&(tfh).magic,sizeof((tfh).magic));	\
	(src).Read(&__tmpdw,sizeof(unsigned int));	\
	(tfh).version = (int)swabdword(__tmpdw);	\
	(src).Read(&__tmpdw,sizeof(unsigned int));	\
	(tfh).numTiles = (int)swabdword(__tmpdw);	\
	(src).Read(&__tmpdw,sizeof(unsigned int));	\
	(tfh).tileSize = (int)swabdword(__tmpdw);	\
	(src).Read(&__tmpdw,sizeof(unsigned int));	\
	(tfh).compressionType = (int)swabdword(__tmpdw);\
} while (0)

//this is followed by the raw data for the tiles
//
#endif //ndef __MAPFILE_H
