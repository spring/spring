/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "TAPalette.h"
#include "FileSystem/FileHandler.h"

CTAPalette palette;

CTAPalette::CTAPalette()
{
	for(int c=0;c<256;c++){
		p[c][0]=0;
		p[c][1]=0;
		p[c][2]=0;		
		p[c][3]=255;
	}
}

CTAPalette::~CTAPalette()
{
}

void CTAPalette::Init(void)
{
	CFileHandler pal("PALETTE.PAL");

	if (pal.FileExists()) {
		for(int c=0;c<256;c++){
			for(int c2=0;c2<4;c2++){
				pal.Read(&p[c][c2],1);
			}
			p[c][3]=255;
		}
	}
}
