// TextureHandler.cpp: implementation of the CTextureHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include "mmgr.h"

#include "TextureHandler.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/SimpleParser.h"
#include "Game/Team.h"
#include "LogOutput.h"
#include "Platform/ConfigHandler.h"
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

CTextureHandler* texturehandler=0;

struct TexFile {
	CBitmap tex;
	std::string name;
};

static int CompareTatex2( const void *arg1, const void *arg2 ){
	if((*(TexFile**)arg1)->tex.ysize > (*(TexFile**)arg2)->tex.ysize)
	   return -1;
   return 1;
}

CTextureHandler::CTextureHandler()
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

	std::vector<std::string> files2 = CFileHandler::FindFiles("unittextures/tatex/", "*.bmp");
	std::vector<std::string> files = CFileHandler::FindFiles("unittextures/tatex/", "*.tga");

	for(std::vector<std::string>::iterator fi = files2.begin(); fi != files2.end(); ++fi) {
		files.push_back(*fi);
	}

	std::set<string> usedNames;

	for(std::vector<std::string>::iterator fi = files.begin(); fi != files.end(); ++fi) {
		std::string s = std::string(*fi);
		std::string s2 = s;

		s2.erase(0, s2.find_last_of('/') + 1);
		s2 = StringToLower(s2.substr(0, s2.find_last_of('.')));

		if(usedNames.find(s2)!=usedNames.end()) //avoid duplicate names and give tga images priority
			continue;
		usedNames.insert(s2);

		if(teamTexes.find(s2) == teamTexes.end()){
			TexFile* tex = SAFE_NEW TexFile;
			tex->tex.Load(s,30);
			tex->name = s2;
			texfiles[numfiles++] = tex;
			totalSize += tex->tex.xsize * tex->tex.ysize;
		} else {
			for(int a = 0; a < gs->activeTeams; ++a){
				TexFile* tex = CreateTeamTex(s, s2, a);
				texfiles[numfiles++] = tex;
				totalSize += tex->tex.xsize * tex->tex.ysize;
			}
		}
	}

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
		handleerror(0,"Too many/large unit textures to fit in 2048*2048","Error",0);
	}

	qsort(texfiles,numfiles,sizeof(TexFile*),CompareTatex2);

	unsigned char* tex=SAFE_NEW unsigned char[bigTexX*bigTexY*4];
	for(int a=0;a<bigTexX*bigTexY*4;++a){
		tex[a]=128;
	}

	int cury=0;
	int maxy=0;
	int curx=0;
	int foundx = 0, foundy = 0;
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
						handleerror(0,"Too many/large unit textures","Error",0);
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

		UnitTexture* unittex=SAFE_NEW UnitTexture;

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
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR/*_MIPMAP_NEAREST*/);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8 ,bigTexX, bigTexY, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
	//glBuildMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,bigTexX, bigTexY, GL_RGBA, GL_UNSIGNED_BYTE, tex);
//	CBitmap save(tex,bigTexX,bigTexY);
	//save.Save("unittex-1x.jpg");

	UnitTexture* t=SAFE_NEW UnitTexture;
	t->xstart=0;
	t->ystart=0;
	t->xend=1;
	t->yend=1;

	s3oTextures.push_back(S3oTex());
	textures[" "]=t;

	delete[] tex;

	s3oTextures.push_back(S3oTex());
}

CTextureHandler::~CTextureHandler()
{
	std::map<string,UnitTexture*>::iterator tti;
	for(tti=textures.begin();tti!=textures.end();++tti){
		delete tti->second;
	}
	while(s3oTextures.size()>1){
		glDeleteTextures (1, &s3oTextures.back().tex1);
		glDeleteTextures (1, &s3oTextures.back().tex2);
		s3oTextures.pop_back();
	}
	glDeleteTextures (1, &(globalTex));
}

CTextureHandler::UnitTexture* CTextureHandler::GetTATexture(std::string name, int team, int teamTex)
{
	if(teamTex){
		char c[50];
		sprintf(c,"team%d_",team);
		name=string(c)+name;
	}

	StringToLowerInPlace(name);

	std::map<std::string,UnitTexture*>::iterator tti;
	if((tti=textures.find(name))!=textures.end()){
		return tti->second;
	}
	logOutput << "Unknown texture " << name.c_str() << "\n";
	return textures[" "];
}

CTextureHandler::UnitTexture* CTextureHandler::GetTATexture(const std::string& name)
{
	std::map<std::string,UnitTexture*>::iterator tti;
	if((tti=textures.find(name))!=textures.end()){

		return tti->second;
	}
	logOutput << "Unknown texture " << name.c_str() << "\n";
	return textures[" "];
}

void CTextureHandler::SetTATexture()
{
	glBindTexture(GL_TEXTURE_2D, globalTex);
}

void CTextureHandler::LoadS3OTexture(S3DOModel *model) {
	string totalName=model->tex1+model->tex2;

	if(s3oTextureNames.find(totalName)!=s3oTextureNames.end()){
		model->textureType=s3oTextureNames[totalName];
	}
	model->textureType=0;
	GML_STDMUTEX_LOCK(model); // LoadS3OTexture
	loadTextures.push_back(model);
}

void CTextureHandler::Update() {
	GML_STDMUTEX_LOCK(model); // Update
	for(std::vector<S3DOModel *>::iterator i=loadTextures.begin(); i!=loadTextures.end();++i)
		(*i)->textureType=LoadS3OTextureNow((char*)(*i)->tex1.c_str(),(char*)(*i)->tex2.c_str());
	loadTextures.clear();
}

int CTextureHandler::LoadS3OTextureNow(const std::string& tex1, const std::string& tex2)
{
	string totalName=tex1+tex2;

	if(s3oTextureNames.find(totalName)!=s3oTextureNames.end()){
		return s3oTextureNames[totalName];
	}
	int newNum=s3oTextures.size();
	S3oTex tex;
	tex.num=newNum;

	CBitmap bm;
	if (!bm.Load(string("unittextures/"+tex1)))
		throw content_error("Could not load S3O texture from file unittextures/" + tex1);
	tex.tex1 = bm.CreateTexture(true);
	tex.tex1SizeX = bm.xsize;
	tex.tex1SizeY = bm.ysize;
	tex.tex2=0;
	tex.tex2SizeX = 0;
	tex.tex2SizeY = 0;
	//if(unitDrawer->advShading)
{
		CBitmap bm;
		// No error checking here... other code relies on an empty texture
		// being generated if it couldn't be loaded.
		// Also many map features specify a tex2 but don't ship it with the map,
		// so throwing here would cause maps to break.
		if(!bm.Load(string("unittextures/"+tex2))) {
			bm.Alloc(1,1);
			bm.mem[3] = 255;//file not found, set alpha to white so unit is visible
		}
		tex.tex2 = bm.CreateTexture(true);
		tex.tex2SizeX = bm.xsize;
		tex.tex2SizeY = bm.ysize;
}
	s3oTextures.push_back(tex);
	s3oTextureNames[totalName]=newNum;
	return newNum;
}

void CTextureHandler::SetS3oTexture(int num)
{
	if (shadowHandler->inShadowPass) {
		glBindTexture(GL_TEXTURE_2D, s3oTextures[num].tex2);
	} else {
		glBindTexture(GL_TEXTURE_2D, s3oTextures[num].tex1);
		//if(unitDrawer->advShading){
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, s3oTextures[num].tex2);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		//}
	}
}

TexFile* CTextureHandler::CreateTeamTex(const std::string& name, const std::string& name2, int team)
{
	TexFile* tex=SAFE_NEW TexFile;
	tex->tex.Load(name,30);
	char tmp[256];
	sprintf(tmp,"%s%02i",name2.c_str(),team);
	tex->name=tmp;

	unsigned char* teamCol=gs->Team(team)->color;
	CBitmap* bm=&tex->tex;
	for(int a=0;a<bm->ysize*bm->xsize;++a){
		if(bm->mem[a*4]==bm->mem[a*4+2] && bm->mem[a*4+1]==0){
			float lum=bm->mem[a*4]/255.0f;
			bm->mem[a*4+0]=(unsigned char)(std::min(255,int(teamCol[0]*lum*1.5f)));
			bm->mem[a*4+1]=(unsigned char)(std::min(255,int(teamCol[1]*lum*1.5f)));
			bm->mem[a*4+2]=(unsigned char)(std::min(255,int(teamCol[2]*lum*1.5f)));
		}
	}
	return tex;
}

