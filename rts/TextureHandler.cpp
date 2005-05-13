// TextureHandler.cpp: implementation of the CTextureHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TextureHandler.h"
#include "myGL.h"
#include <GL/glu.h>			// Header file for the gLu32 library
#include <math.h>
#include "InfoConsole.h"
#include <vector>
#include "Bitmap.h"
#include "TAPalette.h"
#include "FileHandler.h"
#include <algorithm>
#include <cctype>
//#include "mmgr.h"
#include "RegHandler.h"
#include "TextureFilters.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTextureHandler* texturehandler=0;

struct TexFile {
	CBitmap tex;
	string name;
};

static int CompareTatex2( const void *arg1, const void *arg2 ){
	if((*(TexFile**)arg1)->tex.ysize > (*(TexFile**)arg2)->tex.ysize)
	   return -1;
   return 1;
}

CTextureHandler::CTextureHandler()
{
	PrintLoadMsg("Creating unit textures");

	TexFile* texfiles[10000];

	int numfiles=0;
	int totalSize=0;

	std::vector<std::string> files=CFileHandler::FindFiles("unittextures\\tatex\\*.BMP");

	for(vector<string>::iterator fi=files.begin();fi!=files.end();++fi){
		string s=std::string(*fi);

		TexFile* tex=new TexFile;
		tex->tex.Load(s);

		s.erase(0,s.find_last_of('\\')+1);
		std::transform(s.begin(), s.end(), s.begin(), (int (*)(int))std::tolower);

		tex->name=s;
	
		texfiles[numfiles++]=tex;
		totalSize+=tex->tex.xsize*tex->tex.ysize;
	}
	
	for(int a=0;a<256;++a){
		string name="ta_color";
		char t[50];
		sprintf(t,"%i",a);
		name+=t;
		TexFile* tex=new TexFile;
		tex->name=name;
		tex->tex.mem[0]=palette[a][0];
		tex->tex.mem[1]=palette[a][1];
		tex->tex.mem[2]=palette[a][2];
		tex->tex.mem[3]=palette[a][3];

		texfiles[numfiles++]=tex;
		totalSize+=tex->tex.xsize*tex->tex.ysize;
	}
	totalSize*=1.2f;		//pessimistic guess about how much space will be wasted

	if(totalSize<1024*1024){
		bigTexX=1024;
		bigTexY=1024;
	} else if(totalSize<1024*2048){
		bigTexX=1024;
		bigTexY=2048;
	} else if(totalSize<2048*2048){
		bigTexX=2048;
		bigTexY=2048;
	} else {
		bigTexX=2048;
		bigTexY=2048;
		MessageBox(0,"To many/large unit textures to fit in 2048*2048","Error",0);
	}

	qsort(texfiles,numfiles,sizeof(TexFile*),CompareTatex2);

	unsigned char* tex=new unsigned char[bigTexX*bigTexY*4];    
	for(int a=0;a<bigTexX*bigTexY*4;++a){
		tex[a]=128;
	}

	int cury=0;
	int curx=0;
	int maxy=0;
	for(int a=0;a<numfiles;++a){
		CBitmap* curtex=&texfiles[a]->tex;
		if(curx+curtex->xsize>bigTexX){
			curx=0;
			cury=maxy;
		}
		if(cury+curtex->ysize>maxy){
			maxy=cury+curtex->ysize;
		}
		if(maxy>bigTexY){
			MessageBox(0,"To many/large unit textures","Error",0);
			break;
		}
		for(int y=0;y<curtex->ysize;y++){
			for(int x=0;x<curtex->xsize;x++){
//				if(curtex->mem[(y*curtex->xsize+x)*4]==254 && curtex->mem[(y*curtex->xsize+x)*4+1]==0 && curtex->mem[(y*curtex->xsize+x)*4+2]==254){
//					tex[((cury+y)*bigTexX+(curx+x))*4+3]=0;
//				} else {
					for(int col=0;col<4;col++){
						tex[((cury+y)*bigTexX+(curx+x))*4+col]=curtex->mem[(y*curtex->xsize+x)*4+col];
//					}
				}
			}
		}

		UnitTexture* unittex=new UnitTexture;

		unittex->xstart=(curx+0.5f)/(float)bigTexX;
		unittex->ystart=(cury+0.5f)/(float)bigTexY;
		unittex->xend=(curx+curtex->xsize-0.5f)/(float)bigTexX;
		unittex->yend=(cury+curtex->ysize-0.5f)/(float)bigTexY;
		textures[texfiles[a]->name]=unittex;
		
		curx+=curtex->xsize;
		delete texfiles[a];

	}

	int TQ = regHandler.GetInt("UnitTextureQuality",0);

	if (TQ)
	{
		int oldTexX = bigTexX;
		int oldTexY = bigTexY;
		int mul = 2;
		if (TQ == 3)
			mul = 4;
		bigTexX*= mul;
		bigTexY*= mul;
		unsigned char* temptex=new unsigned char[bigTexX*bigTexY*4];
#ifndef NO_TEXTURES
		switch (TQ)
		{
		case 1:		// 2xSaI
#ifndef NO_WINSTUFF
			Super2xSaI_32( (uint32 *)tex, (uint32 *)temptex, oldTexX, oldTexY, oldTexX);
#else 
			Super2xSaI_32( (DWORD*)tex, (DWORD*)temptex, oldTexX, oldTexY, oldTexX);
#endif
			break;
		case 3:		// 4xHQ
			hq4x_init();
			hq4x_32( tex, temptex, oldTexX, oldTexY, oldTexX, bigTexX*4);
			break;
		default:	// 2xHQ
			hq2x_init(32);
			hq2x_32( tex, oldTexX*4, temptex, bigTexX*4, oldTexX, oldTexY);
			break;
		}
#endif
		delete[] tex;
		tex = temptex;
	}
	glGenTextures(1, &globalTex);
	glBindTexture(GL_TEXTURE_2D, globalTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	if(TQ)
		gluBuild2DMipmaps(GL_TEXTURE_2D,GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ,bigTexX, bigTexY, GL_RGBA, GL_UNSIGNED_BYTE, tex);
	else
		gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,bigTexX, bigTexY, GL_RGBA, GL_UNSIGNED_BYTE, tex);
//		CBitmap save(tex,bigTexX,bigTexY);
//		save.Save("unittex-1x.bmp");

	UnitTexture* t=new UnitTexture;
	t->xstart=0;
	t->ystart=0;
	t->xend=1;
	t->yend=1;
	textures[" "]=t;

	delete[] tex;
}

CTextureHandler::~CTextureHandler()
{
	map<string,UnitTexture*>::iterator tti;
	for(tti=textures.begin();tti!=textures.end();++tti){
		delete tti->second;
	}
	glDeleteTextures (1, &(globalTex));
}

CTextureHandler::UnitTexture* CTextureHandler::GetTexture(string name,int team,int teamTex)
{
	if(teamTex){
		char c[50];
		sprintf(c,"team%d_",team);
		name=string(c)+name;
	}

	std::transform(name.begin(), name.end(), name.begin(), (int (*)(int))std::tolower);

	std::map<std::string,UnitTexture*>::iterator tti;
	if((tti=textures.find(name))!=textures.end()){
		return tti->second;
	}
	(*info) << "Unknown texture " << name.c_str() << "\n";
	return textures[" "];
}

CTextureHandler::UnitTexture* CTextureHandler::GetTexture(string name)
{
	std::map<std::string,UnitTexture*>::iterator tti;
	if((tti=textures.find(name))!=textures.end()){
		return tti->second;
	}
	(*info) << "Unknown texture " << name.c_str() << "\n";
	return textures[" "];
}

void CTextureHandler::SetTexture()
{
	glBindTexture(GL_TEXTURE_2D, globalTex);
}
