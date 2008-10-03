#include "StdAfx.h"
#include <algorithm>
#include "mmgr.h"

#include "GrassDrawer.h"
#include "Map/Ground.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Map/ReadMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Rendering/GL/myGL.h"
#include "AdvTreeDrawer.h"
#include "Rendering/Textures/Bitmap.h"
#include "LogOutput.h"
#include "myMath.h"
#include "Platform/ConfigHandler.h"
#include "FileSystem/FileHandler.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/ShadowHandler.h"
//#include "TimeProfiler.h"

static const float turfSize=20;				//single turf size
static const float partTurfSize=turfSize*0.6f;				//single turf size
static const int grassSquareSize=4;		//mapsquares per grass square
static const int grassBlockSize=4;		//grass squares per grass block
static const int gSSsq=SQUARE_SIZE*grassSquareSize;
static const int blockMapSize=grassSquareSize*grassBlockSize;
static const int bMSsq=SQUARE_SIZE*blockMapSize;


inline float fRand(float size)
{
	return float(rand())/RAND_MAX*size;
}

CGrassDrawer::CGrassDrawer()
{
	int detail=configHandler.GetInt("GrassDetail",3);

	if(detail==0){
		grassOff=true;
		return;
	} else{
		grassOff=false;
	}
	MapBitmapInfo grassbm;
	unsigned char* grassdata=readmap->GetInfoMap("grass", &grassbm);
	if (grassdata)
	{
		if (grassbm.width != gs->mapx/grassSquareSize || grassbm.height != gs->mapy/grassSquareSize) {
			char b[128];
			SNPRINTF(b, sizeof(b), "Grass map has wrong size (%dx%d, should be %dx%d)\n", 
				grassbm.width, grassbm.height, gs->mapx/4, gs->mapy/4);
			throw std::runtime_error (b);
		}

		int grassMapSize = gs->mapx*gs->mapy/(grassSquareSize*grassSquareSize);
		grassMap=SAFE_NEW unsigned char[grassMapSize];

		memcpy(grassMap, grassdata, grassMapSize);
		readmap->FreeInfoMap ("grass", grassdata);
	} else {
		grassOff=true;
		return;
	}

	maxGrassDist=800+sqrt((float)detail)*240;
	maxDetailedDist=146+detail*24;
	detailedBlocks=(int)((maxDetailedDist-24)/bMSsq)+1;
	numTurfs=3+(int)(detail*0.5f);
	strawPerTurf=50+(int)sqrt((float)detail)*10;

//	logOutput.Print("%f %f %i %i %i",maxGrassDist,maxDetailedDist,detailedBlocks,numTurfs,strawPerTurf);

	blocksX=gs->mapx/grassSquareSize/grassBlockSize;
	blocksY=gs->mapy/grassSquareSize/grassBlockSize;

	for(int y=0;y<32;y++){
		for(int x=0;x<32;x++){
			grass[y*32+x].va=0;
			grass[y*32+x].lastSeen=0;
			grass[y*32+x].pos=ZeroVector;
			grass[y*32+x].square=0;
		}
	}
	for(int y=0;y<32;y++){
		for(int x=0;x<32;x++){
			nearGrass[y*32+x].square=-1;
		}
	}
	lastListClean=0;

	grassFarNSVP=LoadVertexProgram("grassFarNS.vp");

	grassDL=glGenLists(8);
	srand(15);
	for(int a=0;a<1;++a){
		CreateGrassDispList(grassDL+a);
	}
	unsigned char gbt[64*256*4];
	for(int a=0;a<16;++a){
		CreateGrassBladeTex(&gbt[a*16*4]);
	}
	glGenTextures(1, &grassBladeTex);
	glBindTexture(GL_TEXTURE_2D, grassBladeTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glBuildMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,256, 64, GL_RGBA, GL_UNSIGNED_BYTE, gbt);

	CreateFarTex();

	if(shadowHandler->canUseShadows){
		grassVP=LoadVertexProgram("grass.vp");
		grassFarVP=LoadVertexProgram("grassFar.vp");
	}
}

CGrassDrawer::~CGrassDrawer(void)
{
	if(grassOff)
		return;

	for(int y=0;y<32;y++){
		for(int x=0;x<32;x++){
			if(grass[y*32+x].va)
				delete grass[y*32+x].va;
		}
	}
	delete[] grassMap;
	glDeleteLists(grassDL,8);
	glDeleteTextures(1,&grassBladeTex);
	glDeleteTextures(1,&farTex);	

	glSafeDeleteProgram( grassFarNSVP );

	if(shadowHandler->canUseShadows){
		glSafeDeleteProgram( grassVP );
		glSafeDeleteProgram( grassFarVP );
	}
}

struct InviewGrass {
	int num;
	float dist;
};

struct InviewNearGrass {
	int x;
	int y;
	float dist;
};

static const bool GrassSort(const InviewGrass& a, const InviewGrass& b){
	return a.dist > b.dist;
}

static const bool GrassSortNear(const InviewNearGrass& a, const InviewNearGrass& b){
	return a.dist > b.dist;
}

class CGrassBlockDrawer : public CReadMap::IQuadDrawer
{
public:
	std::vector<InviewGrass> inviewGrass;
	std::vector<InviewNearGrass> inviewNearGrass;
	CVertexArray* va;
	int cx,cy;
	CGrassDrawer *gd;

	void DrawQuad (int x,int y);
};

void CGrassBlockDrawer::DrawQuad (int x,int y)
{
	float maxDetailedDist = gd->maxDetailedDist;
	CGrassDrawer::NearGrassStruct *nearGrass = gd->nearGrass;

	int xgbs=x*grassBlockSize;
	int ygbs=y*grassBlockSize;

	if(abs(x-cx)<=gd->detailedBlocks && abs(y-cy)<=gd->detailedBlocks){	//!blocks close to the camera
		int ygbsy=ygbs;
		for(int y2=0; y2<grassBlockSize; ++y2){
			int xgbsx=xgbs;
			unsigned char* gm=gd->grassMap+ygbsy*gs->mapx/grassSquareSize+xgbsx;
			for(int x2=0; x2<grassBlockSize; ++x2){ //!loop over all squares in block
				if(*gm) {
					float3 squarePos((xgbsx+0.5f)*gSSsq, 0, (ygbsy+0.5f)*gSSsq);
					squarePos.y=ground->GetHeight2(squarePos.x,squarePos.z);
					if(camera->InView(squarePos,gSSsq)){
						float sqdist=(camera->pos-squarePos).SqLength();
						if(sqdist<maxDetailedDist*maxDetailedDist){ //!close grass, draw directly
							srand(ygbsy*1025+xgbsx);
							rand();
							rand();
							int numGrass=gd->numTurfs;
							for(int a=0;a<numGrass;a++){
								float dx=(xgbsx+fRand(1))*gSSsq;
								float dy=(ygbsy+fRand(1))*gSSsq;
								float3 pos(dx,ground->GetHeight2(dx,dy),dy);
								pos.y-=ground->GetSlope(dx,dy)*10+0.03f;
								float col=0.62f;
								glColor3f(col,col,col);
								if(camera->InView(pos,turfSize*0.7f)){
									glPushMatrix();
									glTranslatef3(pos);
									CGrassDrawer::NearGrassStruct* ng=&nearGrass[(ygbsy&31)*32+(xgbsx&31)];
									if(ng->square!=ygbsy*2048+xgbsx){
										float3 v=squarePos-camera->pos;
										ng->rotation=GetHeadingFromVector(v.x,v.z)*180.0f/32768+180;
										ng->square=ygbsy*2048+xgbsx;
									}
									glRotatef(ng->rotation,0,1,0);
									glCallList(gd->grassDL);
									glPopMatrix();
								}
							}
						} else {//near but not close, save for later drawing
							InviewNearGrass iv;
							iv.dist=sqdist;
							iv.x=xgbsx;
							iv.y=ygbsy;
							inviewNearGrass.push_back(iv);
							nearGrass[(ygbsy&31)*32+(xgbsx&31)].square=-1;
						}
					}
				}
				++gm;
				++xgbsx;
			}
			++ygbsy;
		}
		return;
	}

	float3 dif;
	dif.x=camera->pos.x-((x+0.5f)*bMSsq);
	dif.y=0;
	dif.z=camera->pos.z-((y+0.5f)*bMSsq);
	float dist=dif.Length2D();
	dif/=dist;
				
	if(dist<gd->maxGrassDist){
		int curSquare=y*gd->blocksX+x;
		int curModSquare=(y&31)*32+(x&31);
		CGrassDrawer::GrassStruct *grass = gd->grass+curModSquare;
		grass->lastSeen=gs->frameNum;
		if(grass->square!=curSquare){
			grass->square=curSquare;
			if(grass->va){
				delete grass->va;
				grass->va=0;
			}
		}
		if(!grass->va){
			grass->va=SAFE_NEW CVertexArray;;
			grass->pos=float3((x+0.5f)*bMSsq,ground->GetHeight2((x+0.5f)*bMSsq,(y+0.5f)*bMSsq),(y+0.5f)*bMSsq);
			va=grass->va;
			va->Initialize();
			int numGrass=gd->numTurfs;
			va->EnlargeArrays(grassBlockSize*grassBlockSize*numGrass*4,0,VA_SIZE_TN);
			int ygbsy=ygbs;
			for(int y2=0; y2<grassBlockSize; ++y2){ //! CAUTION: loop count must match EnlargeArrays above
				int xgbsx=xgbs;
				unsigned char* gm=gd->grassMap+ygbsy*gs->mapx/grassSquareSize+xgbsx;
				for(int x2=0; x2<grassBlockSize; ++x2){ //! CAUTION: loop count must match EnlargeArrays above
					if(*gm){
						srand(ygbsy*1025+xgbsx);
						rand();
						rand();
						for(int a=0;a<numGrass;a++){ //! CAUTION: loop count must match EnlargeArrays above
							float dx=(xgbsx+fRand(1))*gSSsq;
							float dy=(ygbsy+fRand(1))*gSSsq;
							float3 pos(dx,ground->GetHeight2(dx,dy)+0.5f,dy);
							float col=1;

							pos.y-=ground->GetSlope(dx,dy)*10+0.03f;
							va->AddVertexQTN(pos,0,0,float3(-partTurfSize,-partTurfSize,col));
							va->AddVertexQTN(pos,1/16.0f,0,float3(partTurfSize,-partTurfSize,col));
							va->AddVertexQTN(pos,1/16.0f,1,float3(partTurfSize,partTurfSize,col));
							va->AddVertexQTN(pos,0,1,float3(-partTurfSize,partTurfSize,col));
						}
					}
					++gm;
					++xgbsx;
				}
				++ygbsy;
			}
		}
		InviewGrass ig;
		ig.num=curModSquare;
		ig.dist=dist;
		inviewGrass.push_back(ig);	
	}
}

void CGrassDrawer::Draw(void)
{
	if(grassOff || !readmap->GetGrassShadingTexture())
		return;

	glColor4f(0.62f,0.62f,0.62f,1);

	CBaseGroundDrawer *gd = readmap->GetGroundDrawer ();

	if(shadowHandler->drawShadows && !gd->DrawExtraTex()){
		glBindProgramARB( GL_VERTEX_PROGRAM_ARB, grassVP );
		glEnable(GL_VERTEX_PROGRAM_ARB);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13, 1.0f/(gs->pwr2mapx*SQUARE_SIZE),1.0f/(gs->pwr2mapy*SQUARE_SIZE),0,1);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,14, 1.0f/(gs->mapx*SQUARE_SIZE),1.0f/(gs->mapy*SQUARE_SIZE),0,1);

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, readmap->GetShadingTexture ());
		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB,GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB,GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_ARB,GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB_ARB,GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE_ARB,2);

		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_ARB,GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_MODULATE);

		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);

		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D,  shadowHandler->shadowTexture);
		glEnable(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_ALPHA);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FAIL_VALUE_ARB, 1-mapInfo->light.groundShadowDensity);
		
		float texConstant[]={mapInfo->light.groundAmbientColor.x*1.24f,mapInfo->light.groundAmbientColor.y*1.24f,mapInfo->light.groundAmbientColor.z*1.24f,1};
		glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,texConstant);
		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB,GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB,GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_RGB_ARB,GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_RGB_ARB,GL_SRC_ALPHA);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_INTERPOLATE_ARB);

		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_ARB,GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_MODULATE);

		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);

		glActiveTextureARB(GL_TEXTURE2_ARB);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glBindTexture(GL_TEXTURE_2D, readmap->GetGrassShadingTexture());

		glActiveTextureARB(GL_TEXTURE3_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, grassBladeTex);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

		glActiveTextureARB(GL_TEXTURE0_ARB);

		glMatrixMode(GL_MATRIX0_ARB);
		glLoadMatrixf(shadowHandler->shadowMatrix.m);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glMultMatrixd(camera->GetModelview());
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	} else {
		if(gd->DrawExtraTex()){
			glActiveTextureARB(GL_TEXTURE3_ARB);
			glEnable(GL_TEXTURE_2D);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_ARB,GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);

			SetTexGen(1.0f/(gs->pwr2mapx*SQUARE_SIZE),1.0f/(gs->pwr2mapy*SQUARE_SIZE),0,0);

			glBindTexture(GL_TEXTURE_2D, gd->infoTex);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, grassBladeTex);

		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		SetTexGen(1.0f/(gs->mapx*SQUARE_SIZE),1.0f/(gs->mapy*SQUARE_SIZE),0,0);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glBindTexture(GL_TEXTURE_2D, readmap->GetGrassShadingTexture());

		glActiveTextureARB(GL_TEXTURE2_ARB);
		glEnable(GL_TEXTURE_2D);
		SetTexGen(1.0f/(gs->pwr2mapx*SQUARE_SIZE),1.0f/(gs->pwr2mapy*SQUARE_SIZE),0,0);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glBindTexture(GL_TEXTURE_2D, readmap->GetShadingTexture ());

		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB,GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB,GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE_ARB,2);

		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);

		glActiveTextureARB(GL_TEXTURE0_ARB);
	}
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_FOG);
	glFogfv(GL_FOG_COLOR,mapInfo->atmosphere.fogColor);

	CGrassBlockDrawer drawer;

	drawer.cx=(int)(camera->pos.x/bMSsq);
	drawer.cy=(int)(camera->pos.z/bMSsq);
	drawer.gd = this;

	readmap->GridVisibility (camera, blockMapSize, maxGrassDist, &drawer);
	CVertexArray *va = drawer.va;
	
	float grassDistance=maxGrassDist;

	std::sort(drawer.inviewGrass.begin(),drawer.inviewGrass.end(),GrassSort);
	std::sort(drawer.inviewNearGrass.begin(),drawer.inviewNearGrass.end(),GrassSortNear);
	
	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable( GL_VERTEX_PROGRAM_ARB );
	glAlphaFunc(GL_GREATER,0.01f);
	glDepthMask(false);

	if(shadowHandler->drawShadows && !gd->DrawExtraTex()){
		glActiveTextureARB(GL_TEXTURE3_ARB);
		glBindTexture(GL_TEXTURE_2D, farTex);
		glActiveTextureARB(GL_TEXTURE0_ARB);

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		glBindProgramARB( GL_VERTEX_PROGRAM_ARB, grassFarVP );
	} else {
		glBindProgramARB( GL_VERTEX_PROGRAM_ARB, grassFarNSVP );
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12,  1.0f/(gs->mapx*SQUARE_SIZE),1.0f/(gs->mapy*SQUARE_SIZE),0,1);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13,  1.0f/(gs->pwr2mapx*SQUARE_SIZE),1.0f/(gs->pwr2mapy*SQUARE_SIZE),0,1);
		glBindTexture(GL_TEXTURE_2D, farTex);

		if(gd->DrawExtraTex()){
			glActiveTextureARB(GL_TEXTURE3_ARB);
			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	for (std::vector<InviewGrass>::iterator gi = drawer.inviewGrass.begin(); gi != drawer.inviewGrass.end(); ++gi) {
		if((*gi).dist+128<grassDistance)
			glColor4f(0.62f,0.62f,0.62f,1);			
		else
			glColor4f(0.62f,0.62f,0.62f,1-((*gi).dist+128-grassDistance)/128.0f);			

		float3 v=grass[(*gi).num].pos-camera->pos;
		v.Normalize();
		float3 side(v.cross(UpVector));
		side.Normalize();
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,8,  side.x,side.y,side.z,0);
		float3 up(side.cross(v));
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,9,  up.x,up.y,up.z,0);
		float ang=acos(v.y);
		int texPart=std::min(15,(int)std::max(0,(int)((ang+PI/16-PI/2)/PI*30)));
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10, texPart/16.0f,0,0,0.0f);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,11,  -v.x,-v.y,-v.z,0);
		grass[(*gi).num].va->DrawArrayTN(GL_QUADS);
	}
	glColor4f(0.62f,0.62f,0.62f,1);			
	for(std::vector<InviewNearGrass>::iterator gi = drawer.inviewNearGrass.begin(); gi != drawer.inviewNearGrass.end(); ++gi) {
		int x = (*gi).x;
		int y = (*gi).y;
		if(grassMap[(y)*gs->mapx/grassSquareSize+(x)]){

			float3 squarePos((x+0.5f)*gSSsq, 0, (y+0.5f)*gSSsq);
			squarePos.y=ground->GetHeight2(squarePos.x,squarePos.z);
			float3 v=squarePos-camera->pos;
			v.Normalize();
			float3 side(v.cross(UpVector));
			side.Normalize();
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,8,  side.x,side.y,side.z,0);
			float3 up(side.cross(v));
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,9,  up.x,up.y,up.z,0);
			float ang=acos(v.y);
			int texPart=std::min(15,int(std::max(0,int((ang+PI/16-PI/2)/PI*30))));
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10, texPart/16.0f,0,0,0.0f);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,11,  -v.x,-v.y,-v.z,0);

			srand(y*1025+x);
			rand();
			rand();
			int numGrass=numTurfs;
			va=GetVertexArray();
			va->Initialize();
			va->EnlargeArrays(numGrass*4,0,VA_SIZE_TN);
			for(int a=0;a<numGrass;a++){ //! CAUTION: loop count must match EnlargeArrays above
				float dx=(x+fRand(1))*gSSsq;
				float dy=(y+fRand(1))*gSSsq;
				float3 pos(dx,ground->GetHeight2(dx,dy)+0.5f,dy);
				pos.y-=ground->GetSlope(dx,dy)*10+0.03f;
				float col=1;
				if(camera->InView(pos,turfSize*0.7f)){
					va->AddVertexQTN(pos,0,0,float3(-partTurfSize,-partTurfSize,col));
					va->AddVertexQTN(pos,1/16.0f,0,float3(partTurfSize,-partTurfSize,col));
					va->AddVertexQTN(pos,1/16.0f,1,float3(partTurfSize,partTurfSize,col));
					va->AddVertexQTN(pos,0,1,float3(-partTurfSize,partTurfSize,col));
				}
			}
			va->DrawArrayTN(GL_QUADS);
		}
	}

	//cleanup stuff
	glDisable( GL_VERTEX_PROGRAM_ARB );
	glDepthMask(true);
	glEnable(GL_FOG);
	glDisable(GL_ALPHA_TEST);

	if(shadowHandler->drawShadows && !gd->DrawExtraTex()){
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE_ARB,1);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
		glActiveTextureARB(GL_TEXTURE3_ARB);
		glDisable(GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	if(gd->DrawExtraTex()){
		glActiveTextureARB(GL_TEXTURE3_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glDisable(GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_2D);
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glDisable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE_ARB,1);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glActiveTextureARB(GL_TEXTURE0_ARB);


	int startClean=lastListClean*20%(32*32);
	lastListClean=gs->frameNum;
	int endClean=gs->frameNum*20%(32*32);
	if(startClean>endClean){
		for(GrassStruct *pGS=grass+startClean; pGS<grass+32*32; ++pGS) {
			if(pGS->lastSeen<gs->frameNum-50 && pGS->va){
				delete pGS->va;
				pGS->va=0;
			}
		}
		for(GrassStruct *pGS=grass; pGS<grass+endClean; ++pGS) {
			if(pGS->lastSeen<gs->frameNum-50 && pGS->va){
				delete pGS->va;
				pGS->va=0;
			}
		}
	} else {
		for(GrassStruct *pGS=grass+startClean; pGS<grass+endClean; ++pGS) {
			if(pGS->lastSeen<gs->frameNum-50 && pGS->va){
				delete pGS->va;
				pGS->va=0;
			}
		}
	}
}

void CGrassDrawer::ResetPos(const float3& pos)
{
	if(grassOff)
		return;
	int a=(int(pos.z/bMSsq)&31)*32+(int(pos.x/bMSsq)&31);
	if(grass[a].va){
		delete grass[a].va;
		grass[a].va=0;
	}
	grass[a].square=-1;
}

void CGrassDrawer::CreateGrassDispList(int listNum)
{
	CVertexArray* va=GetVertexArray();
	va->Initialize();


	for(int a=0;a<strawPerTurf;++a){
		float maxAng=fRand(PI/2);
		float3 sideVect(fRand(1)-0.5f,0,fRand(1)-0.5f);
		sideVect.Normalize();
		float3 forwardVect=sideVect.cross(UpVector);
		sideVect*=0.32f;

		float3 basePos(30,0,30);
		while(basePos.SqLength2D()>turfSize*turfSize/4)
			basePos=float3(fRand(turfSize)-turfSize*0.5f,0,fRand(turfSize)-turfSize*0.5f);

		float length=4+fRand(4);

		int tex=(int)fRand(15.9999f);
		float xtexBase=tex*(1/16.0f);
		int numSections=1+(int)(maxAng*5);
		for(int b=0;b<numSections;++b){
			float h=b*(1.0f/numSections);
			float ang=maxAng*h;
			if(b==0){
				va->AddVertexT(basePos+sideVect*(1-h)+(UpVector*cos(ang)+forwardVect*sin(ang))*length*h-float3(0,0.1f,0), xtexBase, h);
				va->AddVertexT(basePos+sideVect*(1-h)+(UpVector*cos(ang)+forwardVect*sin(ang))*length*h-float3(0,0.1f,0), xtexBase, h);
			} else {
				va->AddVertexT(basePos+sideVect*(1-h)+(UpVector*cos(ang)+forwardVect*sin(ang))*length*h, xtexBase, h);
			}
			va->AddVertexT(basePos-sideVect*(1-h)+(UpVector*cos(ang)+forwardVect*sin(ang))*length*h, xtexBase+1.0f/16, h);
		}
		va->AddVertexT(basePos+(UpVector*cos(maxAng)+forwardVect*sin(maxAng))*length, xtexBase+1.0f/32, 1);
		va->AddVertexT(basePos+(UpVector*cos(maxAng)+forwardVect*sin(maxAng))*length, xtexBase+1.0f/32, 1);
	}

	glNewList(listNum,GL_COMPILE);
	va->DrawArrayT(GL_TRIANGLE_STRIP);
	glEndList();
}

void CGrassDrawer::CreateGrassBladeTex(unsigned char* buf)
{
	float3 col(0.59f+fRand(0.11f),0.81f+fRand(0.08f),0.57f+fRand(0.11f));
	for(int y=0;y<64;++y){
		for(int x=0;x<16;++x){
			buf[(y*256+x)*4+0]=(unsigned char) ((col.x+y*0.0005f+fRand(0.05f))*255);
			buf[(y*256+x)*4+1]=(unsigned char) ((col.y+y*0.0006f+fRand(0.05f))*255);
			buf[(y*256+x)*4+2]=(unsigned char) ((col.z+y*0.0004f+fRand(0.05f))*255);
			buf[(y*256+x)*4+3]=1;
		}
	}
	for(int y=0;y<64;++y){
		for(int x=7;x<9;++x){
			buf[(y*256+x)*4+0]=(unsigned char) ((col.x+y*0.0005f+fRand(0.05f)-0.03f)*255);
			buf[(y*256+x)*4+1]=(unsigned char) ((col.y+y*0.0006f+fRand(0.05f)-0.03f)*255);
			buf[(y*256+x)*4+2]=(unsigned char) ((col.z+y*0.0004f+fRand(0.05f)-0.03f)*255);
		}
	}
	for(int y=4;y<64;y+=4+(int)fRand(3)){
		for(int a=0;a<7 && a+y<64;++a){
			buf[((a+y)*256+a+9)*4+0]=(unsigned char) ((col.x+y*0.0005f+fRand(0.05f)-0.03f)*255);
			buf[((a+y)*256+a+9)*4+1]=(unsigned char) ((col.y+y*0.0006f+fRand(0.05f)-0.03f)*255);
			buf[((a+y)*256+a+9)*4+2]=(unsigned char) ((col.z+y*0.0004f+fRand(0.05f)-0.03f)*255);

			buf[((a+y)*256+6-a)*4+0]=(unsigned char) ((col.x+y*0.0005f+fRand(0.05f)-0.03f)*255);
			buf[((a+y)*256+6-a)*4+1]=(unsigned char) ((col.y+y*0.0006f+fRand(0.05f)-0.03f)*255);
			buf[((a+y)*256+6-a)*4+2]=(unsigned char) ((col.z+y*0.0004f+fRand(0.05f)-0.03f)*255);
		}
	}

/*	for(int y=0;y<64;++y){
		for(int x=0;x<16;++x){
			buf[(y*256+x)*4+0]=255;
			buf[(y*256+x)*4+1]=255;
			buf[(y*256+x)*4+2]=255;
			buf[(y*256+x)*4+3]=1;
		}
	}*/
}

void CGrassDrawer::CreateFarTex(void)
{
	int sizeMod=2;
	unsigned char* buf=SAFE_NEW unsigned char[64*sizeMod*1024*sizeMod*4];
	unsigned char* buf2=SAFE_NEW unsigned char[256*sizeMod*256*sizeMod*4];
	memset(buf,0,64*sizeMod*1024*sizeMod*4);
	memset(buf2,0,256*sizeMod*256*sizeMod*4);

	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glBindTexture(GL_TEXTURE_2D, grassBladeTex);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glDisable(GL_BLEND);
	glColor4f(1,1,1,1);			
	glViewport(0,0,256*sizeMod,256*sizeMod);

	for(int a=0;a<16;++a){
		glClearColor(0.0f,0.0f,0.0f,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef((a-1)*90/15.0f,1,0,0);
		glTranslatef(0,-0.5f,0);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-partTurfSize, partTurfSize, -partTurfSize, partTurfSize, -turfSize, turfSize);

		glCallList(grassDL);
		
		glReadPixels(0,0,256*sizeMod,256*sizeMod,GL_RGBA,GL_UNSIGNED_BYTE,buf2);

//		CBitmap bm(buf2,512,512);
//		bm.Save("ft.bmp");

		int dx=a*64*sizeMod;
		for(int y=0;y<64*sizeMod;++y){
			for(int x=0;x<64*sizeMod;++x){
				float r=0,g=0,b=0,a=0;
				for(int y2=0;y2<4;++y2){
					for(int x2=0;x2<4;++x2){
						if(buf2[((y*4+y2)*256*sizeMod+x*4+x2)*4]==0 && buf2[((y*4+y2)*256*sizeMod+x*4+x2)*4+1]==0 && buf2[((y*4+y2)*256*sizeMod+x*4+x2)*4+2]==0){
						} else {
							float s=1;
							if(y!=0 && y!=64*sizeMod-1 && (buf2[((y*4+y2)*256*sizeMod+x*4+x2+1)*4+1]==0 || buf2[((y*4+y2)*256*sizeMod+x*4+x2-1)*4+1]==0 || buf2[((y*4+y2+1)*256*sizeMod+x*4+x2)*4+1]==0 || buf2[((y*4+y2-1)*256*sizeMod+x*4+x2)*4+1]==0)){
								s=0.5f;
							}
							r+=s*buf2[((y*4+y2)*256*sizeMod+x*4+x2)*4+0];
							g+=s*buf2[((y*4+y2)*256*sizeMod+x*4+x2)*4+1];
							b+=s*buf2[((y*4+y2)*256*sizeMod+x*4+x2)*4+2];
							a+=s;
						}
					}
				}
				if(a==0){
					buf[((y)*1024*sizeMod+x+dx)*4+0]=190;
					buf[((y)*1024*sizeMod+x+dx)*4+1]=230;
					buf[((y)*1024*sizeMod+x+dx)*4+2]=190;
					buf[((y)*1024*sizeMod+x+dx)*4+3]=0;
				} else {
					buf[((y)*1024*sizeMod+x+dx)*4+0]=(unsigned char) (r/a);
					buf[((y)*1024*sizeMod+x+dx)*4+1]=(unsigned char) (g/a);
					buf[((y)*1024*sizeMod+x+dx)*4+2]=(unsigned char) (b/a);
					buf[((y)*1024*sizeMod+x+dx)*4+3]=(unsigned char) (std::min((float)255,a*16));
				}
			}
		}
	}
//	CBitmap bm(buf,1024*sizeMod,64*sizeMod);
//	bm.Save("fartex.bmp");

	glViewport(gu->viewPosX,0,gu->viewSizeX,gu->viewSizeY);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glGenTextures(1, &farTex);
	glBindTexture(GL_TEXTURE_2D, farTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glBuildMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,1024*sizeMod, 64*sizeMod, GL_RGBA, GL_UNSIGNED_BYTE, buf);

	delete[] buf;
	delete[] buf2;
}

void CGrassDrawer::AddGrass(float3 pos)
{
	if(grassOff)
		return;
	grassMap[(int(pos.z)/SQUARE_SIZE/grassSquareSize)*gs->mapx/grassSquareSize+int(pos.x)/SQUARE_SIZE/grassSquareSize]=1;
}

void CGrassDrawer::RemoveGrass(int x, int z)
{
	if(grassOff)
		return;
	grassMap[(z/grassSquareSize)*gs->mapx/grassSquareSize+x/grassSquareSize]=0;
	ResetPos(float3(x*SQUARE_SIZE,0,z*SQUARE_SIZE));
}

void CGrassDrawer::SetTexGen(float scalex,float scaley, float offsetx, float offsety)
{
	GLfloat plan[]={scalex,0,0,offsetx};
	glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGenfv(GL_S,GL_EYE_PLANE,plan);
	glEnable(GL_TEXTURE_GEN_S);
	GLfloat plan2[]={0,0,scaley,offsety};
	glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGenfv(GL_T,GL_EYE_PLANE,plan2);
	glEnable(GL_TEXTURE_GEN_T);
}
