#include "StdAfx.h"
#include "BaseGroundDrawer.h"
#include "Platform/ConfigHandler.h"
#include "Rendering/GL/myGL.h"
#include "Game/Camera.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Game/SelectedUnits.h"
#include "Sim/Units/UnitDef.h"
#include "Rendering/GroundDecalHandler.h"
#include "Game/UI/GuiHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Units/UnitHandler.h"
#include "mmgr.h"

CBaseGroundDrawer::CBaseGroundDrawer(void)
{
	updateFov=true;

	striptype=GL_TRIANGLE_STRIP;

	infoTexAlpha=0.25f;
	infoTex=0;

	drawMode=drawNormal;
	drawLineOfSight=false;
	drawRadarAndJammer=true;

	extraTex=0;
	extraTexPal=0;
	extractDepthMap=0;

	infoTexMem=SAFE_NEW unsigned char[gs->pwr2mapx*gs->pwr2mapy*4];
	for(int a=0;a<gs->pwr2mapx*gs->pwr2mapy*4;++a)
		infoTexMem[a]=255;

	highResInfoTexWanted=false;

	highResLosTex = !!configHandler.GetInt("HighResLos", 0);
// 	smoothLosTex = !!configHandler.GetInt("SmoothLos", 1);
}

CBaseGroundDrawer::~CBaseGroundDrawer(void)
{
	delete[] infoTexMem;
	if(infoTex!=0)
		glDeleteTextures(1,&infoTex);
}

void CBaseGroundDrawer::DrawShadowPass(void)
{}

void CBaseGroundDrawer::SetDrawMode (DrawMode dm)
{
	drawMode = dm;
}

//todo: this part of extra textures is a mess really ...
void CBaseGroundDrawer::DisableExtraTexture()
{
	if(drawLineOfSight){
		SetDrawMode(drawLos);
	} else {
		SetDrawMode(drawNormal);
	}
	extraTex=0;
	highResInfoTexWanted=false;
	updateTextureState=0;
	while(!UpdateExtraTexture());
}

void CBaseGroundDrawer::SetHeightTexture()
{
	if (drawMode == drawHeight)
		DisableExtraTexture();
	else {
		SetDrawMode (drawHeight);
		highResInfoTexWanted=true;
		extraTex=0;
		updateTextureState=0;
		while(!UpdateExtraTexture());
	}
}

void CBaseGroundDrawer::SetMetalTexture(unsigned char* tex,float* extractMap,unsigned char* pal,bool highRes)
{
	if (drawMode == drawMetal)
		DisableExtraTexture();
	else {
		SetDrawMode (drawMetal);

		highResInfoTexWanted=false;
		extraTex=tex;
		extraTexPal=pal;
		extractDepthMap=extractMap;
		updateTextureState=0;
		while(!UpdateExtraTexture());
	}
}

void CBaseGroundDrawer::SetPathMapTexture()
{
	if (drawMode==drawPath)
		DisableExtraTexture();
	else {
		SetDrawMode(drawPath);
		extraTex=0;
		highResInfoTexWanted=false;
		updateTextureState=0;
		while(!UpdateExtraTexture());
	}
}

void CBaseGroundDrawer::ToggleLosTexture()
{
	if (drawMode==drawLos) {
		drawLineOfSight=false;
		DisableExtraTexture();
	} else {
		drawLineOfSight=true;
		SetDrawMode(drawLos);
		extraTex=0;
		highResInfoTexWanted=highResLosTex;
		updateTextureState=0;
		while(!UpdateExtraTexture());
	}
}

void CBaseGroundDrawer::ToggleRadarAndJammer()
{
	drawRadarAndJammer=!drawRadarAndJammer;
	if (drawMode==drawLos){
		updateTextureState=0;
		while(!UpdateExtraTexture());
	}
}

static inline int InterpolateLos(unsigned short* p, int xsize, int ysize, int mip, int factor, int x, int y)
{
	int x1 = x >> mip;
	int y1 = y >> mip;
	int s1 = p[y1*xsize+x1] != 0; // top left
	if (mip > 0) {
		int x2 = x1 + 1;
		int y2 = y1 + 1;
		if (x2 >= xsize)
			x2 = xsize - 1;
		if (y2 >= ysize)
			y2 = ysize - 1;
		int s2 = p[y1*xsize+x2] != 0; // top right
		int s3 = p[y2*xsize+x1] != 0; // bottom left
		int s4 = p[y2*xsize+x2] != 0; // bottom right
		int size = 1 << mip;
		int fracx = x % size;
		int fracy = y % size;
		int c1 = factor * (s2 - s1) * fracx / size + factor * s1;
		int c2 = factor * (s4 - s3) * fracx / size + factor * s3;
		return (c2 - c1) * fracy / size + c1;
	}
	return factor * s1;
}

// Gradually calculate the extra texture based on updateTextureState:
//   updateTextureState < 50:   Calculate the texture color values and copy them in a buffer
//   updateTextureState >= 50:  Copy the buffer into a texture
//   updateTextureState = 57:   Reset to 0 and restart updating
bool CBaseGroundDrawer::UpdateExtraTexture()
{
	if(drawMode == drawNormal)
		return true;

	unsigned short* myLos=loshandler->losMap[gu->myAllyTeam];
	unsigned short* myAirLos=loshandler->airLosMap[gu->myAllyTeam];
	unsigned short* myRadar=radarhandler->radarMaps[gu->myAllyTeam];
	unsigned short* myJammer=radarhandler->jammerMaps[gu->myAllyTeam];
	if(updateTextureState<50){
		int starty;
		int endy;
		if(highResInfoTexWanted){
			starty=updateTextureState*gs->mapy/50;
			endy=(updateTextureState+1)*gs->mapy/50;
		} else {
			starty=updateTextureState*gs->hmapy/50;
			endy=(updateTextureState+1)*gs->hmapy/50;
		}

		switch(drawMode) {
		case drawPath:{
			if(selectedUnits.selectedUnits.empty() || !(*selectedUnits.selectedUnits.begin())->unitDef->movedata)
				return true;

			MoveData *md=(*selectedUnits.selectedUnits.begin())->unitDef->movedata;

			for(int y=starty;y<endy;++y){
				for(int x=0;x<gs->hmapx;++x){
					int a=y*gs->pwr2mapx/2+x;

					float m;
					//todo: fix for new gui
					if(guihandler->inCommand>0 && guihandler->inCommand<guihandler->commands.size() && guihandler->commands[guihandler->inCommand].type==CMDTYPE_ICON_BUILDING){
						if(!loshandler->InLos(float3(x*16+8,0,y*16+8),gu->myAllyTeam)){
							m=0.25f;
						}else{
							UnitDef *unitdef = unitDefHandler->GetUnitByID(-guihandler->commands[guihandler->inCommand].id);

							CFeature* f;
							if(uh->TestUnitBuildSquare(BuildInfo(unitdef,float3(x*16+8,0,y*16+8),guihandler->buildFacing),f,gu->myAllyTeam))
								m=1;
							else
								m=0;
							if(f && m)
								m=0.5f;
						}

					} else {
						m=md->moveMath->SpeedMod(*md, x*2,y*2);
						if(gs->cheatEnabled && md->moveMath->IsBlocked2(*md, x*2+1, y*2+1) & (CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN))
							m=0;
						m=min(1.0f,(float)sqrt(m));
					}
					infoTexMem[a*4+0]=255-int(m*255.0f);
					infoTexMem[a*4+1]=int(m*255.0f);
					infoTexMem[a*4+2]=0;
				}
			}
			break;}
		case drawMetal:
			for(int y=starty;y<endy;++y){
				for(int x=0;x<gs->hmapx;++x){
					int a=y*gs->pwr2mapx/2+x;
					if(myAirLos[((y*2)>>loshandler->airMipLevel)*loshandler->airSizeX+((x*2)>>loshandler->airMipLevel)]) {
						float extractDepth = extractDepthMap[y*gs->hmapx+x];
						infoTexMem[a*4]=(unsigned char)min(255.0f,(float)sqrt(sqrt(extractDepth))*900);
					} else
						infoTexMem[a*4]=0;
					infoTexMem[a*4+1]=(extraTexPal[extraTex[y*gs->hmapx+x]*3+1]);
					infoTexMem[a*4+2]=(extraTexPal[extraTex[y*gs->hmapx+x]*3+2]);
				}
			}
			break;
		case drawHeight:
			extraTexPal=readmap->heightLinePal;
			for(int y=starty;y<endy;++y){
				for(int x=0;x<gs->mapx;++x){
					int a=y*gs->pwr2mapx+x;
					float height=readmap->centerheightmap[y*gs->mapx+x];
					unsigned char value=(unsigned char)(height*8);
					infoTexMem[a*4]=64+(extraTexPal[value*3]>>1);
					infoTexMem[a*4+1]=64+(extraTexPal[value*3+1]>>1);
					infoTexMem[a*4+2]=64+(extraTexPal[value*3+2]>>1);
				}
			}
			break;
		case drawLos: {
			int lowRes = highResInfoTexWanted ? 0 : -1;
			int endx = highResInfoTexWanted ? gs->mapx : gs->hmapx;
			int pwr2mapx = gs->pwr2mapx >> (-lowRes);
			for(int y=starty;y<endy;++y){
				for(int x=0;x<endx;++x){
					int a=y*pwr2mapx+x;
					int inRadar=0;
					int inJam=0;
					if (drawRadarAndJammer){
						inRadar = InterpolateLos(myRadar, radarhandler->xsize, radarhandler->ysize, 3+lowRes, 50, x, y);
						inJam =  InterpolateLos(myJammer, radarhandler->xsize, radarhandler->ysize, 3+lowRes, 50, x, y);
					}
					int inLos = InterpolateLos(myLos,    loshandler->losSizeX, loshandler->losSizeY, loshandler->losMipLevel+lowRes, 32, x, y);
					int inAir = InterpolateLos(myAirLos, loshandler->airSizeX, loshandler->airSizeY, loshandler->airMipLevel+lowRes, 32, x, y);
					infoTexMem[a*4] = 64+inLos+inAir+inJam;
					infoTexMem[a*4+1] = 64+inLos+inAir+inRadar;
					infoTexMem[a*4+2] = 64+inLos+inAir;
				}
			}
			break;}
		}
	}

	if(updateTextureState==50){
		if(infoTex!=0 && highResInfoTexWanted!=highResInfoTex){
			glDeleteTextures(1,&infoTex);
			infoTex=0;
		}
		if(infoTex==0){
			glGenTextures(1,&infoTex);
			glBindTexture(GL_TEXTURE_2D, infoTex);

			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			if(highResInfoTexWanted)
				glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8, gs->pwr2mapx, gs->pwr2mapy,0,GL_RGBA, GL_UNSIGNED_BYTE, infoTexMem);
			else
				glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8, gs->pwr2mapx>>1, gs->pwr2mapy>>1,0,GL_RGBA, GL_UNSIGNED_BYTE, infoTexMem);
			highResInfoTex=highResInfoTexWanted;
			updateTextureState=0;
			return true;
		}

	}
	if(updateTextureState>=50){
		glBindTexture(GL_TEXTURE_2D, infoTex);
		if(highResInfoTex)
			glTexSubImage2D(GL_TEXTURE_2D,0, 0,(updateTextureState-50)*(gs->pwr2mapy/8),gs->pwr2mapx, (gs->pwr2mapy/8),GL_RGBA, GL_UNSIGNED_BYTE, &infoTexMem[(updateTextureState-50)*(gs->pwr2mapy/8)*gs->pwr2mapx*4]);
		else
			glTexSubImage2D(GL_TEXTURE_2D,0, 0,(updateTextureState-50)*(gs->pwr2mapy/16),gs->pwr2mapx>>1, (gs->pwr2mapy/16),GL_RGBA, GL_UNSIGNED_BYTE, &infoTexMem[(updateTextureState-50)*(gs->pwr2mapy/16)*(gs->pwr2mapx>>1)*4]);
		if(updateTextureState==57){
			updateTextureState=0;
			return true;
		}
	}
	updateTextureState++;
	return false;
}



void CBaseGroundDrawer::SetTexGen(float scalex,float scaley, float offsetx, float offsety)
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
