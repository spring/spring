// TreeDrawer.cpp: implementation of the CAdvTreeDrawer class.
//
//////////////////////////////////////////////////////////////////////

#include "System/StdAfx.h"
#include "AdvTreeDrawer.h"
#include "Rendering/Map/BaseGroundDrawer.h"
#include "Sim/Map/Ground.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Map/ReadMap.h"
#include "Rendering/GL/myGL.h"
#include "AdvTreeGenerator.h"
#include "System/Bitmap.h"
#include "Game/UI/InfoConsole.h"
#include "GrassDrawer.h"
#include "System/Matrix44f.h"
#include "Rendering/ShadowHandler.h"
//#include "System/mmgr.h"

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

CAdvTreeDrawer::CAdvTreeDrawer()
{
	PrintLoadMsg("Loading tree texture");
	oldTreeDistance=4;

	treeGen=new CAdvTreeGenerator;
	grassDrawer=new CGrassDrawer();
	lastListClean=0;
	treesX=gs->mapx/TREE_SQUARE_SIZE;
	treesY=gs->mapy/TREE_SQUARE_SIZE;
	trees=new TreeSquareStruct[treesX*treesY];

	for(int y=0;y<treesY;y++){
		for(int x=0;x<treesX;x++){
			trees[y*treesX+x].lastSeen=0;
			trees[y*treesX+x].lastSeenFar=0;
			trees[y*treesX+x].viewVector=UpVector;
			trees[y*treesX+x].displist=0;
			trees[y*treesX+x].farDisplist=0;
		}
	}
}

CAdvTreeDrawer::~CAdvTreeDrawer()
{
	for(int y=0;y<treesY;y++){
		for(int x=0;x<treesX;x++){
			if(trees[y*treesX+x].displist)
				glDeleteLists(trees[y*treesX+x].displist,1);
			if(trees[y*treesX+x].farDisplist)
				glDeleteLists(trees[y*treesX+x].farDisplist,1);
		}
	}
	delete[] trees;
	delete treeGen;
	delete grassDrawer;
}

void CAdvTreeDrawer::Update()
{
	for(std::list<FallingTree>::iterator fti=fallingTrees.begin();fti!=fallingTrees.end();){
		fti->fallPos+=fti->speed*0.1;
		if(fti->fallPos>1){		//remove the tree
			std::list<FallingTree>::iterator prev=fti++;
			fallingTrees.erase(prev);
		} else {
			fti->speed+=sin(fti->fallPos)*0.04;
			++fti;
		}
	}

}
static CVertexArray* va;

static void inline SetArray(float t1,float t2,float3 v)
{
	va->AddVertexT(v,t1,t2);
}

void CAdvTreeDrawer::Draw(float treeDistance,bool drawReflection)
{
	int cx=(int)(camera->pos.x/(SQUARE_SIZE*TREE_SQUARE_SIZE));
	int cy=(int)(camera->pos.z/(SQUARE_SIZE*TREE_SQUARE_SIZE));

	oldTreeDistance=treeDistance;
	int treeSquare=int(treeDistance*2)+1;

	int activeFarTex=camera->forward.z<0 ? treeGen->farTex[0] : treeGen->farTex[1];

	bool drawDetailed=true;
	if(treeDistance<4)
		drawDetailed=false;
	if(drawReflection)
		drawDetailed=false;

	if(shadowHandler->drawShadows && !(groundDrawer->drawExtraTex)){
		glBindProgramARB( GL_VERTEX_PROGRAM_ARB, treeGen->treeFarVP );
		glEnable(GL_VERTEX_PROGRAM_ARB);

		glBindTexture(GL_TEXTURE_2D,shadowHandler->shadowTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_ALPHA);

		if(shadowHandler->useFPShadows){
			glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, treeGen->treeFPShadow );
			glEnable( GL_FRAGMENT_PROGRAM_ARB );
			glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,10, readmap->ambientColor.x,readmap->ambientColor.y,readmap->ambientColor.z,1);
			glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,11, 0,0,0,1-readmap->shadowDensity*0.5);

			glActiveTextureARB(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, activeFarTex);
		} else {
			glEnable(GL_TEXTURE_2D);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FAIL_VALUE_ARB, 1-readmap->shadowDensity*0.5);

			float texConstant[]={readmap->ambientColor.x,readmap->ambientColor.y,readmap->ambientColor.z,0.0};
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
	int sy=cy-treeSquare;
	if(sy<0)
		sy=0;
	int ey=cy+treeSquare;
	if(ey>=treesY)
		ey=treesY-1;

	for(int y=sy;y<=ey;y++){
		int sx=cx-treeSquare;
		if(sx<0)
			sx=0;
		int ex=cx+treeSquare;
		if(ex>=treesX)
			ex=treesX-1;
		float xtest,xtest2;
		std::vector<CBaseGroundDrawer::fline>::iterator fli;
		for(fli=groundDrawer->left.begin();fli!=groundDrawer->left.end();fli++){
			xtest=((fli->base/SQUARE_SIZE+fli->dir*(y*TREE_SQUARE_SIZE)));
			xtest2=((fli->base/SQUARE_SIZE+fli->dir*((y*TREE_SQUARE_SIZE)+TREE_SQUARE_SIZE)));
			if(xtest>xtest2)
				xtest=xtest2;
			xtest=xtest/TREE_SQUARE_SIZE;
			if(xtest>sx)
				sx=(int)xtest;
		}
		for(fli=groundDrawer->right.begin();fli!=groundDrawer->right.end();fli++){
			xtest=((fli->base/SQUARE_SIZE+fli->dir*(y*TREE_SQUARE_SIZE)));
			xtest2=((fli->base/SQUARE_SIZE+fli->dir*((y*TREE_SQUARE_SIZE)+TREE_SQUARE_SIZE)));
			if(xtest<xtest2)
				xtest=xtest2;
			xtest=xtest/TREE_SQUARE_SIZE;
			if(xtest<ex)
				ex=(int)xtest;
		}
		for(int x=sx;x<=ex;x++){/**/
			TreeSquareStruct* tss=&trees[y*treesX+x];
			if(abs(cy-y)<=2 && abs(cx-x)<=2 && drawDetailed)	//skip the closest squares
				continue;

			float3 dif;
			dif.x=camera->pos.x-(x*SQUARE_SIZE*TREE_SQUARE_SIZE + SQUARE_SIZE*TREE_SQUARE_SIZE/2);
			dif.y=0;
			dif.z=camera->pos.z-(y*SQUARE_SIZE*TREE_SQUARE_SIZE + SQUARE_SIZE*TREE_SQUARE_SIZE/2);
			float dist=dif.Length();
			dif/=dist;
			
			if(dist<SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance*2 && dist>SQUARE_SIZE*TREE_SQUARE_SIZE*(treeDistance)){//far trees
				tss->lastSeenFar=gs->frameNum;
				if(!tss->farDisplist || dif.dot(tss->viewVector)<0.97){
					va=GetVertexArray();
					va->Initialize();
					tss->viewVector=dif;
					if(!tss->farDisplist)
						tss->farDisplist=glGenLists(1);
					float3 up(0,1,0);
					float3 side=up.cross(dif);

					for(std::map<int,TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
						TreeStruct* ts=&ti->second;

						float3 base(ts->pos);
						int type=ts->type;

						float height=MAX_TREE_HEIGHT;
						float width=MAX_TREE_HEIGHT*0.5;
	
						float xdif;
						float ydif;
						if(ts->type<8){
							xdif=type*0.125;
							ydif=0.5;
						} else {
							xdif=(type-8)*0.125;
							ydif=0;
						}
						SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_START_Y4+ydif,base+side*width);
						SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_END_Y4+ydif  ,base+side*width+float3(0,height,0));
						SetArray(TEX_LEAF_END_X1+xdif  ,TEX_LEAF_END_Y4+ydif  ,base-side*width+float3(0,height,0));
						SetArray(TEX_LEAF_END_X1+xdif  ,TEX_LEAF_START_Y4+ydif,base-side*width);
					}
					glNewList(trees[y*treesX+x].farDisplist,GL_COMPILE);
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
					tss->displist=glGenLists(1);

					for(std::map<int,TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
						TreeStruct* ts=&ti->second;
						float3 pos(ts->pos);
						int type=ts->type;

						float height=MAX_TREE_HEIGHT;
						float width=MAX_TREE_HEIGHT*0.5;
						float xdif;
						float ydif;
						if(ts->type<8){
							xdif=type*0.125;
							ydif=0.5;

						} else {
							xdif=(type-8)*0.125;
							ydif=0;
						}
						SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_START_Y1+ydif,pos+float3(width,0,0));
						SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_END_Y1+ydif,pos+float3(width,height,0));
						SetArray(TEX_LEAF_END_X1+xdif,TEX_LEAF_END_Y1+ydif,pos+float3(-width,height,0));
						SetArray(TEX_LEAF_END_X1+xdif,TEX_LEAF_START_Y1+ydif,pos+float3(-width,0,0));

						SetArray(TEX_LEAF_START_X2+xdif,TEX_LEAF_START_Y2+ydif,pos+float3(0,0,width));
						SetArray(TEX_LEAF_START_X2+xdif,TEX_LEAF_END_Y2+ydif,pos+float3(0,height,width));
						SetArray(TEX_LEAF_END_X2+xdif,TEX_LEAF_END_Y2+ydif,pos+float3(0,height,-width));
						SetArray(TEX_LEAF_END_X2+xdif,TEX_LEAF_START_Y2+ydif,pos+float3(0,0,-width));

						//width*=1.41f;
						SetArray(TEX_LEAF_START_X3+xdif,TEX_LEAF_START_Y3+ydif,pos+float3(width,height*0.4,0));
						SetArray(TEX_LEAF_START_X3+xdif,TEX_LEAF_END_Y3+ydif,pos+float3(0,height*0.4,-width));
						SetArray(TEX_LEAF_END_X3+xdif,TEX_LEAF_END_Y3+ydif,pos+float3(-width,height*0.4,0));
						SetArray(TEX_LEAF_END_X3+xdif,TEX_LEAF_START_Y3+ydif,pos+float3(0,height*0.4,width));
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
	}
	if(drawDetailed){
		if(shadowHandler->drawShadows && !(groundDrawer->drawExtraTex)){
			glBindProgramARB( GL_VERTEX_PROGRAM_ARB, treeGen->treeVP );

			glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, treeGen->barkTex);
			glActiveTextureARB(GL_TEXTURE0_ARB);

		} else {
			glBindTexture(GL_TEXTURE_2D, treeGen->barkTex);
			glBindProgramARB( GL_VERTEX_PROGRAM_ARB, treeGen->treeNSVP );
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,15, 1.0/(gs->pwr2mapx*SQUARE_SIZE),1.0/(gs->pwr2mapy*SQUARE_SIZE),1.0/(gs->pwr2mapx*SQUARE_SIZE),1);
		}
		glEnable( GL_VERTEX_PROGRAM_ARB );
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13,  camera->right.x,camera->right.y,camera->right.z,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,9,  camera->up.x,camera->up.y,camera->up.z,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,11, readmap->sunColor.x,readmap->sunColor.y,readmap->sunColor.z,0.85f);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,14, readmap->ambientColor.x,readmap->ambientColor.y,readmap->ambientColor.z,0.85f);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12, 0,0,0,0.20f*(1.0/MAX_TREE_HEIGHT));	//w=alpha/height modifier
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
		int curFade=0;

		for(int y=max(0,cy-2);y<=min(gs->mapy/TREE_SQUARE_SIZE-1,cy+2);++y){	//close trees
			for(int x=max(0,cx-2);x<=min(gs->mapx/TREE_SQUARE_SIZE-1,cx+2);++x){
				TreeSquareStruct* tss=&trees[y*treesX+x];
				tss->lastSeen=gs->frameNum;
				for(std::map<int,TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
					TreeStruct* ts=&ti->second;
					float3 pos(ts->pos);
					int type=ts->type;
					float dy;
					unsigned int displist;
					if(type<8){
						dy=0.5;
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
							glAlphaFunc(GL_GREATER,0.8+relDist*0.2);
							glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10,pos.x,pos.y,pos.z,0);							
							glCallList(displist);
							glAlphaFunc(GL_GREATER,0.5f);
							fadeTrees[curFade].pos=pos;
							fadeTrees[curFade].deltaY=dy;
							fadeTrees[curFade].type=type;
							fadeTrees[curFade++].relDist=relDist;
						} else {																//draw undetailed tree
							float height=MAX_TREE_HEIGHT;
							float width=MAX_TREE_HEIGHT*0.5;
							float xdif=type*0.125;
							SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_START_Y1+dy,pos+float3(width,0,0));
							SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_END_Y1+dy,pos+float3(width,height,0));
							SetArray(TEX_LEAF_END_X1+xdif,TEX_LEAF_END_Y1+dy,pos+float3(-width,height,0));
							SetArray(TEX_LEAF_END_X1+xdif,TEX_LEAF_START_Y1+dy,pos+float3(-width,0,0));

							SetArray(TEX_LEAF_START_X2+xdif,TEX_LEAF_START_Y2+dy,pos+float3(0,0,width));
							SetArray(TEX_LEAF_START_X2+xdif,TEX_LEAF_END_Y2+dy,pos+float3(0,height,width));
							SetArray(TEX_LEAF_END_X2+xdif,TEX_LEAF_END_Y2+dy,pos+float3(0,height,-width));
							SetArray(TEX_LEAF_END_X2+xdif,TEX_LEAF_START_Y2+dy,pos+float3(0,0,-width));

							//width*=1.41f;
							SetArray(TEX_LEAF_START_X3+xdif,TEX_LEAF_START_Y3+dy,pos+float3(width,height*0.4,width));
							SetArray(TEX_LEAF_START_X3+xdif,TEX_LEAF_END_Y3+dy,pos+float3(width,height*0.4,-width));
							SetArray(TEX_LEAF_END_X3+xdif,TEX_LEAF_END_Y3+dy,pos+float3(-width,height*0.4,-width));
							SetArray(TEX_LEAF_END_X3+xdif,TEX_LEAF_START_Y3+dy,pos+float3(-width,height*0.4,width));
						}
					}
				}
			}
			//draw trees that has been marked as falling
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10,0,0,0,0);							
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13,  camera->right.x,camera->right.y,camera->right.z,0);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,9,  camera->up.x,camera->up.y,camera->up.z,0);
			for(std::list<FallingTree>::iterator fti=fallingTrees.begin();fti!=fallingTrees.end();){
				float3 pos=fti->pos-UpVector*(fti->fallPos*20);
				if(camera->InView(pos+float3(0,MAX_TREE_HEIGHT/2,0),MAX_TREE_HEIGHT/2)){
					float ang=fti->fallPos*PI;
					float3 up(fti->dir.x*sin(ang),cos(ang),fti->dir.z*sin(ang));
					float3 z(up.cross(float3(-1,0,0)));
					z.Normalize();
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
				++fti;
			}
		}
		glDisable( GL_VERTEX_PROGRAM_ARB );

		if(shadowHandler->drawShadows && !(groundDrawer->drawExtraTex)){
			glBindProgramARB( GL_VERTEX_PROGRAM_ARB, treeGen->treeFarVP );
			glEnable(GL_VERTEX_PROGRAM_ARB);

			glActiveTextureARB(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, activeFarTex);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		} else {
			glBindTexture(GL_TEXTURE_2D, activeFarTex);
		}
		va->DrawArrayT(GL_QUADS);

		for(int a=0;a<curFade;++a){	//faded close trees
			va=GetVertexArray();
			va->Initialize();
			int type=fadeTrees[a].type;
			float dy=fadeTrees[a].deltaY;

			float height=MAX_TREE_HEIGHT;
			float width=MAX_TREE_HEIGHT*0.5;
			float xdif=type*0.125;
			SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_START_Y1+dy,fadeTrees[a].pos+float3(width,0,0));
			SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_END_Y1+dy,fadeTrees[a].pos+float3(width,height,0));
			SetArray(TEX_LEAF_END_X1+xdif,TEX_LEAF_END_Y1+dy,fadeTrees[a].pos+float3(-width,height,0));
			SetArray(TEX_LEAF_END_X1+xdif,TEX_LEAF_START_Y1+dy,fadeTrees[a].pos+float3(-width,0,0));

			SetArray(TEX_LEAF_START_X2+xdif,TEX_LEAF_START_Y2+dy,fadeTrees[a].pos+float3(0,0,width));
			SetArray(TEX_LEAF_START_X2+xdif,TEX_LEAF_END_Y2+dy,fadeTrees[a].pos+float3(0,height,width));
			SetArray(TEX_LEAF_END_X2+xdif,TEX_LEAF_END_Y2+dy,fadeTrees[a].pos+float3(0,height,-width));
			SetArray(TEX_LEAF_END_X2+xdif,TEX_LEAF_START_Y2+dy,fadeTrees[a].pos+float3(0,0,-width));

			//width*=1.41f;
			SetArray(TEX_LEAF_START_X3+xdif,TEX_LEAF_START_Y3+dy,fadeTrees[a].pos+float3(width,height*0.4,width));
			SetArray(TEX_LEAF_START_X3+xdif,TEX_LEAF_END_Y3+dy,fadeTrees[a].pos+float3(width,height*0.4,-width));
			SetArray(TEX_LEAF_END_X3+xdif,TEX_LEAF_END_Y3+dy,fadeTrees[a].pos+float3(-width,height*0.4,-width));
			SetArray(TEX_LEAF_END_X3+xdif,TEX_LEAF_START_Y3+dy,fadeTrees[a].pos+float3(-width,height*0.4,width));

			glAlphaFunc(GL_GREATER,1-fadeTrees[a].relDist*0.5);
			va->DrawArrayT(GL_QUADS);
		}
	}
	if(shadowHandler->drawShadows && !(groundDrawer->drawExtraTex)){
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

	//clean out squares from memory that are no longer visible
	int startClean=lastListClean*20%(treesX*treesY);
	lastListClean=gs->frameNum;
	int endClean=gs->frameNum*20%(treesX*treesY);

	if(startClean>endClean){
		for(int a=startClean;a<treesX*treesY;a++){
			if(trees[a].lastSeen<gs->frameNum-50 && trees[a].displist){
				glDeleteLists(trees[a].displist,1);
				trees[a].displist=0;
			}
			if(trees[a].lastSeenFar<gs->frameNum-50 && trees[a].farDisplist){
				glDeleteLists(trees[a].farDisplist,1);
				trees[a].farDisplist=0;
			}
		}
		for(int a=0;a<endClean;a++){
			if(trees[a].lastSeen<gs->frameNum-50 && trees[a].displist){
				glDeleteLists(trees[a].displist,1);
				trees[a].displist=0;
			}
			if(trees[a].lastSeenFar<gs->frameNum-50 && trees[a].farDisplist){
				glDeleteLists(trees[a].farDisplist,1);
				trees[a].farDisplist=0;
			}
		}
	} else {
		for(int a=startClean;a<endClean;a++){
			if(trees[a].lastSeen<gs->frameNum-50 && trees[a].displist){
				glDeleteLists(trees[a].displist,1);
				trees[a].displist=0;
			}
			if(trees[a].lastSeenFar<gs->frameNum-50 && trees[a].farDisplist){
				glDeleteLists(trees[a].farDisplist,1);
				trees[a].farDisplist=0;
			}
		}
	}
}

void CAdvTreeDrawer::DrawShadowPass(void)
{
	int cx=(int)(camera->pos.x/(SQUARE_SIZE*TREE_SQUARE_SIZE));
	int cy=(int)(camera->pos.z/(SQUARE_SIZE*TREE_SQUARE_SIZE));
	
	float treeDistance=oldTreeDistance;
	int treeSquare=int(treeDistance*2)+1;
	
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

	int sy=cy-treeSquare;
	if(sy<0)
		sy=0;
	int ey=cy+treeSquare;
	if(ey>=treesY)
		ey=treesY-1;

	for(int y=sy;y<=ey;y++){
		int sx=cx-treeSquare;
		if(sx<0)
			sx=0;
		int ex=cx+treeSquare;
		if(ex>=treesX)
			ex=treesX-1;
		float xtest,xtest2;
		std::vector<CBaseGroundDrawer::fline>::iterator fli;
		for(fli=groundDrawer->left.begin();fli!=groundDrawer->left.end();fli++){
			xtest=((fli->base/SQUARE_SIZE+fli->dir*(y*TREE_SQUARE_SIZE)));
			xtest2=((fli->base/SQUARE_SIZE+fli->dir*((y*TREE_SQUARE_SIZE)+TREE_SQUARE_SIZE)));
			if(xtest>xtest2)
				xtest=xtest2;
			xtest=xtest/TREE_SQUARE_SIZE;
			if(xtest-1>sx)//increse visible trees somewhat
				sx=(int)xtest-1;
		}
		for(fli=groundDrawer->right.begin();fli!=groundDrawer->right.end();fli++){
			xtest=((fli->base/SQUARE_SIZE+fli->dir*(y*TREE_SQUARE_SIZE)));
			xtest2=((fli->base/SQUARE_SIZE+fli->dir*((y*TREE_SQUARE_SIZE)+TREE_SQUARE_SIZE)));
			if(xtest<xtest2)
				xtest=xtest2;
			xtest=xtest/TREE_SQUARE_SIZE;
			if(xtest+1<ex)//increse visible trees somewhat
				ex=(int)xtest+1;
		}
		for(int x=sx;x<=ex;x++){
			TreeSquareStruct* tss=&trees[y*treesX+x];
			if(abs(cy-y)<=2 && abs(cx-x)<=2 && drawDetailed)	//skip the closest squares
				continue;

			float3 dif;
			dif.x=camera->pos.x-(x*SQUARE_SIZE*TREE_SQUARE_SIZE + SQUARE_SIZE*TREE_SQUARE_SIZE/2);
			dif.y=0;
			dif.z=camera->pos.z-(y*SQUARE_SIZE*TREE_SQUARE_SIZE + SQUARE_SIZE*TREE_SQUARE_SIZE/2);
			float dist=dif.Length();
			dif/=dist;
			
			if(dist<SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance*2 && dist>SQUARE_SIZE*TREE_SQUARE_SIZE*(treeDistance)){//far trees
				tss->lastSeenFar=gs->frameNum;
				if(!tss->farDisplist || dif.dot(tss->viewVector)<0.97){
					va=GetVertexArray();
					va->Initialize();
					tss->viewVector=dif;
					if(!tss->farDisplist)
						tss->farDisplist=glGenLists(1);
					float3 up(0,1,0);
					float3 side=up.cross(dif);

					for(std::map<int,TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
						TreeStruct* ts=&ti->second;

						float3 base(ts->pos);
						int type=ts->type;

						float height=MAX_TREE_HEIGHT;
						float width=MAX_TREE_HEIGHT*0.5;
	
						float xdif;
						float ydif;
						if(ts->type<8){
							xdif=type*0.125;
							ydif=0.5;
						} else {
							xdif=(type-8)*0.125;
							ydif=0;
						}
						SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_START_Y4+ydif,base+side*width);
						SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_END_Y4+ydif  ,base+side*width+float3(0,height,0));
						SetArray(TEX_LEAF_END_X1+xdif  ,TEX_LEAF_END_Y4+ydif  ,base-side*width+float3(0,height,0));
						SetArray(TEX_LEAF_END_X1+xdif  ,TEX_LEAF_START_Y4+ydif,base-side*width);
					}
					glNewList(trees[y*treesX+x].farDisplist,GL_COMPILE);
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
					tss->displist=glGenLists(1);

					for(std::map<int,TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
						TreeStruct* ts=&ti->second;
						float3 pos(ts->pos);
						int type=ts->type;

						float height=MAX_TREE_HEIGHT;
						float width=MAX_TREE_HEIGHT*0.5;
						float xdif;
						float ydif;
						if(ts->type<8){
							xdif=type*0.125;
							ydif=0.5;

						} else {
							xdif=(type-8)*0.125;
							ydif=0;
						}
						SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_START_Y1+ydif,pos+float3(width,0,0));
						SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_END_Y1+ydif,pos+float3(width,height,0));
						SetArray(TEX_LEAF_END_X1+xdif,TEX_LEAF_END_Y1+ydif,pos+float3(-width,height,0));
						SetArray(TEX_LEAF_END_X1+xdif,TEX_LEAF_START_Y1+ydif,pos+float3(-width,0,0));

						SetArray(TEX_LEAF_START_X2+xdif,TEX_LEAF_START_Y2+ydif,pos+float3(0,0,width));
						SetArray(TEX_LEAF_START_X2+xdif,TEX_LEAF_END_Y2+ydif,pos+float3(0,height,width));
						SetArray(TEX_LEAF_END_X2+xdif,TEX_LEAF_END_Y2+ydif,pos+float3(0,height,-width));
						SetArray(TEX_LEAF_END_X2+xdif,TEX_LEAF_START_Y2+ydif,pos+float3(0,0,-width));

						//width*=1.41f;
						SetArray(TEX_LEAF_START_X3+xdif,TEX_LEAF_START_Y3+ydif,pos+float3(width,height*0.4,0));
						SetArray(TEX_LEAF_START_X3+xdif,TEX_LEAF_END_Y3+ydif,pos+float3(0,height*0.4,-width));
						SetArray(TEX_LEAF_END_X3+xdif,TEX_LEAF_END_Y3+ydif,pos+float3(-width,height*0.4,0));
						SetArray(TEX_LEAF_END_X3+xdif,TEX_LEAF_START_Y3+ydif,pos+float3(0,height*0.4,width));
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
	}
	if(drawDetailed){
		glBindTexture(GL_TEXTURE_2D, treeGen->barkTex);
		glEnable(GL_TEXTURE_2D);
		glBindProgramARB( GL_VERTEX_PROGRAM_ARB, treeGen->treeShadowVP );
		glEnable( GL_VERTEX_PROGRAM_ARB );
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13,  camera->right.x,camera->right.y,camera->right.z,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,9,  camera->up.x,camera->up.y,camera->up.z,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,11, 1,1,1,0.85f);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12, 0,0,0,0.20f*(1.0/MAX_TREE_HEIGHT));	//w=alpha/height modifier
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
		int curFade=0;

		for(int y=max(0,cy-2);y<=min(gs->mapy/TREE_SQUARE_SIZE-1,cy+2);++y){	//close trees
			for(int x=max(0,cx-2);x<=min(gs->mapx/TREE_SQUARE_SIZE-1,cx+2);++x){
				TreeSquareStruct* tss=&trees[y*treesX+x];
				tss->lastSeen=gs->frameNum;
				for(std::map<int,TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
					TreeStruct* ts=&ti->second;
					float3 pos(ts->pos);
					int type=ts->type;
					float dy;
					unsigned int displist;
					if(type<8){
						dy=0.5;
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
							glAlphaFunc(GL_GREATER,0.8+relDist*0.2);
							glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10,pos.x,pos.y,pos.z,0);							
							glCallList(displist);
							glAlphaFunc(GL_GREATER,0.5f);
							fadeTrees[curFade].pos=pos;
							fadeTrees[curFade].deltaY=dy;
							fadeTrees[curFade].type=type;
							fadeTrees[curFade++].relDist=relDist;
						} else {																//draw undetailed tree
							float height=MAX_TREE_HEIGHT;
							float width=MAX_TREE_HEIGHT*0.5;
							float xdif=type*0.125;
							SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_START_Y1+dy,pos+float3(width,0,0));
							SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_END_Y1+dy,pos+float3(width,height,0));
							SetArray(TEX_LEAF_END_X1+xdif,TEX_LEAF_END_Y1+dy,pos+float3(-width,height,0));
							SetArray(TEX_LEAF_END_X1+xdif,TEX_LEAF_START_Y1+dy,pos+float3(-width,0,0));

							SetArray(TEX_LEAF_START_X2+xdif,TEX_LEAF_START_Y2+dy,pos+float3(0,0,width));
							SetArray(TEX_LEAF_START_X2+xdif,TEX_LEAF_END_Y2+dy,pos+float3(0,height,width));
							SetArray(TEX_LEAF_END_X2+xdif,TEX_LEAF_END_Y2+dy,pos+float3(0,height,-width));
							SetArray(TEX_LEAF_END_X2+xdif,TEX_LEAF_START_Y2+dy,pos+float3(0,0,-width));

							//width*=1.41f;
							SetArray(TEX_LEAF_START_X3+xdif,TEX_LEAF_START_Y3+dy,pos+float3(width,height*0.4,width));
							SetArray(TEX_LEAF_START_X3+xdif,TEX_LEAF_END_Y3+dy,pos+float3(width,height*0.4,-width));
							SetArray(TEX_LEAF_END_X3+xdif,TEX_LEAF_END_Y3+dy,pos+float3(-width,height*0.4,-width));
							SetArray(TEX_LEAF_END_X3+xdif,TEX_LEAF_START_Y3+dy,pos+float3(-width,height*0.4,width));
						}
					}
				}
			}
		}
		//draw trees that has been marked as falling
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10,0,0,0,0);							
		for(std::list<FallingTree>::iterator fti=fallingTrees.begin();fti!=fallingTrees.end();){
			float3 pos=fti->pos-UpVector*(fti->fallPos*20);
			if(camera->InView(pos+float3(0,MAX_TREE_HEIGHT/2,0),MAX_TREE_HEIGHT/2)){
				float ang=fti->fallPos*PI;
				float3 up(fti->dir.x*sin(ang),cos(ang),fti->dir.z*sin(ang));
				float3 z(up.cross(float3(1,0,0)));
				z.Normalize();
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
			++fti;
		}
		glBindProgramARB( GL_VERTEX_PROGRAM_ARB, treeGen->treeFarShadowVP );
		glBindTexture(GL_TEXTURE_2D, activeFarTex);
		va->DrawArrayT(GL_QUADS);

		for(int a=0;a<curFade;++a){	//faded close trees
			va=GetVertexArray();
			va->Initialize();
			int type=fadeTrees[a].type;
			float dy=fadeTrees[a].deltaY;

			float height=MAX_TREE_HEIGHT;
			float width=MAX_TREE_HEIGHT*0.5;
			float xdif=type*0.125;
			SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_START_Y1+dy,fadeTrees[a].pos+float3(width,0,0));
			SetArray(TEX_LEAF_START_X1+xdif,TEX_LEAF_END_Y1+dy,fadeTrees[a].pos+float3(width,height,0));
			SetArray(TEX_LEAF_END_X1+xdif,TEX_LEAF_END_Y1+dy,fadeTrees[a].pos+float3(-width,height,0));
			SetArray(TEX_LEAF_END_X1+xdif,TEX_LEAF_START_Y1+dy,fadeTrees[a].pos+float3(-width,0,0));

			SetArray(TEX_LEAF_START_X2+xdif,TEX_LEAF_START_Y2+dy,fadeTrees[a].pos+float3(0,0,width));
			SetArray(TEX_LEAF_START_X2+xdif,TEX_LEAF_END_Y2+dy,fadeTrees[a].pos+float3(0,height,width));
			SetArray(TEX_LEAF_END_X2+xdif,TEX_LEAF_END_Y2+dy,fadeTrees[a].pos+float3(0,height,-width));
			SetArray(TEX_LEAF_END_X2+xdif,TEX_LEAF_START_Y2+dy,fadeTrees[a].pos+float3(0,0,-width));

			//width*=1.41f;
			SetArray(TEX_LEAF_START_X3+xdif,TEX_LEAF_START_Y3+dy,fadeTrees[a].pos+float3(width,height*0.4,width));
			SetArray(TEX_LEAF_START_X3+xdif,TEX_LEAF_END_Y3+dy,fadeTrees[a].pos+float3(width,height*0.4,-width));
			SetArray(TEX_LEAF_END_X3+xdif,TEX_LEAF_END_Y3+dy,fadeTrees[a].pos+float3(-width,height*0.4,-width));
			SetArray(TEX_LEAF_END_X3+xdif,TEX_LEAF_START_Y3+dy,fadeTrees[a].pos+float3(-width,height*0.4,width));

			glAlphaFunc(GL_GREATER,1-fadeTrees[a].relDist*0.5);
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
	int a=y*treesX+x;
	if(trees[a].displist){
		glDeleteLists(trees[a].displist,1);
		trees[a].displist=0;
	}
	if(trees[a].farDisplist){
		glDeleteLists(trees[a].farDisplist,1);
		trees[a].farDisplist=0;
	}
	grassDrawer->ResetPos(pos);
}

void CAdvTreeDrawer::AddTree(int type, float3 pos, float size)
{
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
	int hash=(int)pos.x+((int)(pos.z))*20000;
	int square=((int)pos.x)/(SQUARE_SIZE*TREE_SQUARE_SIZE)+((int)pos.z)/(SQUARE_SIZE*TREE_SQUARE_SIZE)*treesX;

	trees[square].trees.erase(hash);

	ResetPos(pos);
}

int CAdvTreeDrawer::AddFallingTree(float3 pos, float3 dir, int type)
{
	FallingTree ft;

	ft.pos=pos;
	dir.y=0;
	float s=dir.Length();
	if(s>500)
		return 0;
	ft.dir=dir/s;
	ft.speed=max(0.01,s*0.0004);
	ft.type=type;
	ft.fallPos=0;

	fallingTrees.push_back(ft);
	return 0;
}

void CAdvTreeDrawer::AddGrass(float3 pos)
{
	grassDrawer->AddGrass(pos);
}

void CAdvTreeDrawer::RemoveGrass(int x, int z)
{
	grassDrawer->RemoveGrass(x,z);
}

