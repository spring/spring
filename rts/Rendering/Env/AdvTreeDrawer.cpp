// TreeDrawer.cpp: implementation of the CAdvTreeDrawer class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Rendering/GL/myGL.h"
#include "mmgr.h"
#include "AdvTreeDrawer.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Map/ReadMap.h"
#include "Rendering/GL/myGL.h"
#include "AdvTreeGenerator.h"
#include "Rendering/Textures/Bitmap.h"
#include "LogOutput.h"
#include "GrassDrawer.h"
#include "Matrix44f.h"
#include "Rendering/ShadowHandler.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static const float TEX_LEAF_START_Y1=0.001f;
static const float TEX_LEAF_END_Y1=0.124f;
static const float TEX_LEAF_START_Y2=0.126f;
static const float TEX_LEAF_END_Y2=0.249f;
static const float TEX_LEAF_START_Y3=0.251f;
static const float TEX_LEAF_END_Y3=0.374f;
static const float TEX_LEAF_START_Y4=0.376f;
static const float TEX_LEAF_END_Y4=0.499f;

static const float TEX_LEAF_START_X1=0.0f;
static const float TEX_LEAF_END_X1=0.125f;
static const float TEX_LEAF_START_X2=0.0f;
static const float TEX_LEAF_END_X2=0.125f;
static const float TEX_LEAF_START_X3=0.0f;
static const float TEX_LEAF_END_X3=0.125f;

static const float PART_MAX_TREE_HEIGHT=MAX_TREE_HEIGHT*0.4f;
static const float HALF_MAX_TREE_HEIGHT=MAX_TREE_HEIGHT*0.5f;
static const float DOUBLE_MAX_TREE_HEIGHT=MAX_TREE_HEIGHT*2.0f;

CAdvTreeDrawer::CAdvTreeDrawer()
{
	oldTreeDistance=4;

	treeGen=SAFE_NEW CAdvTreeGenerator;
	grassDrawer=SAFE_NEW CGrassDrawer();
	lastListClean=0;
	treesX=gs->mapx/TREE_SQUARE_SIZE;
	treesY=gs->mapy/TREE_SQUARE_SIZE;
	nTrees=treesX*treesY;
	trees=SAFE_NEW TreeSquareStruct[nTrees];

	for(TreeSquareStruct* pTSS=trees; pTSS<trees+nTrees; ++pTSS) {
		pTSS->lastSeen=0;
		pTSS->lastSeenFar=0;
		pTSS->viewVector=UpVector;
		pTSS->displist=0;
		pTSS->farDisplist=0;
	}
}

CAdvTreeDrawer::~CAdvTreeDrawer()
{
	for(TreeSquareStruct* pTSS=trees; pTSS<trees+nTrees; ++pTSS) {
		if(pTSS->displist)
			glDeleteLists(pTSS->displist,1);
		if(pTSS->farDisplist)
			glDeleteLists(pTSS->farDisplist,1);
	}
	delete[] trees;
	delete treeGen;
	delete grassDrawer;
}

void CAdvTreeDrawer::Update()
{
	GML_STDMUTEX_LOCK(tree); // Update

	for(std::list<FallingTree>::iterator fti=fallingTrees.begin();fti!=fallingTrees.end();){
		fti->fallPos+=fti->speed*0.1f;
		if(fti->fallPos>1){		//remove the tree
			std::list<FallingTree>::iterator prev=fti++;
			fallingTrees.erase(prev);
		} else {
			fti->speed+=sin(fti->fallPos)*0.04f;
			++fti;
		}
	}

}

static CVertexArray *va;

static void inline SetArrayQ(float t1,float t2,float3 v)
{
	va->AddVertexQT(v,t1,t2);
}

inline void DrawTreeVertexA(float3 &ftpos, float dx, float dy) {
	SetArrayQ(TEX_LEAF_START_X1+dx,TEX_LEAF_START_Y1+dy,ftpos);
	ftpos.y+=MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_START_X1+dx,TEX_LEAF_END_Y1+dy,ftpos);
	ftpos.x-=MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X1+dx,TEX_LEAF_END_Y1+dy,ftpos);
	ftpos.y-=MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X1+dx,TEX_LEAF_START_Y1+dy,ftpos);
	ftpos.x+=HALF_MAX_TREE_HEIGHT;
	ftpos.z+=HALF_MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_START_X2+dx,TEX_LEAF_START_Y2+dy,ftpos);
	ftpos.y+=MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_START_X2+dx,TEX_LEAF_END_Y2+dy,ftpos);
	ftpos.z-=MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X2+dx,TEX_LEAF_END_Y2+dy,ftpos);
	ftpos.y-=MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X2+dx,TEX_LEAF_START_Y2+dy,ftpos);
	//width*=1.41f;
	ftpos.x+=HALF_MAX_TREE_HEIGHT;
	ftpos.y+=PART_MAX_TREE_HEIGHT;
}

inline void DrawTreeVertex(float3 pos, float dx, float dy, bool enlarge=true) {
	if(enlarge)
		va->EnlargeArrays(12,0,VA_SIZE_T);
	float3 ftpos=pos;
	ftpos.x+=HALF_MAX_TREE_HEIGHT;
	DrawTreeVertexA(ftpos, dx, dy);
	ftpos.z+=MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_START_X3+dx,TEX_LEAF_START_Y3+dy,ftpos);
	ftpos.z-=MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_START_X3+dx,TEX_LEAF_END_Y3+dy,ftpos);
	ftpos.x-=MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X3+dx,TEX_LEAF_END_Y3+dy,ftpos);
	ftpos.z+=MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X3+dx,TEX_LEAF_START_Y3+dy,ftpos);
}

inline void DrawTreeVertexMid(const float3 pos, float dx, float dy, bool enlarge=true) {
	if(enlarge)
		va->EnlargeArrays(12,0,VA_SIZE_T);
	float3 ftpos=pos;
	ftpos.x+=HALF_MAX_TREE_HEIGHT;
	DrawTreeVertexA(ftpos, dx, dy);
	ftpos.z+=HALF_MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_START_X3+dx,TEX_LEAF_START_Y3+dy,ftpos);
	ftpos.x-=HALF_MAX_TREE_HEIGHT;
	ftpos.z-=HALF_MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_START_X3+dx,TEX_LEAF_END_Y3+dy,ftpos);
	ftpos.x-=HALF_MAX_TREE_HEIGHT;
	ftpos.z+=HALF_MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X3+dx,TEX_LEAF_END_Y3+dy,ftpos);
	ftpos.x+=HALF_MAX_TREE_HEIGHT;
	ftpos.z+=HALF_MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X3+dx,TEX_LEAF_START_Y3+dy,ftpos);
}

inline void DrawTreeVertexFar(float3 pos, float3 swd, float dx, float dy, bool enlarge=true) {
	if(enlarge)
		va->EnlargeArrays(4,0,VA_SIZE_T);
	float3 base=pos+swd;
	SetArrayQ(TEX_LEAF_START_X1+dx,TEX_LEAF_START_Y4+dy,base);
	base.y+=MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_START_X1+dx,TEX_LEAF_END_Y4+dy,base);
	base-=swd;
	base-=swd;
	SetArrayQ(TEX_LEAF_END_X1+dx,TEX_LEAF_END_Y4+dy,base);
	base.y-=MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X1+dx,TEX_LEAF_START_Y4+dy,base);
}

struct CAdvTreeSquareDrawer : CReadMap::IQuadDrawer
{
	CAdvTreeSquareDrawer() {td=0;}

	void DrawQuad (int x,int y);

	CAdvTreeDrawer *td;
	int cx,cy;
	float treeDistance;
	bool drawDetailed;
};

void CAdvTreeSquareDrawer::DrawQuad (int x,int y)
{
	int treesX = td->treesX;
	CAdvTreeDrawer::TreeSquareStruct* tss=&td->trees[y*treesX+x];
	if(abs(cy-y)<=2 && abs(cx-x)<=2 && drawDetailed)	//skip the closest squares
		return;

	float3 dif;
	dif.x=camera->pos.x-(x*SQUARE_SIZE*TREE_SQUARE_SIZE + SQUARE_SIZE*TREE_SQUARE_SIZE/2);
	dif.y=0;
	dif.z=camera->pos.z-(y*SQUARE_SIZE*TREE_SQUARE_SIZE + SQUARE_SIZE*TREE_SQUARE_SIZE/2);
	float dist=dif.Length();
	dif/=dist;

	if(dist<SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance*2 && dist>SQUARE_SIZE*TREE_SQUARE_SIZE*(treeDistance)){//far trees
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

			for(std::map<int,CAdvTreeDrawer::TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
				CAdvTreeDrawer::TreeStruct* ts=&ti->second;

				if(ts->type<8)
					DrawTreeVertexFar(ts->pos, side*HALF_MAX_TREE_HEIGHT, ts->type*0.125f, 0.5f, false);
				else
					DrawTreeVertexFar(ts->pos, side*HALF_MAX_TREE_HEIGHT, (ts->type-8)*0.125f, 0, false);
			}
			glNewList(tss->farDisplist,GL_COMPILE);
			va->DrawArrayT(GL_QUADS);
			glEndList();
		}
		if(dist>SQUARE_SIZE*TREE_SQUARE_SIZE*(treeDistance*2-1)){
			float trans=(SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance*2-dist)/(SQUARE_SIZE*TREE_SQUARE_SIZE);
			glEnable(GL_BLEND);
			glColor4f(1,1,1,trans);
			glAlphaFunc(GL_GREATER,(SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance*2-dist)/(SQUARE_SIZE*TREE_SQUARE_SIZE*2));
		} else {
			glColor4f(1,1,1,1);
			glDisable(GL_BLEND);
			glAlphaFunc(GL_GREATER,0.5f);
		}
		glCallList(tss->farDisplist);
	}

	if(dist<SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance){	//midle distance trees
		tss->lastSeen=gs->frameNum;
		if(!tss->displist){
			va=GetVertexArray();
			va->Initialize();
			va->EnlargeArrays(12*tss->trees.size(),0,VA_SIZE_T); //!alloc room for all tree vertexes

			tss->displist=glGenLists(1);

			for(std::map<int,CAdvTreeDrawer::TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
				CAdvTreeDrawer::TreeStruct* ts=&ti->second;

				if(ts->type<8)
					DrawTreeVertexMid(ts->pos, ts->type*0.125f, 0.5f, false);
				else
					DrawTreeVertexMid(ts->pos, (ts->type-8)*0.125f, 0, false);
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
}

void CAdvTreeDrawer::Draw(float treeDistance,bool drawReflection)
{
	int activeFarTex=camera->forward.z<0 ? treeGen->farTex[0] : treeGen->farTex[1];

	bool drawDetailed=true;
	if(treeDistance<4)
		drawDetailed=false;
	if(drawReflection)
		drawDetailed=false;

	CBaseGroundDrawer *gd = readmap->GetGroundDrawer ();

	glEnable(GL_ALPHA_TEST);

	if(shadowHandler->drawShadows && !gd->DrawExtraTex()){
		glBindProgramARB( GL_VERTEX_PROGRAM_ARB, treeGen->treeFarVP );
		glEnable(GL_VERTEX_PROGRAM_ARB);

		glBindTexture(GL_TEXTURE_2D,shadowHandler->shadowTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_ALPHA);

		if(shadowHandler->useFPShadows){
			glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, treeGen->treeFPShadow );
			glEnable( GL_FRAGMENT_PROGRAM_ARB );
			glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,10, mapInfo->light.groundAmbientColor.x,mapInfo->light.groundAmbientColor.y,mapInfo->light.groundAmbientColor.z,1);
			glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,11, 0,0,0,1-mapInfo->light.groundShadowDensity*0.5f);

			glActiveTextureARB(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, activeFarTex);
		} else {
			glEnable(GL_TEXTURE_2D);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FAIL_VALUE_ARB, 1-mapInfo->light.groundShadowDensity*0.5f);

			float texConstant[]={mapInfo->light.groundAmbientColor.x,mapInfo->light.groundAmbientColor.y,mapInfo->light.groundAmbientColor.z,0.0f};
			glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,texConstant);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB,GL_CONSTANT);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_RGB_ARB,GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_RGB_ARB,GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_INTERPOLATE_ARB);

			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_ARB,GL_CONSTANT);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_ADD);

			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);

			glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, activeFarTex);
		}
		glActiveTextureARB(GL_TEXTURE0_ARB);

		glMatrixMode(GL_MATRIX0_ARB);
		glLoadMatrixf(shadowHandler->shadowMatrix.m);
		glMatrixMode(GL_MODELVIEW);
	} else {
		glBindTexture(GL_TEXTURE_2D, activeFarTex);
	}
	glEnable(GL_TEXTURE_2D);

	int cx=(int)(camera->pos.x/(SQUARE_SIZE*TREE_SQUARE_SIZE));
	int cy=(int)(camera->pos.z/(SQUARE_SIZE*TREE_SQUARE_SIZE));

	CAdvTreeSquareDrawer drawer;
	drawer.td = this;
	drawer.cx = cx;
	drawer.cy = cy;
	drawer.treeDistance = treeDistance;
	drawer.drawDetailed = drawDetailed;

	GML_STDMUTEX_LOCK(tree); // Draw

	// draw far away trees using the map dependent grid visibility
	oldTreeDistance=treeDistance;
	readmap->GridVisibility (camera, TREE_SQUARE_SIZE, treeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE, &drawer);

	if(drawDetailed){
		int xstart=std::max(0,cx-2);
		int xend=std::min(gs->mapx/TREE_SQUARE_SIZE-1,cx+2);
		int ystart=std::max(0,cy-2);
		int yend=std::min(gs->mapy/TREE_SQUARE_SIZE-1,cy+2);

		if(shadowHandler->drawShadows && !gd->DrawExtraTex()){
			glBindProgramARB( GL_VERTEX_PROGRAM_ARB, treeGen->treeVP );

			glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, treeGen->barkTex);
			glActiveTextureARB(GL_TEXTURE0_ARB);

		} else {
			glBindTexture(GL_TEXTURE_2D, treeGen->barkTex);
			glBindProgramARB( GL_VERTEX_PROGRAM_ARB, treeGen->treeNSVP );
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,15, 1.0f/(gs->pwr2mapx*SQUARE_SIZE),1.0f/(gs->pwr2mapy*SQUARE_SIZE),1.0f/(gs->pwr2mapx*SQUARE_SIZE),1);
		}
		glEnable( GL_VERTEX_PROGRAM_ARB );
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13,  camera->right.x,camera->right.y,camera->right.z,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,9,  camera->up.x,camera->up.y,camera->up.z,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,11, mapInfo->light.groundSunColor.x,mapInfo->light.groundSunColor.y,mapInfo->light.groundSunColor.z,0.85f);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,14, mapInfo->light.groundAmbientColor.x,mapInfo->light.groundAmbientColor.y,mapInfo->light.groundAmbientColor.z,0.85f);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12, 0,0,0,0.20f*(1.0f/MAX_TREE_HEIGHT));	//w=alpha/height modifier
		glAlphaFunc(GL_GREATER,0.5f);
		glDisable(GL_BLEND);
		glColor4f(1,1,1,1);
		va=GetVertexArray();
		va->Initialize();

		struct FadeTree{
			float3 pos;
			float relDist;
			float deltaY;
			int type;
		};
		static FadeTree fadeTrees[3000];
		FadeTree *pFT=fadeTrees;

		for(TreeSquareStruct* pTSS=trees+ystart*treesX; pTSS<=trees+yend*treesX; pTSS+=treesX) {
			for(TreeSquareStruct* tss=pTSS+xstart; tss<=pTSS+xend; ++tss) {
				tss->lastSeen=gs->frameNum;
				va->EnlargeArrays(12*tss->trees.size(),0,VA_SIZE_T); //!alloc room for all tree vertexes
				for(std::map<int,TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
					TreeStruct* ts=&ti->second;
					float3 pos(ts->pos);
					int type=ts->type;
					float dy;
					unsigned int displist;
					if(type<8){
						dy=0.5f;
						displist=treeGen->pineDL+type;
					} else {
						type-=8;
						dy=0;
						displist=treeGen->leafDL+type;
					}
					if(camera->InView(pos+float3(0,MAX_TREE_HEIGHT/2,0),MAX_TREE_HEIGHT/2)){
						float camDist=(pos-camera->pos).SqLength();
						if(camDist<SQUARE_SIZE*SQUARE_SIZE*110*110){	//draw detailed tree
							glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10,pos.x,pos.y,pos.z,0);
							glCallList(displist);
						} else if(camDist<SQUARE_SIZE*SQUARE_SIZE*125*125){	//draw fading tree
							float relDist=(pos.distance(camera->pos)-SQUARE_SIZE*110)/(SQUARE_SIZE*15);
							glAlphaFunc(GL_GREATER,0.8f+relDist*0.2f);
							glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10,pos.x,pos.y,pos.z,0);
							glCallList(displist);
							glAlphaFunc(GL_GREATER,0.5f);
							pFT->pos=pos;
							pFT->deltaY=dy;
							pFT->type=type;
							pFT->relDist=relDist;
							++pFT;
						} else {																//draw undetailed tree
							DrawTreeVertex(pos, type*0.125f, dy, false);
						}
					}
				}
			}
		}
		//draw trees that has been marked as falling
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10,0,0,0,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13,  camera->right.x,camera->right.y,camera->right.z,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,9,  camera->up.x,camera->up.y,camera->up.z,0);
		for(std::list<FallingTree>::iterator fti=fallingTrees.begin(); fti!=fallingTrees.end(); ++fti){
			float3 pos=fti->pos-UpVector*(fti->fallPos*20);
			if(camera->InView(pos+float3(0,MAX_TREE_HEIGHT/2,0),MAX_TREE_HEIGHT/2)){
				float ang=fti->fallPos*PI;
				float3 up(fti->dir.x*sin(ang),cos(ang),fti->dir.z*sin(ang));
				float3 z(up.cross(float3(-1,0,0)));
				z.ANormalize();
				float3 x(up.cross(z));
				CMatrix44f transMatrix(pos,x,up,z);

				glPushMatrix();
				glMultMatrixf(&transMatrix[0]);

				int type=fti->type;
				int displist;
				if(type<8){
					displist=treeGen->pineDL+type;
				} else {
					type-=8;
					displist=treeGen->leafDL+type;
				}
				glCallList(displist);

				glPopMatrix();
			}
		}

		glDisable( GL_VERTEX_PROGRAM_ARB );

		if(shadowHandler->drawShadows && !gd->DrawExtraTex()){
			glBindProgramARB( GL_VERTEX_PROGRAM_ARB, treeGen->treeFarVP );
			glEnable(GL_VERTEX_PROGRAM_ARB);

			glActiveTextureARB(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, activeFarTex);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		} else {
			glBindTexture(GL_TEXTURE_2D, activeFarTex);
		}
		va->DrawArrayT(GL_QUADS);

		for(FadeTree *pFTree=fadeTrees; pFTree<pFT; ++pFTree) { //faded close trees
			va=GetVertexArray();
			va->Initialize();
			va->CheckInitSize(12*VA_SIZE_T);

			DrawTreeVertex(pFTree->pos, pFTree->type*0.125f, pFTree->deltaY, false);

			glAlphaFunc(GL_GREATER,1-pFTree->relDist*0.5f);
			va->DrawArrayT(GL_QUADS);
		}
	}
	if(shadowHandler->drawShadows && !gd->DrawExtraTex()){
		glDisable( GL_VERTEX_PROGRAM_ARB );
		if(shadowHandler->useFPShadows){
			glDisable(GL_FRAGMENT_PROGRAM_ARB);
		}
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
	}
	glDisable(GL_ALPHA_TEST);

	//clean out squares from memory that are no longer visible
	int startClean=lastListClean*20%(nTrees);
	lastListClean=gs->frameNum;
	int endClean=gs->frameNum*20%(nTrees);

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
}

struct CAdvTreeSquareDrawer_SP : CReadMap::IQuadDrawer
{
	void DrawQuad (int x,int y);

	CAdvTreeDrawer *td;
	int cx,cy;
	bool drawDetailed;
	float treeDistance;
};

void CAdvTreeSquareDrawer_SP::DrawQuad (int x,int y)
{
	int treesX = td->treesX;
	CAdvTreeDrawer::TreeSquareStruct* tss=&td->trees[y*treesX+x];
	if(abs(cy-y)<=2 && abs(cx-x)<=2 && drawDetailed)	//skip the closest squares
		return;

	float3 dif;
	dif.x=camera->pos.x-(x*SQUARE_SIZE*TREE_SQUARE_SIZE + SQUARE_SIZE*TREE_SQUARE_SIZE/2);
	dif.y=0;
	dif.z=camera->pos.z-(y*SQUARE_SIZE*TREE_SQUARE_SIZE + SQUARE_SIZE*TREE_SQUARE_SIZE/2);
	float dist=dif.Length();
	dif/=dist;

	if(dist<SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance*2 && dist>SQUARE_SIZE*TREE_SQUARE_SIZE*(treeDistance)){//far trees
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

			for(std::map<int,CAdvTreeDrawer::TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
				CAdvTreeDrawer::TreeStruct* ts=&ti->second;

				if(ts->type<8)
					DrawTreeVertexFar(ts->pos, side*HALF_MAX_TREE_HEIGHT, ts->type*0.125f, 0.5f, false);
				else
					DrawTreeVertexFar(ts->pos, side*HALF_MAX_TREE_HEIGHT, (ts->type-8)*0.125f, 0, false);
			}
			glNewList(tss->farDisplist,GL_COMPILE);
			va->DrawArrayT(GL_QUADS);
			glEndList();
		}
		if(dist>SQUARE_SIZE*TREE_SQUARE_SIZE*(treeDistance*2-1)){
			float trans=(SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance*2-dist)/(SQUARE_SIZE*TREE_SQUARE_SIZE);
			glColor4f(1,1,1,trans);
			glAlphaFunc(GL_GREATER,(SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance*2-dist)/(SQUARE_SIZE*TREE_SQUARE_SIZE*2));
		} else {
			glColor4f(1,1,1,1);
			glAlphaFunc(GL_GREATER,0.5f);
		}
		glCallList(tss->farDisplist);
	}

	if(dist<SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance){	//midle distance trees
		tss->lastSeen=gs->frameNum;
		if(!tss->displist){
			va=GetVertexArray();
			va->Initialize();
			va->EnlargeArrays(12*tss->trees.size(),0,VA_SIZE_T); //!alloc room for all tree vertexes
			tss->displist=glGenLists(1);

			for(std::map<int,CAdvTreeDrawer::TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
				CAdvTreeDrawer::TreeStruct* ts=&ti->second;

				if(ts->type<8)
					DrawTreeVertexMid(ts->pos, ts->type*0.125f, 0.5f, false);
				else
					DrawTreeVertexMid(ts->pos, (ts->type-8)*0.125f, 0, false);
			}
			glNewList(tss->displist,GL_COMPILE);
			va->DrawArrayT(GL_QUADS);
			glEndList();
		}
		glColor4f(1,1,1,1);
		glAlphaFunc(GL_GREATER,0.5f);
		glCallList(tss->displist);
	}
}

void CAdvTreeDrawer::DrawShadowPass(void)
{
	float treeDistance=oldTreeDistance;

	int activeFarTex=camera->forward.z<0 ? treeGen->farTex[0] : treeGen->farTex[1];

	bool drawDetailed=true;
	if(treeDistance<4)
		drawDetailed=false;

	glBindTexture(GL_TEXTURE_2D, activeFarTex);
	glEnable(GL_TEXTURE_2D);
	glBindProgramARB( GL_VERTEX_PROGRAM_ARB, treeGen->treeFarShadowVP );
	glEnable( GL_VERTEX_PROGRAM_ARB );
	glEnable(GL_ALPHA_TEST);
	glPolygonOffset(1,1);
	glEnable(GL_POLYGON_OFFSET_FILL);

	CAdvTreeSquareDrawer_SP drawer;
	int cx = drawer.cx=(int)(camera->pos.x/(SQUARE_SIZE*TREE_SQUARE_SIZE));
	int cy = drawer.cy=(int)(camera->pos.z/(SQUARE_SIZE*TREE_SQUARE_SIZE));

	drawer.drawDetailed = drawDetailed;
	drawer.td = this;
	drawer.treeDistance = treeDistance;

	GML_STDMUTEX_LOCK(tree); // DrawShadowPass

	// draw with extraSize=1
	readmap->GridVisibility (camera, TREE_SQUARE_SIZE, treeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE, &drawer, 1);

	if(drawDetailed){
		int xstart=std::max(0,cx-2);
		int xend=std::min(gs->mapx/TREE_SQUARE_SIZE-1,cx+2);
		int ystart=std::max(0,cy-2);
		int yend=std::min(gs->mapy/TREE_SQUARE_SIZE-1,cy+2);

		glBindTexture(GL_TEXTURE_2D, treeGen->barkTex);
		glEnable(GL_TEXTURE_2D);
		glBindProgramARB( GL_VERTEX_PROGRAM_ARB, treeGen->treeShadowVP );
		glEnable( GL_VERTEX_PROGRAM_ARB );
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13,  camera->right.x,camera->right.y,camera->right.z,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,9,  camera->up.x,camera->up.y,camera->up.z,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,11, 1,1,1,0.85f);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12, 0,0,0,0.20f*(1.0f/MAX_TREE_HEIGHT));	//w=alpha/height modifier
		glAlphaFunc(GL_GREATER,0.5f);
		glEnable(GL_ALPHA_TEST);
		glColor4f(1,1,1,1);
		va=GetVertexArray();
		va->Initialize();

		struct FadeTree{
			float3 pos;
			float relDist;
			float deltaY;
			int type;
		};
		static FadeTree fadeTrees[3000];
		FadeTree *pFT=fadeTrees;

		for(TreeSquareStruct* pTSS=trees+ystart*treesX; pTSS<=trees+yend*treesX; pTSS+=treesX) {
			for(TreeSquareStruct* tss=pTSS+xstart; tss<=pTSS+xend; ++tss) {
				tss->lastSeen=gs->frameNum;
				va->EnlargeArrays(12*tss->trees.size(),0,VA_SIZE_T); //!alloc room for all tree vertexes
				for(std::map<int,TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
					TreeStruct* ts=&ti->second;
					float3 pos(ts->pos);
					int type=ts->type;
					float dy;
					unsigned int displist;
					if(type<8){
						dy=0.5f;
						displist=treeGen->pineDL+type;
					} else {
						type-=8;
						dy=0;
						displist=treeGen->leafDL+type;
					}
					if(camera->InView(pos+float3(0,MAX_TREE_HEIGHT/2,0),MAX_TREE_HEIGHT/2+150)){
						float camDist=(pos-camera->pos).SqLength();
						if(camDist<SQUARE_SIZE*SQUARE_SIZE*110*110){	//draw detailed tree
							glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10,pos.x,pos.y,pos.z,0);
							glCallList(displist);
						} else if(camDist<SQUARE_SIZE*SQUARE_SIZE*125*125){	//draw fading tree
							float relDist=(pos.distance(camera->pos)-SQUARE_SIZE*110)/(SQUARE_SIZE*15);
							glAlphaFunc(GL_GREATER,0.8f+relDist*0.2f);
							glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10,pos.x,pos.y,pos.z,0);
							glCallList(displist);
							glAlphaFunc(GL_GREATER,0.5f);
							pFT->pos=pos;
							pFT->deltaY=dy;
							pFT->type=type;
							pFT->relDist=relDist;
							++pFT;
						} else {																//draw undetailed tree
							DrawTreeVertex(pos, type*0.125f, dy, false);
						}
					}
				}
			}
		}
		//draw trees that have been marked as falling
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10,0,0,0,0);
		for(std::list<FallingTree>::iterator fti=fallingTrees.begin(); fti!=fallingTrees.end(); ++fti){
			float3 pos=fti->pos-UpVector*(fti->fallPos*20);
			if(camera->InView(pos+float3(0,MAX_TREE_HEIGHT/2,0),MAX_TREE_HEIGHT/2)){
				float ang=fti->fallPos*PI;
				float3 up(fti->dir.x*sin(ang),cos(ang),fti->dir.z*sin(ang));
				float3 z(up.cross(float3(1,0,0)));
				z.ANormalize();
				float3 x(z.cross(up));
				CMatrix44f transMatrix(pos,x,up,z);

				glPushMatrix();
				glMultMatrixf(&transMatrix[0]);

				int type=fti->type;
				int displist;
				if(type<8){
					displist=treeGen->pineDL+type;
				} else {
					type-=8;
					displist=treeGen->leafDL+type;
				}
				glCallList(displist);

				glPopMatrix();
			}
		}
		glBindProgramARB( GL_VERTEX_PROGRAM_ARB, treeGen->treeFarShadowVP );
		glBindTexture(GL_TEXTURE_2D, activeFarTex);
		va->DrawArrayT(GL_QUADS);

		for(FadeTree *pFTree=fadeTrees; pFTree<pFT; ++pFTree) { //faded close trees
			va=GetVertexArray();
			va->Initialize();
			va->CheckInitSize(12*VA_SIZE_T);

			DrawTreeVertex(pFTree->pos, pFTree->type*0.125f, pFTree->deltaY, false);

			glAlphaFunc(GL_GREATER,1-pFTree->relDist*0.5f);
			va->DrawArrayT(GL_QUADS);
		}
	}
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable( GL_VERTEX_PROGRAM_ARB );
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
}

void CAdvTreeDrawer::DrawGrass(void)
{
	grassDrawer->Draw();
}

void CAdvTreeDrawer::ResetPos(const float3& pos)
{
	int x=(int)pos.x/TREE_SQUARE_SIZE/SQUARE_SIZE;
	int y=(int)pos.z/TREE_SQUARE_SIZE/SQUARE_SIZE;
	TreeSquareStruct* pTSS=trees+y*treesX+x;
	if(pTSS->displist){
//		glDeleteLists(pTSS->displist,1);
		delDispLists.push_back(pTSS->displist);
		pTSS->displist=0;
	}
	if(pTSS->farDisplist){
//		glDeleteLists(pTSS->farDisplist,1);
		delDispLists.push_back(pTSS->farDisplist);
		pTSS->farDisplist=0;
	}
	grassDrawer->ResetPos(pos);
}

void CAdvTreeDrawer::AddTree(int type, float3 pos, float size)
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

void CAdvTreeDrawer::DeleteTree(float3 pos)
{
	GML_STDMUTEX_LOCK(tree); // DeleteTree

	int hash=(int)pos.x+((int)(pos.z))*20000;
	int square=((int)pos.x)/(SQUARE_SIZE*TREE_SQUARE_SIZE)+((int)pos.z)/(SQUARE_SIZE*TREE_SQUARE_SIZE)*treesX;

	trees[square].trees.erase(hash);

	ResetPos(pos);
}

int CAdvTreeDrawer::AddFallingTree(float3 pos, float3 dir, int type)
{
	GML_STDMUTEX_LOCK(tree); // AddFallingTree

	FallingTree ft;

	ft.pos=pos;
	dir.y=0;
	float s=dir.Length();
	if(s>500)
		return 0;
	ft.dir=dir/s;
	ft.speed=std::max(0.01f,s*0.0004f);
	ft.type=type;
	ft.fallPos=0;

	fallingTrees.push_back(ft);
	return 0;
}

void CAdvTreeDrawer::AddGrass(float3 pos)
{
	GML_STDMUTEX_LOCK(tree); // AddGrass

	grassDrawer->AddGrass(pos);
}

void CAdvTreeDrawer::RemoveGrass(int x, int z)
{
	GML_STDMUTEX_LOCK(tree); // RemoveGrass

	grassDrawer->RemoveGrass(x,z);
}

