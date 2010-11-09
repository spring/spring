/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "BasicTreeDrawer.h"
#include "Game/Camera.h"
#include "Lua/LuaParser.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/LogOutput.h"
#include "System/Exceptions.h"
#include "System/GlobalUnsynced.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static const float MAX_TREE_HEIGHT_25=MAX_TREE_HEIGHT*0.25f;
static const float MAX_TREE_HEIGHT_3=MAX_TREE_HEIGHT*0.3f;
static const float MAX_TREE_HEIGHT_36=MAX_TREE_HEIGHT*0.36f;
static const float MAX_TREE_HEIGHT_6=MAX_TREE_HEIGHT*0.6f;

CBasicTreeDrawer::CBasicTreeDrawer(): CBaseTreeDrawer()
{
	lastListClean = 0;

	LuaParser resourcesParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!resourcesParser.Execute()) {
		logOutput.Print(resourcesParser.GetErrorLog());
	}

	const LuaTable treesTable = resourcesParser.GetRoot().SubTable("graphics").SubTable("trees");
	const float tintc[3] = {0.6f,0.7f,0.6f};

	std::string fn;
	CBitmap sprite;
	CBitmap TexImage;
	TexImage.Alloc(512,512);

	{
		fn = "bitmaps/"+treesTable.GetString("gran1", "gran.bmp");

		if (!sprite.Load(fn))
			throw content_error("Could not load tree texture from " + fn);
		if (sprite.xsize != 256 || sprite.ysize != 256)
			throw content_error("texture " + fn + " must be 256x256!");
		sprite.ReverseYAxis();
		sprite.SetTransparent(SColor(72,72,72), SColor(33,54,29,0));
		TexImage.CopySubImage(sprite, 0, 0);
	}

	{
		fn = "bitmaps/"+treesTable.GetString("gran2", "gran2.bmp");
		if (!sprite.Load(fn))
			throw content_error("Could not load tree texture from file " + fn);
		if (sprite.xsize != 256 && sprite.ysize != 256)
			throw content_error("texture " + fn + " must be 256x256!");
		sprite.ReverseYAxis();
		sprite.SetTransparent(SColor(72,72,72), SColor(33,54,29,0));
		TexImage.CopySubImage(sprite, 255, 0);
	}

	{
		fn = "bitmaps/"+treesTable.GetString("birch1", "birch1.bmp");
		if (!sprite.Load(fn))
			throw content_error("Could not load tree texture from file " + fn);
		if (sprite.xsize != 128 || sprite.ysize != 256)
			throw content_error("texture " + fn + " must be 128x256!");
		sprite.ReverseYAxis();
		sprite.SetTransparent(SColor(72,72,72), SColor(75,102,49,0));
		sprite.Tint(tintc);
		TexImage.CopySubImage(sprite, 0, 255);
	}

	{
		fn = "bitmaps/"+treesTable.GetString("birch2", "birch2.bmp");
		if (!sprite.Load(fn))
			throw content_error("Could not load tree texture from file " + fn);
		if (sprite.xsize != 128 || sprite.ysize != 256)
			throw content_error("texture " + fn + " must be 128x256!");
		sprite.ReverseYAxis();
		sprite.SetTransparent(SColor(72,72,72), SColor(75,102,49,0));
		sprite.Tint(tintc);
		TexImage.CopySubImage(sprite, 127, 255);
	}

	{
		fn = "bitmaps/"+treesTable.GetString("birch3", "birch3.bmp");
		if (!sprite.Load(fn))
			throw content_error("Could not load tree texture from file " + fn);
		if (sprite.xsize != 256 || sprite.ysize != 256)
			throw content_error("texture " + fn + " must be 256x256!");
		sprite.ReverseYAxis();
		sprite.SetTransparent(SColor(72,72,72), SColor(75,102,49,0));
		sprite.Tint(tintc);
		TexImage.CopySubImage(sprite, 255, 255);
	}

	// create mipmapped texture
	treetex = TexImage.CreateTexture(true);

	treesX = gs->mapx / TREE_SQUARE_SIZE;
	treesY = gs->mapy / TREE_SQUARE_SIZE;
	nTrees = treesX * treesY;
	trees = new TreeSquareStruct[nTrees];

	for (TreeSquareStruct* pTSS = trees; pTSS < trees + nTrees; ++pTSS) {
		pTSS->displist = 0;
		pTSS->farDisplist = 0;
	}
}

CBasicTreeDrawer::~CBasicTreeDrawer()
{
	glDeleteTextures (1, &treetex);

	for(TreeSquareStruct* pTSS=trees; pTSS<trees+nTrees; ++pTSS) {
		if(pTSS->displist)
			glDeleteLists(pTSS->displist,1);
		if(pTSS->farDisplist)
			glDeleteLists(pTSS->farDisplist,1);
	}

	delete[] trees;
}

static CVertexArray* va;

static void inline SetArrayQ(float t1,float t2,float3 v)
{
	va->AddVertexQT(v,t1,t2);
}

struct CBasicTreeSquareDrawer : CReadMap::IQuadDrawer
{
	CBasicTreeSquareDrawer(CBasicTreeDrawer* td_, int cx_, int cy_, float treeDistance_)
		: td(td_),
		cx(cx_),
		cy(cy_),
		treeDistance(treeDistance_)
	{}

	void DrawQuad(int x, int y);

	CBasicTreeDrawer* td;
	int cx,cy;
	float treeDistance;
};

inline void DrawTreeVertexFar1(float3 pos, float3 swd, bool enlarge=true) { 
	if(enlarge)
		va->EnlargeArrays(4,0,VA_SIZE_T);
	float3 base=pos+swd;
	SetArrayQ(0,0,base);
	base.y+=MAX_TREE_HEIGHT;
	SetArrayQ(0,0.25f,base);
	base-=swd;
	base-=swd;
	SetArrayQ(0.5f,0.5f,base);
	base.y-=MAX_TREE_HEIGHT;
	SetArrayQ(0.5f,0.0f,base);
}

inline void DrawTreeVertexFar2(float3 pos, float3 swd, bool enlarge=true) { 
	if(enlarge)
		va->EnlargeArrays(4,0,VA_SIZE_T);
	float3 base=pos+swd;
	SetArrayQ(0, 0.5f,base);
	base.y+=MAX_TREE_HEIGHT;
	SetArrayQ(0, 1.0f,base);
	base-=swd;
	base-=swd;
	SetArrayQ(0.25f,1.0f,base);
	base.y-=MAX_TREE_HEIGHT;
	SetArrayQ(0.25f,0.5f,base);
}

inline void DrawTreeVertexMid1(float3 pos, bool enlarge=true) {
	if(enlarge)
		va->EnlargeArrays(12,0,VA_SIZE_T);
	float3 base=pos;
	base.x+=MAX_TREE_HEIGHT_3;

	SetArrayQ(0.0f, 0.0f,base);
	base.y+=MAX_TREE_HEIGHT;
	SetArrayQ(0.0f, 0.5f,base);
	base.x-=MAX_TREE_HEIGHT_6;
	SetArrayQ(0.5f, 0.5f,base);
	base.y-=MAX_TREE_HEIGHT;
	SetArrayQ(0.5f, 0.0f,base);

	base.x+=MAX_TREE_HEIGHT_3;
	base.z+=MAX_TREE_HEIGHT_3;
	SetArrayQ(0.0f, 0.0f,base);
	base.y+=MAX_TREE_HEIGHT;
	SetArrayQ(0.0f, 0.5f,base);
	base.z-=MAX_TREE_HEIGHT_6;
	SetArrayQ(0.5f, 0.5f,base);
	base.y-=MAX_TREE_HEIGHT;
	SetArrayQ(0.5f, 0.0f,base);

	base.z+=MAX_TREE_HEIGHT_3;
	base.x+=MAX_TREE_HEIGHT_36;
	base.y+=MAX_TREE_HEIGHT_25;
	SetArrayQ(0.5f, 0.0f,base);
	base.x-=MAX_TREE_HEIGHT_36;
	base.z-=MAX_TREE_HEIGHT_36;
	SetArrayQ(0.5f, 0.5f,base);
	base.x-=MAX_TREE_HEIGHT_36;
	base.z+=MAX_TREE_HEIGHT_36;
	SetArrayQ(1.0f, 0.5f,base);
	base.x+=MAX_TREE_HEIGHT_36;
	base.z+=MAX_TREE_HEIGHT_36;
	SetArrayQ(1.0f, 0.0f,base);
}

inline void DrawTreeVertexMid2(float3 pos, bool enlarge=true) {
	if(enlarge)
		va->EnlargeArrays(12,0,VA_SIZE_T);
	float3 base=pos;
	base.x+=MAX_TREE_HEIGHT_3;

	SetArrayQ(0.0f,  0.5f,base);
	base.y+=MAX_TREE_HEIGHT;
	SetArrayQ(0.0f,  1.0f,base);
	base.x-=MAX_TREE_HEIGHT_6;
	SetArrayQ(0.25f, 1.0f,base);
	base.y-=MAX_TREE_HEIGHT;
	SetArrayQ(0.25f, 0.5f,base);

	base.x+=MAX_TREE_HEIGHT_3;
	base.z+=MAX_TREE_HEIGHT_3;
	SetArrayQ(0.25f, 0.5f,base);
	base.y+=MAX_TREE_HEIGHT;
	SetArrayQ(0.25f, 1.0f,base);
	base.z-=MAX_TREE_HEIGHT_6;
	SetArrayQ(0.5f,  1.0f,base);
	base.y-=MAX_TREE_HEIGHT;
	SetArrayQ(0.5f,  0.5f,base);

	base.z+=MAX_TREE_HEIGHT_3;
	base.x+=MAX_TREE_HEIGHT_36;
	base.y+=MAX_TREE_HEIGHT_3;
	SetArrayQ(0.5f, 0.5f,base);
	base.x-=MAX_TREE_HEIGHT_36;
	base.z-=MAX_TREE_HEIGHT_36;
	SetArrayQ(0.5f, 1.0f,base);
	base.x-=MAX_TREE_HEIGHT_36;
	base.z+=MAX_TREE_HEIGHT_36;
	SetArrayQ(1.0f, 1.0f,base);
	base.x+=MAX_TREE_HEIGHT_36;
	base.z+=MAX_TREE_HEIGHT_36;
	SetArrayQ(1.0f, 0.5f,base);
}

void CBasicTreeSquareDrawer::DrawQuad(int x,int y)
{
	int treesX = td->treesX;
	CBasicTreeDrawer::TreeSquareStruct* tss=&td->trees[y*treesX+x];

	float3 dif;
	dif.x=camera->pos.x-(x*SQUARE_SIZE*TREE_SQUARE_SIZE + SQUARE_SIZE*TREE_SQUARE_SIZE/2);
	dif.y=0;
	dif.z=camera->pos.z-(y*SQUARE_SIZE*TREE_SQUARE_SIZE + SQUARE_SIZE*TREE_SQUARE_SIZE/2);
	float dist=dif.Length();
	dif/=dist;

	float distfactor = dist / treeDistance;

	if(distfactor < MID_TREE_DIST_FACTOR){ // midle distance trees
		tss->lastSeen=gs->frameNum;
		if(!tss->displist){
			va=GetVertexArray();
			va->Initialize();
			va->EnlargeArrays(12*tss->trees.size(),0,VA_SIZE_T); //!alloc room for all tree vertexes
			tss->displist=glGenLists(1);

			for(std::map<int,CBasicTreeDrawer::TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
				CBasicTreeDrawer::TreeStruct* ts=&ti->second;
				if(ts->type<8)
					DrawTreeVertexMid1(ts->pos, false);
				else
					DrawTreeVertexMid2(ts->pos, false);
			}
			glNewList(tss->displist,GL_COMPILE);
			va->DrawArrayT(GL_QUADS);
			glEndList();
		}
		glColor4f(1,1,1,1);
		glDisable(GL_BLEND);
		glAlphaFunc(GL_GREATER,0.5f);
		glCallList(tss->displist);
	}
	else if(distfactor < FAR_TREE_DIST_FACTOR){ // far trees
		tss->lastSeenFar=gs->frameNum;
		if(!tss->farDisplist || dif.dot(tss->viewVector)<0.97f){
			va=GetVertexArray();
			va->Initialize();
			va->EnlargeArrays(4*tss->trees.size(),0,VA_SIZE_T); //!alloc room for all tree vertexes
			tss->viewVector=dif;
			if(!tss->farDisplist)
				tss->farDisplist=glGenLists(1);
			float3 up(0,1,0);
			float3 side=up.cross(dif);

			for(std::map<int,CBasicTreeDrawer::TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
				CBasicTreeDrawer::TreeStruct* ts=&ti->second;
				if(ts->type<8){
					DrawTreeVertexFar1(ts->pos, side*MAX_TREE_HEIGHT_3, false);
				} else {
					DrawTreeVertexFar2(ts->pos, side*MAX_TREE_HEIGHT_3, false);
				}
			}
			glNewList(tss->farDisplist,GL_COMPILE);
			va->DrawArrayT(GL_QUADS);
			glEndList();
		}
		if(distfactor > FADE_TREE_DIST_FACTOR){ // faded far trees
			float trans = 1.0f - (distfactor - FADE_TREE_DIST_FACTOR) / (FAR_TREE_DIST_FACTOR - FADE_TREE_DIST_FACTOR);
			glEnable(GL_BLEND);
			glColor4f(1,1,1,trans);
			glAlphaFunc(GL_GREATER, trans / 2.0f);
		} else {
			glColor4f(1,1,1,1);
			glDisable(GL_BLEND);
			glAlphaFunc(GL_GREATER,0.5f);
		}
		glCallList(tss->farDisplist);
	}
}

void CBasicTreeDrawer::Draw(float treeDistance,bool drawReflection)
{
	glBindTexture(GL_TEXTURE_2D, treetex);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);

	if(globalRendering->drawFog) {
		glFogfv(GL_FOG_COLOR, mapInfo->atmosphere.fogColor);
		glEnable(GL_FOG);
	}
	glColor4f(1,1,1,1);

	const int cx=(int)(camera->pos.x/(SQUARE_SIZE*TREE_SQUARE_SIZE));
	const int cy=(int)(camera->pos.z/(SQUARE_SIZE*TREE_SQUARE_SIZE));

	CBasicTreeSquareDrawer drawer(this, cx, cy, treeDistance * SQUARE_SIZE * TREE_SQUARE_SIZE);

	GML_STDMUTEX_LOCK(tree); // Draw

	readmap->GridVisibility (camera, TREE_SQUARE_SIZE, drawer.treeDistance * 2.0f, &drawer);

	int startClean=lastListClean*20%nTrees;
	lastListClean=gs->frameNum;
	int endClean=gs->frameNum*20%nTrees;

	if(startClean>endClean){
		for(TreeSquareStruct *pTSS=trees+startClean; pTSS<trees+nTrees; ++pTSS) {
			if(pTSS->lastSeen<gs->frameNum-50 && pTSS->displist){
				glDeleteLists(pTSS->displist,1);
				pTSS->displist=0;
			}
			if(pTSS->lastSeenFar<gs->frameNum-50 && pTSS->farDisplist){
				glDeleteLists(pTSS->farDisplist,1);
				pTSS->farDisplist=0;
			}
		}
		for(TreeSquareStruct *pTSS=trees; pTSS<trees+endClean; ++pTSS) {
			if(pTSS->lastSeen<gs->frameNum-50 && pTSS->displist){
				glDeleteLists(pTSS->displist,1);
				pTSS->displist=0;
			}
			if(pTSS->lastSeenFar<gs->frameNum-50 && pTSS->farDisplist){
				glDeleteLists(pTSS->farDisplist,1);
				pTSS->farDisplist=0;
			}
		}
	} else {
		for(TreeSquareStruct *pTSS=trees+startClean; pTSS<trees+endClean; ++pTSS) {
			if(pTSS->lastSeen<gs->frameNum-50 && pTSS->displist){
				glDeleteLists(pTSS->displist,1);
				pTSS->displist=0;
			}
			if(pTSS->lastSeenFar<gs->frameNum-50 && pTSS->farDisplist){
				glDeleteLists(pTSS->farDisplist,1);
				pTSS->farDisplist=0;
			}
		}
	}

	glDisable(GL_FOG);

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
}

void CBasicTreeDrawer::Update()
{
	GML_STDMUTEX_LOCK(tree); // Update

}

void CBasicTreeDrawer::ResetPos(const float3& pos)
{
	int x=(int)(pos.x/TREE_SQUARE_SIZE/SQUARE_SIZE);
	int y=(int)(pos.z/TREE_SQUARE_SIZE/SQUARE_SIZE);
	TreeSquareStruct* pTSS=trees+y*treesX+x;
	if(pTSS->displist){
		delDispLists.push_back(pTSS->displist);
		pTSS->displist=0;
	}
	if(pTSS->farDisplist){
		delDispLists.push_back(pTSS->farDisplist);
		pTSS->farDisplist=0;
	}
}

void CBasicTreeDrawer::AddTree(int type, float3 pos, float size)
{
	GML_STDMUTEX_LOCK(tree); // AddTree

	TreeStruct ts;
	ts.pos=pos;
	ts.type=type;
	int hash=(int)pos.x+((int)(pos.z))*20000;
	int square=((int)pos.x)/(SQUARE_SIZE*TREE_SQUARE_SIZE)+((int)pos.z)/(SQUARE_SIZE*TREE_SQUARE_SIZE)*treesX;
	trees[square].trees[hash]=ts;
	ResetPos(pos);
}

void CBasicTreeDrawer::DeleteTree(float3 pos)
{
	GML_STDMUTEX_LOCK(tree); // DeleteTree

	int hash=(int)pos.x+((int)(pos.z))*20000;
	int square=((int)pos.x)/(SQUARE_SIZE*TREE_SQUARE_SIZE)+((int)pos.z)/(SQUARE_SIZE*TREE_SQUARE_SIZE)*treesX;

	trees[square].trees.erase(hash);

	ResetPos(pos);
}
