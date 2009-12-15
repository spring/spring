#include "StdAfx.h"
#include "mmgr.h"

#include "TAPalette.h"
#include "FileSystem/FileHandler.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTAPalette palette;

CTAPalette::CTAPalette()
{
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
