/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "TAPalette.h"
#include "System/FileSystem/FileHandler.h"

CTAPalette palette;

CTAPalette::CTAPalette()
{
	for (unsigned c = 0; c < NUM_PALETTE_ENTRIES; ++c) {
		p[c][0] = 0;
		p[c][1] = 0;
		p[c][2] = 0;
		p[c][3] = 255;
	}
}

void CTAPalette::Init(CFileHandler& paletteFile)
{
	for (unsigned c = 0; c < NUM_PALETTE_ENTRIES; ++c) {
		for (unsigned c2 = 0; c2 < 4; ++c2) {
			paletteFile.Read(&p[c][c2], 1);
		}
		p[c][3] = 255;
	}
}
