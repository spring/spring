/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include "mmgr.h"

#include "S3OTextureHandler.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/SimpleParser.h"
#include "LogOutput.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Textures/Bitmap.h"
#include "TAPalette.h"
#include "System/Util.h"
#include "System/Exceptions.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CS3OTextureHandler* texturehandlerS3O = 0;

CS3OTextureHandler::CS3OTextureHandler()
{
	s3oTextures.push_back(S3oTex());
	s3oTextures.push_back(S3oTex());
}

CS3OTextureHandler::~CS3OTextureHandler()
{
	while(s3oTextures.size()>1){
		glDeleteTextures (1, &s3oTextures.back().tex1);
		glDeleteTextures (1, &s3oTextures.back().tex2);
		s3oTextures.pop_back();
	}
}

void CS3OTextureHandler::LoadS3OTexture(S3DModel* model) {
#if defined(USE_GML) && GML_ENABLE_SIM
	model->textureType=-1;
#else
	model->textureType = LoadS3OTextureNow(model);
#endif
}

void CS3OTextureHandler::Update() {
}

int CS3OTextureHandler::LoadS3OTextureNow(const S3DModel* model)
{
	GML_STDMUTEX_LOCK(model); // LoadS3OTextureNow

	string totalName = model->tex1 + model->tex2;

	if(s3oTextureNames.find(totalName)!=s3oTextureNames.end()){
		return s3oTextureNames[totalName];
	}
	int newNum=s3oTextures.size();
	S3oTex tex;
	tex.num=newNum;

	CBitmap bm;
	if (!bm.Load(std::string("unittextures/" + model->tex1))) {
		throw content_error("Could not load texture unittextures/" + model->tex1 + " from S3O model " + model->name);
	}
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
		if (!bm.Load(std::string("unittextures/" + model->tex2))) {
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

void CS3OTextureHandler::SetS3oTexture(int num)
{
	if (shadowHandler->inShadowPass) {
		glBindTexture(GL_TEXTURE_2D, s3oTextures[num].tex2);
	} else {
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_2D, s3oTextures[num].tex1);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, s3oTextures[num].tex2);
	}
}
