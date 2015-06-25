/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
@brief Defines the Spring map format

This file defines the Spring map format that is most common at the time of this
writing, sometimes also refered to as SM2 (Spring Map format 2).

This type of maps consists of a .smd (Spring Map Definition/Description) file,
a .smf (Spring Map Format) file and a .smt (Spring Map Tiles) file. As you may
have guessed by this, the map is a tilemap.

The SMD file is a file in the textual TDF format defining a number of
properties for the map.

The SMF file is the main file for the map, containing the tilemap and a lot of
additional data. See SMFHeader for details.

The SMT file, for which the filename is stored in the SMF file, contains the
tiles used by the map. This file can be shared between different maps. See
TileFileHeader for details.
*/

#ifndef SMF_MAPFORMAT_H
#define SMF_MAPFORMAT_H

/// Size in bytes of a single tile in the .smt
#define SMALL_TILE_SIZE 680
/// Size in bytes of the minimap (all 9 mipmap levels) in the .smf
#define MINIMAP_SIZE 699048
/// Number of mipmap levels stored in the file
#define MINIMAP_NUM_MIPMAP 9

/**
@brief Spring Map File (.smf) main header

Map file (.smf) layout is like this:

	- SMFHeader
	- ExtraHeader
	- ExtraHeader
	- ...
	- Chunk of data pointed to by header or extra headers
	- Chunk of data pointed to by header or extra headers
	- ...
*/
struct SMFHeader {
	char magic[16];      ///< "spring map file\0"
	int version;         ///< Must be 1 for now
	int mapid;           ///< Sort of a GUID of the file, just set to a random value when writing a map

	int mapx;            ///< Must be divisible by 128
	int mapy;            ///< Must be divisible by 128
	int squareSize;      ///< Distance between vertices. Must be 8
	int texelPerSquare;  ///< Number of texels per square, must be 8 for now
	int tilesize;        ///< Number of texels in a tile, must be 32 for now
	float minHeight;     ///< Height value that 0 in the heightmap corresponds to	
	float maxHeight;     ///< Height value that 0xffff in the heightmap corresponds to

	int heightmapPtr;    ///< File offset to elevation data (short int[(mapy+1)*(mapx+1)])
	int typeMapPtr;      ///< File offset to typedata (unsigned char[mapy/2 * mapx/2])
	int tilesPtr;        ///< File offset to tile data (see MapTileHeader)
	int minimapPtr;      ///< File offset to minimap (always 1024*1024 dxt1 compresed data plus 8 mipmap sublevels)
	int metalmapPtr;     ///< File offset to metalmap (unsigned char[mapx/2 * mapy/2])
	int featurePtr;      ///< File offset to feature data (see MapFeatureHeader)

	int numExtraHeaders; ///< Numbers of extra headers following main header
};

/**
@brief Header for extensions in .smf file

Start of every extra header must look like this, then comes data specific
for header type.

Defined ExtraHeader types are:

	- MEH_None
	- MEH_Vegetation
*/
struct ExtraHeader {
	int size; ///< Size of extra header
	int type; ///< Type of extra header
};

// Defined types for extra headers

/// Not sure why this one should be used
#define MEH_None 0

/**
@brief Extension containing a ground vegetation map

This extension contains an offset to an unsigned char[mapx/4 * mapy/4] array
that defines ground vegetation, if it's missing there is no ground vegetation.

	- 0=none
	- 1=grass
	- rest undefined so far
*/
#define MEH_Vegetation 1

// Some structures used in the chunks of data later in the file

/**
@brief The header at offset SMFHeader.tilesPtr in the .smf

MapTileHeader is followed by numTileFiles file definition where each file
definition is an int followed by a zero terminated file name. On loading,
Spring prepends the filename with "maps/" and looks in it's VFS for a file
with the resulting filename. See TileFileHeader for details.

Each file defines as many tiles the int indicates with the following files
starting where the last one ended so if there is 2 files with 100 tiles each
the first defines 0-99 and the second 100-199.

After this followes an int[mapx*texelPerSquare/tileSize * mapy*texelPerSquare/tileSize]
(this is int[mapx/4 * mapy/4] with currently hardcoded texelPerSquare=8 and tileSize=32)
which are indices to the defined tiles.
*/
struct MapTileHeader
{
	int numTileFiles; ///< Number of tile files to read in (usually 1)
	int numTiles;     ///< Total number of tiles
};

/**
@brief The header at offset SMFHeader.featurePtr in the .smf

MapFeatureHeader is followed by numFeatureType zero terminated strings indicating the names
of the features in the map. Then follow numFeatures MapFeatureStructs.
*/
struct MapFeatureHeader 
{
	int numFeatureType;
	int numFeatures;
};

/**
@brief Structure defining how features are stored in .smf

MapFeatureHeader is followed by numFeatureType zero terminated strings
indicating the names of the features in the map. Then follow numFeatures
MapFeatureStructs.
*/
struct MapFeatureStruct
{
	int featureType;    ///< Index to one of the strings above
	float xpos;         ///< X coordinate of the feature
	float ypos;         ///< Y coordinate of the feature (height)
	float zpos;         ///< Z coordinate of the feature

	float rotation;     ///< Orientation of this feature (-32768..32767 for full circle)
	float relativeSize; ///< Not used at the moment keep 1
};


/**
@brief Spring Tile File (.smt) main header

Map texture tile file (.smt) layout is like this:

	- TileFileHeader
	- Tile
	- Tile
	- ...

In other words TileFileHeader is followed by the raw data for the tiles.

Each 32x32 tile is dxt1 compressed data with 4 mipmap levels. This takes up
exactly SMALL_TILE_SIZE (680) bytes per tile (512 + 128 + 32 + 8).
*/
struct TileFileHeader
{
	char magic[16];      ///< "spring tilefile\0"
	int version;         ///< Must be 1 for now

	int numTiles;        ///< Total number of tiles in this file
	int tileSize;        ///< Must be 32 for now
	int compressionType; ///< Must be 1 (= dxt1) for now
};

#endif //ndef _MAPFILE_H

