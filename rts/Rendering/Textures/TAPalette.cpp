/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "TAPalette.h"
#include "System/FileSystem/FileHandler.h"

CTAPalette palette;

CTAPalette::CTAPalette()
{
	for (auto& color: p) {
		color[0] = 0;
		color[1] = 0;
		color[2] = 0;
		color[3] = 255;
	}
}

void CTAPalette::Init(CFileHandler& paletteFile)
{
	for (auto& color: p) {
		for (auto& channel: color) {
			paletteFile.Read(&color, 1);
		}
		color[3] = 255;
	}
}
