/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "TAPalette.h"
#include "FileSystem/FileHandler.h"

CTAPalette palette;

CTAPalette::CTAPalette()
{
	for (unsigned c = 0; c < 256; ++c) {
		p[c][0] = 0;
		p[c][1] = 0;
		p[c][2] = 0;		
		p[c][3] = 255;
	}
}

CTAPalette::~CTAPalette()
{
}

void CTAPalette::Init()
{
	CFileHandler pal("PALETTE.PAL");

	if (pal.FileExists()) {
		for (unsigned c = 0; c < 256; ++c) {
			for (unsigned c2 = 0; c2 < 4; ++c2) {
				pal.Read(&p[c][c2], 1);
			}
			p[c][3] = 255;
		}
	}
}
