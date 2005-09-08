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
#include "ConfigHandler.h"
#include <set>
#include "errorhandler.h"
//#include "mmgr.h"

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

	std::vector<std::string> files2=CFileHandler::FindFiles("unittextures/tatex/*.bmp");
	std::vector<std::string> files=CFileHandler::FindFiles("unittextures/tatex/*.tga");

	for(std::vector<string>::iterator fi=files2.begin();fi!=files2.end();++fi){
		files.push_back(*fi);
	}

	set<string> usedNames;

	for(vector<string>::iterator fi=files.begin();fi!=files.end();++fi){
		string s=std::string(*fi);

		string s2=s;
		s2.erase(0,s2.find_last_of('/')+1);
		s2=s2.substr(0,s2.find_last_of('.'));
		std::transform(s2.begin(), s2.end(), s2.begin(), (int (*)(int))std::tolower);		

		if(usedNames.find(s2)!=usedNames.end())		//avoid duplicate names and give tga images priority
			continue;
		usedNames.insert(s2);

		TexFile* tex=new TexFile;
		tex->tex.Load(s,30);

		tex->name=s2;
	
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
		tex->tex.mem[3]=30;

		texfiles[numfiles++]=tex;
		totalSize+=tex->tex.xsize*tex->tex.ysize;
	}
	totalSize=(int)(totalSize * 1.2f);		//pessimistic guess about how much space will be wasted

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
		handleerror(0,"To many/large unit textures to fit in 2048*2048","Error",0);
	}

	qsort(texfiles,numfiles,sizeof(TexFile*),CompareTatex2);

	unsigned char* tex=new unsigned char[bigTexX*bigTexY*4];    
	for(int a=0;a<bigTexX*bigTexY*4;++a){
		tex[a]=128;
	}

	int cury=0;
	int maxy=0;
	int curx=0;
	int foundx,foundy;
	std::list<int2> nextSub;
	std::list<int2> thisSub;
	for(int a=0;a<numfiles;++a){
		CBitmap* curtex=&texfiles[a]->tex;

		bool done=false;
		while(!done){
			if(thisSub.empty()){
				if(nextSub.empty()){
					cury=maxy;
					maxy+=curtex->ysize;
					if(maxy>bigTexY){
						handleerror(0,"To many/large unit textures","Error",0);
						break;
					}
					thisSub.push_back(int2(0,cury));
				} else {
					thisSub=nextSub;
					nextSub.clear();
				}
			}
			if(thisSub.front().x+curtex->xsize>bigTexX){
				thisSub.clear();
				continue;
			}
			if(thisSub.front().y+curtex->ysize>maxy){
				thisSub.pop_front();
				continue;
			}
			//ok found space for us
			foundx=thisSub.front().x;
			foundy=thisSub.front().y;
			done=true;

			if(thisSub.front().y+curtex->ysize<maxy){
				nextSub.push_back(int2(thisSub.front().x,thisSub.front().y+curtex->ysize));
			}

			thisSub.front().x+=curtex->xsize;
			while(thisSub.size()>1 && thisSub.front().x >= (++thisSub.begin())->x){
				(++thisSub.begin())->x=thisSub.front().x;
				thisSub.erase(thisSub.begin());
			}

		}
		for(int y=0;y<curtex->ysize;y++){
			for(int x=0;x<curtex->xsize;x++){
//				if(curtex->mem[(y*curtex->xsize+x)*4]==254 && curtex->mem[(y*curtex->xsize+x)*4+1]==0 && curtex->mem[(y*curtex->xsize+x)*4+2]==254){
//					tex[((cury+y)*bigTexX+(curx+x))*4+3]=0;
//				} else {
					for(int col=0;col<4;col++){
						tex[((foundy+y)*bigTexX+(foundx+x))*4+col]=curtex->mem[(y*curtex->xsize+x)*4+col];
//					}
				}
			}
		}

		UnitTexture* unittex=new UnitTexture;

		unittex->xstart=(foundx+0.5f)/(float)bigTexX;
		unittex->ystart=(foundy+0.5f)/(float)bigTexY;
		unittex->xend=(foundx+curtex->xsize-0.5f)/(float)bigTexX;
		unittex->yend=(foundy+curtex->ysize-0.5f)/(float)bigTexY;
		textures[texfiles[a]->name]=unittex;
		
		curx+=curtex->xsize;
		delete texfiles[a];

	}

	glGenTextures(1, &globalTex);
	glBindTexture(GL_TEXTURE_2D, globalTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,bigTexX, bigTexY, GL_RGBA, GL_UNSIGNED_BYTE, tex);
//	CBitmap save(tex,bigTexX,bigTexY);
	//save.Save("unittex-1x.jpg");

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
