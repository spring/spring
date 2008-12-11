// 3DOTextureHandler.cpp: implementation of the C3DOTextureHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include "mmgr.h"

#include "3DOTextureHandler.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/SimpleParser.h"
#include "LogOutput.h"
#include "Platform/errorhandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "TAPalette.h"
#include "System/Util.h"
#include "System/Exceptions.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

C3DOTextureHandler* texturehandler3DO = 0;

struct TexFile {
	CBitmap tex;  // same format
	CBitmap tex2; // as s3o's
	std::string name;
};

static int CompareTatex2( const void *arg1, const void *arg2 ){
	if((*(TexFile**)arg1)->tex.ysize > (*(TexFile**)arg2)->tex.ysize)
		return -1;
	return 1;
}

C3DOTextureHandler::C3DOTextureHandler()
{
	PrintLoadMsg("Creating unit textures");

	CFileHandler file("unittextures/tatex/teamtex.txt");
	CSimpleParser parser(file);

	std::set<std::string> teamTexes;
	while(!file.Eof()) {
		teamTexes.insert(StringToLower(parser.GetCleanLine()));
	}

	TexFile* texfiles[10000];

	int numfiles = 0;
	int totalSize = 0;

	std::vector<std::string> filesBMP = CFileHandler::FindFiles("unittextures/tatex/", "*.bmp");
	std::vector<std::string> files    = CFileHandler::FindFiles("unittextures/tatex/", "*.tga");
	files.insert(files.end(),filesBMP.begin(),filesBMP.end());

	std::set<string> usedNames;
	for(std::vector<std::string>::iterator fi = files.begin(); fi != files.end(); ++fi) {
		std::string s = std::string(*fi);
		std::string s2 = s;

		s2.erase(0, s2.find_last_of('/') + 1);
		s2 = StringToLower(s2.substr(0, s2.find_last_of('.')));

		//avoid duplicate names and give tga images priority
		if(usedNames.find(s2)!=usedNames.end())
			continue;
		usedNames.insert(s2);

		if(teamTexes.find(s2) == teamTexes.end()){
			TexFile* tex = CreateTex(s, s2, false);
			texfiles[numfiles++] = tex;
			totalSize += tex->tex.xsize * tex->tex.ysize;
		} else {
			TexFile* tex = CreateTex(s, s2, true);
			texfiles[numfiles++] = tex;
			totalSize += tex->tex.xsize * tex->tex.ysize;
		}
	}

	// "TAPalette.h"
	for(int a=0;a<256;++a){
		string name="ta_color";
		char t[50];
		sprintf(t,"%i",a);
		name+=t;
		TexFile* tex=SAFE_NEW TexFile;
		tex->name=name;
		tex->tex.Alloc(1,1);
		tex->tex.mem[0]=palette[a][0];
		tex->tex.mem[1]=palette[a][1];
		tex->tex.mem[2]=palette[a][2];
		tex->tex.mem[3]=0;   //teamcolor

		tex->tex2.Alloc(1,1);
		tex->tex2.mem[0]=0;  //self illum
		tex->tex2.mem[1]=30; //reflectivity
		tex->tex2.mem[2]=0;
		tex->tex2.mem[3]=255;

		texfiles[numfiles++]=tex;
		totalSize+=tex->tex.xsize*tex->tex.ysize;
	}

	//pessimistic guess about how much space will be wasted
	totalSize=(int)(totalSize * 1.2f);

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
		handleerror(0,"Too many/large unit textures to fit in 2048*2048","Error",0);
	}

	qsort(texfiles,numfiles,sizeof(TexFile*),CompareTatex2);

	unsigned char* bigtex1 = SAFE_NEW unsigned char[bigTexX*bigTexY*4];
	unsigned char* bigtex2 = SAFE_NEW unsigned char[bigTexX*bigTexY*4];
	for(int a=0;a<bigTexX*bigTexY;++a) {
		bigtex1[a*4+0] = 128;
		bigtex1[a*4+1] = 128;
		bigtex1[a*4+2] = 128;
		bigtex1[a*4+3] = 0;

		bigtex2[a*4+0] = 0;
		bigtex2[a*4+1] = 128;
		bigtex2[a*4+2] = 0;
		bigtex2[a*4+3] = 255;
	}

	int cury=0;
	int maxy=0;
	int curx=0;
	int foundx = 0, foundy = 0;
	std::list<int2> nextSub;
	std::list<int2> thisSub;
	for(int a=0;a<numfiles;++a){
		CBitmap* curtex1=&texfiles[a]->tex;
		CBitmap* curtex2=&texfiles[a]->tex2;

		bool done=false;
		while(!done){
			// Find space for us
			if(thisSub.empty()){
				if(nextSub.empty()){
					cury=maxy;
					maxy+=curtex1->ysize;
					if(maxy>bigTexY){
						handleerror(0,"Too many/large unit textures","Error",0);
						break;
					}
					thisSub.push_back(int2(0,cury));
				} else {
					thisSub=nextSub;
					nextSub.clear();
				}
			}
			if(thisSub.front().x+curtex1->xsize>bigTexX){
				thisSub.clear();
				continue;
			}
			if(thisSub.front().y+curtex1->ysize>maxy){
				thisSub.pop_front();
				continue;
			}
			// ok found space for us
			foundx=thisSub.front().x;
			foundy=thisSub.front().y;
			done=true;

			if(thisSub.front().y+curtex1->ysize<maxy){
				nextSub.push_back(int2(thisSub.front().x,thisSub.front().y+curtex1->ysize));
			}

			thisSub.front().x+=curtex1->xsize;
			while(thisSub.size()>1 && thisSub.front().x >= (++thisSub.begin())->x){
				(++thisSub.begin())->x=thisSub.front().x;
				thisSub.erase(thisSub.begin());
			}

		}
		for(int y=0;y<curtex1->ysize;y++){
			for(int x=0;x<curtex1->xsize;x++){
//				if(curtex1->mem[(y*curtex1->xsize+x)*4]==254 && curtex1->mem[(y*curtex1->xsize+x)*4+1]==0 && curtex1->mem[(y*curtex1->xsize+x)*4+2]==254){
//					bigtex1[((cury+y)*bigTexX+(curx+x))*4+3] = 0;
//				} else {
					for(int col=0;col<4;col++){
						bigtex1[((foundy+y)*bigTexX+(foundx+x))*4+col] = curtex1->mem[(y*curtex1->xsize+x)*4+col];
						bigtex2[((foundy+y)*bigTexX+(foundx+x))*4+col] = curtex2->mem[(y*curtex1->xsize+x)*4+col];
//					}
				}
			}
		}

		UnitTexture* unittex=SAFE_NEW UnitTexture;

		unittex->xstart=(foundx+0.5f)/(float)bigTexX;
		unittex->ystart=(foundy+0.5f)/(float)bigTexY;
		unittex->xend=(foundx+curtex1->xsize-0.5f)/(float)bigTexX;
		unittex->yend=(foundy+curtex1->ysize-0.5f)/(float)bigTexY;
		textures[texfiles[a]->name]=unittex;

		curx+=curtex1->xsize;
		delete texfiles[a];
	}

	glGenTextures(1, &atlas3do1);
	glBindTexture(GL_TEXTURE_2D, atlas3do1);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR/*_MIPMAP_NEAREST*/);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8 ,bigTexX, bigTexY, 0, GL_RGBA, GL_UNSIGNED_BYTE, bigtex1);
	//glBuildMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,bigTexX, bigTexY, GL_RGBA, GL_UNSIGNED_BYTE, bigtex1);

	glGenTextures(1, &atlas3do2);
	glBindTexture(GL_TEXTURE_2D, atlas3do2);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR/*_MIPMAP_NEAREST*/);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8 ,bigTexX, bigTexY, 0, GL_RGBA, GL_UNSIGNED_BYTE, bigtex2);
	//glBuildMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,bigTexX, bigTexY, GL_RGBA, GL_UNSIGNED_BYTE, bigtex2);


//	CBitmap save(tex,bigTexX,bigTexY);
//	save.Save("unittex-1x.jpg");

	UnitTexture* t=SAFE_NEW UnitTexture;
	t->xstart=0;
	t->ystart=0;
	t->xend=1;
	t->yend=1;
	textures[" "]=t;

	delete[] bigtex1;
	delete[] bigtex2;
}

C3DOTextureHandler::~C3DOTextureHandler()
{
	std::map<string,UnitTexture*>::iterator tti;
	for(tti=textures.begin();tti!=textures.end();++tti){
		delete tti->second;
	}
	glDeleteTextures(1, &atlas3do1);
	glDeleteTextures(1, &atlas3do2);
}

C3DOTextureHandler::UnitTexture* C3DOTextureHandler::Get3DOTexture(std::string name)
{
	StringToLowerInPlace(name);
	std::map<std::string,UnitTexture*>::iterator tti;
	if((tti=textures.find(name))!=textures.end()){
		return tti->second;
	}
	logOutput << "Unknown texture " << name.c_str() << "\n";
	return textures[" "];
}

void C3DOTextureHandler::Set3doAtlases() const
{
	if(unitDrawer->advShading){
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, atlas3do2);
		glActiveTexture(GL_TEXTURE0);
	}
	glBindTexture(GL_TEXTURE_2D, atlas3do1);
}

TexFile* C3DOTextureHandler::CreateTex(const std::string& name, const std::string& name2, bool teamcolor)
{
	TexFile* tex=SAFE_NEW TexFile;
	tex->tex.Load(name,30);
	char tmp[256];
	tex->name = name2;
	//sprintf(tmp,"%s%02i",name2.c_str(),team);
	//tex->name=tmp;

	tex->tex2.Alloc(tex->tex.xsize,tex->tex.ysize);

	CBitmap* tex1 = &tex->tex;
	CBitmap* tex2 = &tex->tex2;

	for(int a=0;a<tex1->ysize*tex1->xsize;++a){
		tex2->mem[a*4+0] = 0;
		tex2->mem[a*4+1] = tex1->mem[a*4+3]; //move reflectivity to texture2
		tex2->mem[a*4+2] = 0;
		tex2->mem[a*4+3] = 255;

		tex1->mem[a*4+3] = 0;

		if (teamcolor) {
			//purple = teamcolor
			if(tex1->mem[a*4]==tex1->mem[a*4+2] && tex1->mem[a*4+1]==0){
				unsigned char lum = tex1->mem[a*4];
				tex1->mem[a*4+0] = 0;
				tex1->mem[a*4+1] = 0;
				tex1->mem[a*4+2] = 0;
				tex1->mem[a*4+3] = (unsigned char)(std::min(255.0f,lum*1.5f));
			}
		}
	}
	return tex;
}

