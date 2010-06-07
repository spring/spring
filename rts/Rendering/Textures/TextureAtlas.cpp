/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include <list>
#include <cstring>

#include "mmgr.h"

#include "TextureAtlas.h"
#include "Bitmap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/PBO.h"
#include "System/GlobalUnsynced.h"
#include "System/FileSystem/FileHandler.h"
#include "System/LogOutput.h"
#include "System/Util.h"
#include "System/Exceptions.h"
#include "System/Vec2.h"

CR_BIND(AtlasedTexture, );
CR_BIND_DERIVED(GroundFXTexture, AtlasedTexture, );

//texture spacing in the atlas (in pixels)
#define TEXMARGIN 2

bool CTextureAtlas::debug;

CTextureAtlas::CTextureAtlas(int maxxsize, int maxysize)
{
	this->maxxsize = maxxsize;
	this->maxysize = maxysize;
	xsize = 4;
	ysize = 4;
	usedPixels = 0;
	initialized = false;
	freeTexture = true;
}

CTextureAtlas::~CTextureAtlas(void)
{
	if(initialized && freeTexture)
		glDeleteTextures(1, &gltex);
}

void* CTextureAtlas::AddTex(std::string name, int xsize, int ysize, TextureType texType)
{
	int gpp = GetBPP(texType);
	MemTex* memtex = new MemTex;
	memtex->xsize = xsize;
	memtex->ysize = ysize;
	memtex->xpos = 0;
	memtex->ypos = 0;
	memtex->texType = texType;
	memtex->data = new char[xsize*ysize*gpp/8];
	StringToLowerInPlace(name);
	memtex->names.push_back(name);
	memtextures.push_back(memtex);

	return memtex->data;
}

int CTextureAtlas::AddTexFromMem(std::string name, int xsize, int ysize, TextureType texType, void  *data)
{
	int gpp = GetBPP(texType);
	MemTex* memtex = new MemTex;
	memtex->xsize = xsize;
	memtex->ysize = ysize;
	memtex->xpos = 0;
	memtex->ypos = 0;
	memtex->texType = texType;
	memtex->data = new char[xsize*ysize*gpp/8];
	StringToLowerInPlace(name);
	memtex->names.push_back(name);
	std::memcpy(memtex->data, data, xsize*ysize*gpp/8);
	memtextures.push_back(memtex);

	return 1;
}

int CTextureAtlas::AddTexFromFile(std::string name, std::string file)
{
	StringToLowerInPlace(name);

	//if the file already loaded, use that instead
	std::string lcfile = StringToLower(file);
	std::map<std::string, MemTex*>::iterator it = files.find(lcfile);

	if(it != files.end())
	{
		MemTex *memtex = it->second;
		memtex->names.push_back(name);
		return 1;
	}

	CBitmap bitmap;

	if (!bitmap.Load(file))
		throw content_error("Could not load texture from file " + file);

	if(bitmap.type != CBitmap::BitmapTypeStandardRGBA)  //only suport rgba for now
		throw content_error("Unsupported bitmap format in file " + file);

	int ret = AddTexFromMem(name, bitmap.xsize, bitmap.ysize, RGBA32, bitmap.mem);

	if (ret == 1)
		files[lcfile] = memtextures.back();

	return ret;
}

bool CTextureAtlas::Finalize()
{
	sort(memtextures.begin(), memtextures.end(), CTextureAtlas::CompareTex);

	bool success = true;
	int maxx=0;
	int curx=0;
	int maxy=0;
	int cury=0;
	std::list<int2> nextSub;
	std::list<int2> thisSub;
	bool recalc=false;
	for(int a=0; a < static_cast<int>(memtextures.size()); ++a) {
		MemTex* curtex = memtextures[a];

		bool done = false;
		while(!done) {
			if(thisSub.empty()) {
				if(nextSub.empty()) {
					maxx = std::max(maxx, curx);
					cury = maxy;
					maxy += curtex->ysize + TEXMARGIN;
					if(maxy > ysize) {
						if(IncreaseSize()){
 							nextSub.clear();
							thisSub.clear();
							cury = maxy = curx = 0;
							recalc = true;
							break;
						} else {
							success = false;
							break;
						}
					}
					thisSub.push_back(int2(0,cury));
				} else {
					thisSub = nextSub;
					nextSub.clear();
				}
			}

			if(thisSub.front().x + curtex->xsize > xsize) {
				thisSub.clear();
				continue;
			}
			if(thisSub.front().y + curtex->ysize > maxy) {
				thisSub.pop_front();
				continue;
			}

			//ok found space for us
			curtex->xpos = thisSub.front().x;
			curtex->ypos = thisSub.front().y;

			done = true;

			if(thisSub.front().y + curtex->ysize + TEXMARGIN < maxy) {
				nextSub.push_back(int2(thisSub.front().x + TEXMARGIN, thisSub.front().y + curtex->ysize + TEXMARGIN));
			}

			thisSub.front().x += curtex->xsize + TEXMARGIN;
			while(thisSub.size()>1 && thisSub.front().x >= (++thisSub.begin())->x) {
				(++thisSub.begin())->x = thisSub.front().x;
				thisSub.erase(thisSub.begin());
			}

		}
		if(recalc) {
			recalc=false;
			a=-1;
			continue;
		}
	}

	if (globalRendering->supportNPOTs && !debug) {
		maxx = std::max(maxx,curx);
		//xsize = maxx;
		ysize = maxy;
	}

	CreateTexture();
	for(std::vector<MemTex*>::iterator it = memtextures.begin(); it != memtextures.end(); it++)
	{
		AtlasedTexture tex;
		//adjust texture coordinates by half a pixel (in opengl pixel centers are centeriods)
		tex.xstart =                    ((*it)->xpos + 0.5f) / (float)(xsize);
		tex.xend   = (((*it)->xpos + (*it)->xsize-1) + 0.5f) / (float)(xsize);
		tex.ystart =                    ((*it)->ypos + 0.5f) / (float)(ysize);
		tex.yend   = (((*it)->ypos + (*it)->ysize-1) + 0.5f) / (float)(ysize);
		tex.ixstart = (*it)->xpos;
		tex.iystart = (*it)->ypos;
		for(size_t n=0; n<(*it)->names.size(); n++) {
			textures[(*it)->names[n]] = tex;
		}

		usedPixels += (*it)->xpos * (*it)->ypos;
		delete[] (char*)(*it)->data;
		delete (*it);
	}
	memtextures.clear();

	return success;
}

void CTextureAtlas::CreateTexture()
{
	PBO pbo;
	pbo.Bind();
	pbo.Resize(xsize*ysize*4);
	unsigned char* data = (unsigned char*)pbo.MapBuffer(debug ? GL_READ_WRITE : GL_WRITE_ONLY);
	std::memset(data,0,xsize*ysize*4); // make spacing between textures black transparent to avoid ugly lines with linear filtering

	for(size_t i=0; i<memtextures.size(); i++) {
		MemTex& tex = *memtextures[i];
		for(int y=0; y<tex.ysize; y++) {
			int* dst = ((int*)data) + tex.xpos + (tex.ypos + y) * xsize;
			int* src = ((int*)tex.data) + y * tex.xsize;
			memcpy(dst, src, tex.xsize*4);
		}
	}
	pbo.UnmapBuffer();

	if (debug) {
		// hack to make sure we don't overwrite our own atlases
		static int count = 0;
		char fname[256];
		SNPRINTF(fname, sizeof(fname), "textureatlas%d.png", ++count);
		CBitmap save(data,xsize,ysize);
		save.Save(fname);
		logOutput.Print("Saved finalized textureatlas to '%s'.", fname);
	}


	glGenTextures(1, &gltex);
	glBindTexture(GL_TEXTURE_2D, gltex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST/*GL_NEAREST_MIPMAP_LINEAR*/);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, xsize, ysize, 0, GL_RGBA, GL_UNSIGNED_BYTE, pbo.GetPtr());

	pbo.Unbind();

	initialized=true;
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

bool CTextureAtlas::TextureExists(const std::string& name)
{
	return textures.find(StringToLower(name)) != textures.end();
}


AtlasedTexture CTextureAtlas::GetTexture(const std::string& name)
{
	return textures[StringToLower(name)];
}

AtlasedTexture* CTextureAtlas::GetTexturePtr(const std::string& name)
{
	return &textures[StringToLower(name)];
}


AtlasedTexture CTextureAtlas::GetTextureWithBackup(const std::string& name,
                                                   const std::string& backupName)
{
	if (textures.find(StringToLower(name)) != textures.end()) {
		return textures[StringToLower(name)];
	} else {
		return textures[StringToLower(backupName)];
	}
}

AtlasedTexture* CTextureAtlas::GetTexturePtrWithBackup(const std::string& name,
                                                   const std::string& backupName)
{
	if (textures.find(StringToLower(name)) != textures.end()) {
		return &textures[StringToLower(name)];
	} else {
		return &textures[StringToLower(backupName)];
	}
}
