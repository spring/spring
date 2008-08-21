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

	for(int a=0;a<10;++a)
		teamColor[a][3]=255;

	teamColor[0][0]=90;  //blue 
	teamColor[0][1]=90; 
	teamColor[0][2]=255; 

	teamColor[1][0]=200; //red 
	teamColor[1][1]=0; 
	teamColor[1][2]=0; 

	teamColor[2][0]=255; //white 
	teamColor[2][1]=255; 
	teamColor[2][2]=255; 

	teamColor[3][0]=38; //green 
	teamColor[3][1]=155; 
	teamColor[3][2]=32; 

	teamColor[4][0]=7;  //blue 
	teamColor[4][1]=31; 
	teamColor[4][2]=125; 

	teamColor[5][0]=150; //purple 
	teamColor[5][1]=10; 
	teamColor[5][2]=180; 

	teamColor[6][0]=255; //yellow 
	teamColor[6][1]=255; 
	teamColor[6][2]=0; 

	teamColor[7][0]=50;  //black 
	teamColor[7][1]=50; 
	teamColor[7][2]=50; 

	teamColor[8][0]=152; // ltblue 
	teamColor[8][1]=200; 
	teamColor[8][2]=220; 

	teamColor[9][0]=171; //tan 
	teamColor[9][1]=171; 
	teamColor[9][2]=131;
}
