/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ROW_ATLAS_ALLOC_H
#define ROW_ATLAS_ALLOC_H

#include <vector>

#include "IAtlasAllocator.h"


class CRowAtlasAlloc : public IAtlasAllocator
{
public:
	CRowAtlasAlloc();

	virtual bool Allocate();
	virtual int GetMaxMipMaps() { return 0; }

private:
	struct Row {
		Row(int _ypos,int _height):
			position(_ypos),
			height(_height),
			width(0) {
		};

		int position;
		int height;
		int width;
	};

private:
	int nextRowPos;
	std::vector<Row> imageRows;

private:
	void EstimateNeededSize();
	Row* AddRow(int glyphWidth, int glyphHeight);
	Row* FindRow(int glyphWidth, int glyphHeight);
	static int CompareTex(SAtlasEntry* tex1, SAtlasEntry* tex2);
};

#endif // ROW_ATLAS_ALLOC_H
