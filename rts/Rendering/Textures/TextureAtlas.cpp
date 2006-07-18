#include "StdAfx.h"
#include ".\textureatlas.h"
#include "Rendering/Textures/Bitmap.h"
#include "FileSystem/FileHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Rendering/GL/myGL.h"
#include <GL/glu.h>

CTextureAtlas::CTextureAtlas(int maxxsize, int maxysize)
{
	this->maxxsize = maxxsize;
	this->maxysize = maxysize;
	xsize = 4;
	ysize = 4;
	usedPixels=0;
	initialized=false;
}

CTextureAtlas::~CTextureAtlas(void)
{
	if(initialized)
		glDeleteTextures(1, &gltex);
}

int CTextureAtlas::AddTexFromMem(std::string name, int xsize, int ysize, TextureType texType, void  *data)
{
	int gpp = GetBPP(texType);
	MemTex *memtex = new MemTex;
	memtex->xsize = xsize;
	memtex->ysize = ysize;
	memtex->xpos = 0;
	memtex->ypos = 0;
	memtex->texType = texType;
	memtex->data = new char[xsize*ysize*gpp/8];
	memtex->name = name;
	memcpy(memtex->data, data, xsize*ysize*gpp/8);
	memtextures.push_back(memtex);

	return 1;
}

int CTextureAtlas::AddTexFromFile(std::string name, std::string file)
{
	CBitmap bitmap(file);
	if(bitmap.type != CBitmap::BitmapTypeStandardRGBA)  //only suport rgba for now
	{
		info->AddLine ("Unsoported bitmap format in file " + file);
		return -1;
	}

	return AddTexFromMem(name, bitmap.xsize, bitmap.ysize, RGBA32, bitmap.mem);
}

bool CTextureAtlas::Finalize()
{
	sort(memtextures.begin(), memtextures.end(), CTextureAtlas::CompareTex);

	bool success = true;
	int cury=0;
	int maxy=0;
	int curx=0;
	std::list<int2> nextSub;
	std::list<int2> thisSub;
	bool recalc=false;
	for(int a=0;a<memtextures.size();++a){
		MemTex *curtex = memtextures[a];

		bool done=false;
		while(!done){
			if(thisSub.empty()){
				if(nextSub.empty()){
					cury=maxy;
					maxy+=curtex->ysize;
					if(maxy>ysize){
						if(IncreaseSize())
						{
                            nextSub.clear();
							thisSub.clear();
							cury=maxy=curx=0;
							recalc=true;
							break;
						}
						else
						{
							success = false;
							break;
						}
					}
					thisSub.push_back(int2(0,cury));
				} else {
					thisSub=nextSub;
					nextSub.clear();
				}
			}
			if(thisSub.front().x+curtex->xsize>xsize){
				thisSub.clear();
				continue;
			}
			if(thisSub.front().y+curtex->ysize>maxy){
				thisSub.pop_front();
				continue;
			}
			//ok found space for us
			curtex->xpos=thisSub.front().x;
			curtex->ypos=thisSub.front().y;

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
		if(recalc)
		{
			recalc=false;
			a=-1;
			continue;
		}
	}

	CreateTexture();
	for(int i=0; i<memtextures.size(); i++)
	{
		Texture tex;
		//ajust textur coordinates by half a pixel to avoid filtering artifacts
		float halfx = 1/((float)xsize*2);
		float halfy = 1/((float)ysize*2);
		tex.xstart = memtextures[i]->xpos/(float)xsize + halfx;
		tex.xend = (memtextures[i]->xpos+memtextures[i]->xsize)/(float)xsize - halfx;
		tex.ystart = memtextures[i]->ypos/(float)ysize + halfy;
		tex.yend = (memtextures[i]->ypos+memtextures[i]->ysize)/(float)ysize - halfy;
		tex.ixstart = memtextures[i]->xpos;
		tex.iystart = memtextures[i]->ypos;
		textures[memtextures[i]->name] = tex;

		usedPixels += memtextures[i]->xpos*memtextures[i]->ypos;
		delete [] memtextures[i]->data;
		delete memtextures[i];
	}
	memtextures.clear();

	return success;
}

void CTextureAtlas::CreateTexture()
{
	unsigned char *data = new unsigned char[xsize*ysize*4];
	for(int i=0; i<memtextures.size(); i++)
	{
		MemTex *tex = memtextures[i];
        for(int x=0; x<tex->xsize; x++)
			for(int y=0; y<tex->ysize; y++)
			{
				((int*)data)[tex->xpos+x+(tex->ypos+y)*xsize] = ((int*)tex->data)[x+y*tex->xsize];
			}
	}

	glGenTextures(1, &gltex);
	glBindTexture(GL_TEXTURE_2D, gltex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR/*_MIPMAP_NEAREST*/);
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, data);
	//CBitmap save(data,xsize,ysize);
	//save.Save("textureatlas.tga");

	initialized=true;

	delete [] data;
}

int CTextureAtlas::GetBPP(TextureType texType)
{
	switch(texType)
	{
	case RGBA32:
		return 32;
	default:
		return 32;
	}
}

int CTextureAtlas::CompareTex(MemTex *tex1, MemTex *tex2)
{
	//sort in reverse order
	if(tex1->ysize == tex2->ysize)
		return tex1->xsize > tex2->xsize;
	return tex1->ysize > tex2->ysize;
}

bool CTextureAtlas::IncreaseSize()
{
	if(ysize<xsize)
	{
		if(ysize<maxysize)
		{
			ysize*=2;
			return true;
		}
		else
		{
			if(xsize<maxxsize)
			{
				xsize*=2;
				return true;
			}
		}
	}
	else
	{
		if(xsize<maxxsize)
		{
			xsize*=2;
			return true;
		}
		else
		{
			if(ysize<maxysize)
			{
				ysize*=2;
				return true;
			}
		}
	}
	return false;
}

void CTextureAtlas::BindTexture()
{
	glBindTexture(GL_TEXTURE_2D, gltex);
}

CTextureAtlas::Texture CTextureAtlas::GetTexture(std::string name)
{
	return textures[name];
}