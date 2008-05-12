#include "StdAfx.h"
#include "TextureAtlas.h"
#include "Rendering/Textures/Bitmap.h"
#include "FileSystem/FileHandler.h"
#include "LogOutput.h"
#include "Rendering/GL/myGL.h"
#include <list>

CR_BIND(AtlasedTexture, );
CR_BIND_DERIVED(GroundFXTexture, AtlasedTexture, );

//texture spacing in the atlas (in pixels)
#define TEXMARGIN 1

bool CTextureAtlas::debug;

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
	MemTex *memtex = SAFE_NEW MemTex;
	memtex->xsize = xsize;
	memtex->ysize = ysize;
	memtex->xpos = 0;
	memtex->ypos = 0;
	memtex->texType = texType;
	memtex->data = SAFE_NEW char[xsize*ysize*gpp/8];
	StringToLowerInPlace(name);
	memtex->names.push_back(name);
	memcpy(memtex->data, data, xsize*ysize*gpp/8);
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

			if(thisSub.front().y+curtex->ysize+TEXMARGIN<maxy){
				nextSub.push_back(int2(thisSub.front().x+TEXMARGIN,thisSub.front().y+curtex->ysize+TEXMARGIN));
			}

			thisSub.front().x+=curtex->xsize+TEXMARGIN;
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
		AtlasedTexture tex;
		//adjust texture coordinates by half a pixel to avoid filtering artifacts
		float halfx = 1/((float)xsize*2);
		float halfy = 1/((float)ysize*2);
		tex.xstart = memtextures[i]->xpos/(float)xsize + halfx;
		tex.xend = (memtextures[i]->xpos+memtextures[i]->xsize)/(float)xsize - halfx;
		tex.ystart = memtextures[i]->ypos/(float)ysize + halfy;
		tex.yend = (memtextures[i]->ypos+memtextures[i]->ysize)/(float)ysize - halfy;
		tex.ixstart = memtextures[i]->xpos;
		tex.iystart = memtextures[i]->ypos;
		for(int n=0; n<memtextures[i]->names.size(); n++)
			textures[memtextures[i]->names[n]] = tex;

		usedPixels += memtextures[i]->xpos*memtextures[i]->ypos;
		delete [] (char*)memtextures[i]->data;
		delete memtextures[i];
	}
	memtextures.clear();

	return success;
}

void CTextureAtlas::CreateTexture()
{

	unsigned char *data;
	data = SAFE_NEW unsigned char[xsize*ysize*4];
	memset(data,0,xsize*ysize*4); // make spacing between textures black transparent to avoid ugly lines with linear filtering

	for(int i=0; i<memtextures.size(); i++)
	{
		MemTex *tex = memtextures[i];
		for(int x=0; x<tex->xsize; x++) {
			for(int y=0; y<tex->ysize; y++) {
				((int*)data)[tex->xpos+x+(tex->ypos+y)*xsize] = ((int*)tex->data)[x+y*tex->xsize];
			}
		}			
	}

	if (debug) {
		// hack to make sure we don't overwrite our own atlases
		static int count = 0;
		char fname[256];
		SNPRINTF(fname, sizeof(fname), "textureatlas%d.png", ++count);
		CBitmap save(data,xsize,ysize);
		save.Save(fname);
		logOutput.Print("Saved finalized textureatlas to '%s'.", fname);
		//CBitmap save2(data[1],xsize>>1,ysize>>1);
		//save2.Save("textureatlas2.tga");
		//CBitmap save3(data[2],xsize>>2,ysize>>2);
		//save3.Save("textureatlas3.tga");
	}


	glGenTextures(1, &gltex);
	glBindTexture(GL_TEXTURE_2D, gltex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,/*GL_NEAREST_MIPMAP_LINEAR*/GL_NEAREST);
	if (GLEW_EXT_texture_edge_clamp) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	glBuildMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, data);

	delete [] data;

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

AtlasedTexture CTextureAtlas::GetTexture(const std::string& name)
{
	return textures[StringToLower(name)];
}

AtlasedTexture* CTextureAtlas::GetTexturePtr(const std::string& name)
{
	return &textures[StringToLower(name)];
}

bool CTextureAtlas::TextureExists(const std::string& name)
{
	return textures.find(StringToLower(name)) != textures.end();
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


