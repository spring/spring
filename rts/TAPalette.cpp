#include "StdAfx.h"
#include "TAPalette.h"
#include "FileHandler.h"
#include "TAPalette.h"
#include <iostream>
//#include <fstream>
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTAPalette palette;

CTAPalette::CTAPalette()
{
	Init();
}

CTAPalette::~CTAPalette()
{

}

void CTAPalette::Init(void)
{
	CFileHandler pal("palette.pal");

	if(!pal.FileExists()){
	  std::cerr << "Error loading \"palette.pal\"" << std::endl;
	  exit(1);
	}
	for(int c=0;c<256;c++){
		for(int c2=0;c2<4;c2++){
			pal.Read(&p[c][c2],1);
		}
		p[c][3]=255;
	}
	for(int a=0;a<10;++a)
		teamColor[a][3]=255;

	teamColor[0][0]=90;
	teamColor[0][1]=90;
	teamColor[0][2]=255;

	teamColor[1][0]=255;
	teamColor[1][1]=80;
	teamColor[1][2]=80;

	teamColor[2][0]=255;
	teamColor[2][1]=255;
	teamColor[2][2]=255;

	teamColor[3][0]=70;
	teamColor[3][1]=220;
	teamColor[3][2]=70;

	teamColor[4][0]=10;
	teamColor[4][1]=10;
	teamColor[4][2]=220;

	teamColor[5][0]=150;
	teamColor[5][1]=10;
	teamColor[5][2]=180;

	teamColor[6][0]=255;
	teamColor[6][1]=255;
	teamColor[6][2]=0;

	teamColor[7][0]=60;
	teamColor[7][1]=60;
	teamColor[7][2]=60;

	teamColor[8][0]=140;
	teamColor[8][1]=190;
	teamColor[8][2]=255;

	teamColor[9][0]=160;
	teamColor[9][1]=160;
	teamColor[9][2]=145;
}
