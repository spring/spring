#pragma once

#include "ReadMap.h"

class CTileSetReader : public CReadMap
{
public:
	//CTileSetReader(std::string filename);
	virtual ~CTileSetReader(void){};
	virtual void RecalcTexture(int x1, int x2, int y1, int y2)=0;

	int numXTile;
	int numYTile;
	int tileDiv;

	unsigned int *tilesettex;
	unsigned short *tilemap;

	unsigned int *displmap;
	/*struct TexPart {
		float xstart;
		float xend;
		float ystart;
		float yend;
	};

	TexPart *tilesetTexCoords;*/
protected:
};