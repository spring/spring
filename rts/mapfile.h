#pragma once

struct MapHeader {
	char magic[16]; //"spring map file"
	int version;		//1
	int mapid;			//sort of a checksum of the file

	int mapx;				//must be 512
	int mapy;				//must be 512
	int squareSize;	//must be 8

	int numCities;
	int numNeutrals;//how many cities is neutral at start of game
	float incomeMul;//all income  are multiplied by this
	float supplyMul;//modify how easily supply can flow over the map

	int elevOffset;	//file offset to elevation data (short int[(mapy+1)*(mapx+1)])
	int tileOffset;	//file offset to tile data (unsigned char[mapy*mapx])
	int cityInfoOffset;	//file offset to city info data (int+int+int[numCities])
	int cityOffset;	//file offset to city data (unsigned char[mapy*mapx])
	int vegOffset;	//file offset to vegetation data (unsigned char[mapy*mapx]) note that this data is unsynced
	int previewOffset; //file offset to a picture of the map (unsigned char[256*256*3])
};
