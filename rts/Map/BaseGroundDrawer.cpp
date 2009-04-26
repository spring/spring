#include "StdAfx.h"
#include "mmgr.h"

#include "BaseGroundDrawer.h"

#include "Game/Camera.h"
#include "Game/SelectedUnits.h"
#include "Game/UI/GuiHandler.h"
#include "Ground.h"
#include "HeightLinePalette.h"
#include "ReadMap.h"
#include "MapInfo.h"
#include "Rendering/GroundDecalHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "ConfigHandler.h"
#include "FastMath.h"

CBaseGroundDrawer::CBaseGroundDrawer(void)
{
	updateFov = true;

	LODScaleReflection = configHandler->Get("GroundLODScaleReflection", 1.0f);
	LODScaleRefraction = configHandler->Get("GroundLODScaleRefraction", 1.0f);
	LODScaleUnitReflection = configHandler->Get("GroundLODScaleUnitReflection", 1.0f);

	infoTexAlpha = 0.25f;
	infoTex = 0;

	drawMode = drawNormal;
	drawLineOfSight = false;
	drawRadarAndJammer = true;
	wireframe = false;

	extraTex = 0;
	extraTexPal = 0;
	extractDepthMap = 0;

	infoTexMem = new unsigned char[gs->pwr2mapx*gs->pwr2mapy*4];
	for (int a = 0; a < (gs->pwr2mapx * gs->pwr2mapy * 4); ++a) {
		infoTexMem[a] = 255;
	}

	highResInfoTexWanted = false;

	highResLosTex = !!configHandler->Get("HighResLos", 0);
// 	smoothLosTex = !!configHandler->Get("SmoothLos", 1);

	jamColor[0] = (int)(losColorScale * 0.25f);
	jamColor[1] = (int)(losColorScale * 0.0f);
	jamColor[2] = (int)(losColorScale * 0.0f);

	losColor[0] = (int)(losColorScale * 0.15f);
	losColor[1] = (int)(losColorScale * 0.05f);
	losColor[2] = (int)(losColorScale * 0.40f);

	radarColor[0] = (int)(losColorScale *  0.05f);
	radarColor[1] = (int)(losColorScale *  0.15f);
	radarColor[2] = (int)(losColorScale * -0.20f);

	alwaysColor[0] = (int)(losColorScale * 0.25f);
	alwaysColor[1] = (int)(losColorScale * 0.25f);
	alwaysColor[2] = (int)(losColorScale * 0.25f);

	heightLinePal = new CHeightLinePalette();
}


CBaseGroundDrawer::~CBaseGroundDrawer(void)
{
	delete[] infoTexMem;
	if (infoTex!=0) {
		glDeleteTextures(1, &infoTex);
	}

	delete heightLinePal;
}


void CBaseGroundDrawer::DrawShadowPass(void)
{}


void CBaseGroundDrawer::DrawTrees(bool drawReflection) const
{
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.005f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	if (treeDrawer->drawTrees) {
		if (DrawExtraTex()) {
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
			glBindTexture(GL_TEXTURE_2D, infoTex);
			SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}

		treeDrawer->Draw(drawReflection);

		if (DrawExtraTex()) {
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
	}

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
}


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


static inline int InterpolateLos(const unsigned short* p, int xsize, int ysize,
                                 int mip, int factor, int x, int y)
{
	const int x1 = (x >> mip);
	const int y1 = (y >> mip);
	const int s1 = (p[(y1 * xsize) + x1] != 0); // top left
	if (mip > 0) {
		int x2 = (x1 + 1);
		int y2 = (y1 + 1);
		if (x2 >= xsize) { x2 = xsize - 1; }
		if (y2 >= ysize) { y2 = ysize - 1; }
		const int s2 = (p[(y1 * xsize) + x2] != 0); // top right
		const int s3 = (p[(y2 * xsize) + x1] != 0); // bottom left
		const int s4 = (p[(y2 * xsize) + x2] != 0); // bottom right
		const int size  = (1 << mip);
		const int fracx = (x % size);
		const int fracy = (y % size);
		const int c1 = (factor * (s2 - s1) * fracx) / size + (factor * s1);
		const int c2 = (factor * (s4 - s3) * fracx) / size + (factor * s3);
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
	if (mapInfo->map.voidWater && readmap->currMaxHeight<0) {
		return true;
	}

	if (drawMode == drawNormal) {
		return true;
	}

	const unsigned short* myLos         = &loshandler->losMap[gu->myAllyTeam].front();
	const unsigned short* myAirLos      = &loshandler->airLosMap[gu->myAllyTeam].front();
	const unsigned short* myRadar       = &radarhandler->radarMaps[gu->myAllyTeam].front();
	const unsigned short* myJammer      = &radarhandler->jammerMaps[gu->myAllyTeam].front();
#ifdef SONAR_JAMMER_MAPS
	const unsigned short* mySonar       = &radarhandler->sonarMaps[gu->myAllyTeam].front();
	const unsigned short* mySonarJammer = &radarhandler->sonarJammerMaps[gu->myAllyTeam].front();
#endif

	if (updateTextureState < 50) {
		int starty;
		int endy;
		if (highResInfoTexWanted) {
			starty = updateTextureState * gs->mapy / 50;
			endy = (updateTextureState + 1) * gs->mapy / 50;
		} else {
			starty = updateTextureState * gs->hmapy / 50;
			endy = (updateTextureState + 1) * gs->hmapy / 50;
		}

		switch(drawMode) {
			case drawPath: {
				if (guihandler->inCommand > 0 && static_cast<size_t>(guihandler->inCommand) < guihandler->commands.size() &&
						guihandler->commands[guihandler->inCommand].type == CMDTYPE_ICON_BUILDING) {
					// use the current build order
					for (int y = starty; y < endy; ++y) {
						for (int x = 0; x < gs->hmapx; ++x) {
							float m;
							if (!loshandler->InLos(float3(x*16+8, 0, y*16+8), gu->myAllyTeam)) {
								m = 0.25f;
							} else {
								const UnitDef* unitdef = unitDefHandler->GetUnitByID(-guihandler->commands[guihandler->inCommand].id);
								CFeature* f;

								GML_RECMUTEX_LOCK(quad); // UpdateExtraTexture - testunitbuildsquare accesses features in the quadfield

								if(uh->TestUnitBuildSquare(BuildInfo(unitdef, float3(x*16+8, 0, y*16+8), guihandler->buildFacing), f, gu->myAllyTeam)) {
									if (f) {
										m = 0.5f;
									} else {
										m = 1.0f;
									}
								} else {
									m = 0.0f;
								}
							}
							const int a=y*gs->pwr2mapx/2+x;
							infoTexMem[a*4+0]=255-int(m*255.0f);
							infoTexMem[a*4+1]=int(m*255.0f);
							infoTexMem[a*4+2]=0;
						}
					}
				}
				else {
					// use the first selected unit

					GML_RECMUTEX_LOCK(sel); // UpdateExtraTexture

					if (selectedUnits.selectedUnits.empty()) {
						return true;
					}
					const MoveData* md = (*selectedUnits.selectedUnits.begin())->unitDef->movedata;
					if (md == NULL) {
						return true;
					}
					for (int y = starty; y < endy; ++y) {
						for (int x = 0; x < gs->hmapx; ++x) {
							float m = md->moveMath->SpeedMod(*md, x*2, y*2);
							if (gs->cheatEnabled && md->moveMath->IsBlocked2(*md, x*2+1, y*2+1) & (CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN)) {
								m = 0.0f;
							}
							m = std::min(1.0f, (float)fastmath::sqrt(m));
							const int a=y*gs->pwr2mapx/2+x;
							infoTexMem[a*4+0]=255-int(m*255.0f);
							infoTexMem[a*4+1]=int(m*255.0f);
							infoTexMem[a*4+2]=0;
						}
					}
				}
				break;
			}
			case drawMetal: {
				for (int y = starty; y < endy; ++y) {
					for (int x = 0; x < gs->hmapx; ++x) {
						int a = (y * gs->pwr2mapx) / 2 + x;
						const int alx = ((x * 2) >> loshandler->airMipLevel);
						const int aly = ((y * 2) >> loshandler->airMipLevel);
						if (myAirLos[alx + (aly * loshandler->airSizeX)]) {
							float extractDepth = extractDepthMap[(y * gs->hmapx) + x];
							// a single pow(x, 0.25) call would be faster?
							infoTexMem[a*4]=(unsigned char)std::min(255.0f,(float)fastmath::sqrt(fastmath::sqrt(extractDepth))*900);
						} else {
							infoTexMem[a*4]=0;
						}
						infoTexMem[a*4+1]=(extraTexPal[extraTex[y*gs->hmapx+x]*3+1]);
						infoTexMem[a*4+2]=(extraTexPal[extraTex[y*gs->hmapx+x]*3+2]);
					}
				}
				break;
			}
			case drawHeight: {
				extraTexPal = heightLinePal->GetData();
				for (int y = starty; y < endy; ++y) {
					for (int x = 0; x  < gs->mapx; ++x){
						const float height = readmap->centerheightmap[(y * gs->mapx) + x];
						const unsigned char value = (unsigned char)(height * 8);
						const int i = 4 * ((y * gs->pwr2mapx) + x);
						infoTexMem[i]     = 64 + (extraTexPal[value * 3]     >> 1);
						infoTexMem[i + 1] = 64 + (extraTexPal[value * 3 + 1] >> 1);
						infoTexMem[i + 2] = 64 + (extraTexPal[value * 3 + 2] >> 1);
					}
				}
				break;
			}
			case drawLos: {
				int lowRes = highResInfoTexWanted ? 0 : -1;
				int endx = highResInfoTexWanted ? gs->mapx : gs->hmapx;
				int pwr2mapx = gs->pwr2mapx >> (-lowRes);
				const int losSizeX = loshandler->losSizeX;
				const int losSizeY = loshandler->losSizeY;
				const int airSizeX = loshandler->airSizeX;
				const int airSizeY = loshandler->airSizeY;
				const int losMipLevel = loshandler->losMipLevel;
				const int airMipLevel = loshandler->airMipLevel;
				if (drawRadarAndJammer) {
					const int rxsize = radarhandler->xsize;
					const int rzsize = radarhandler->zsize;
					//const int posScale = highResInfoTexWanted ? SQUARE_SIZE : (SQUARE_SIZE * 2);
					for (int y = starty; y < endy; ++y) {
						//const float zPos = y * posScale;
						for (int x = 0; x < endx; ++x) {
							//const float xPos = x * posScale;
							int a = (y * pwr2mapx) + x;
							int totalLos;
							if (gs->globalLOS) {
								totalLos = 255;
							} else {
								const int inLos = InterpolateLos(myLos,    losSizeX, losSizeY, losMipLevel + lowRes, 255, x, y);
								const int inAir = InterpolateLos(myAirLos, airSizeX, airSizeY, airMipLevel + lowRes, 255, x, y);
								totalLos = (inLos + inAir) / 2;
							}

#ifdef SONAR_JAMMER_MAPS
							const bool useRadar = (ground->GetHeight2(xPos, zPos) >= 0.0f);
							const unsigned short* radarMap  = useRadar ? myRadar  : mySonar;
							const unsigned short* jammerMap = useRadar ? myJammer : mySonarJammer;
#else
							const unsigned short* radarMap  = myRadar;
							const unsigned short* jammerMap = myJammer;
#endif // SONAR_JAMMER_MAPS
							const int inRadar = InterpolateLos(radarMap,  rxsize, rzsize, 3 + lowRes, 255, x, y);
							const int inJam   = InterpolateLos(jammerMap, rxsize, rzsize, 3 + lowRes, 255, x, y);

							const int index = (a * 4);
							for (int c = 0; c < 3; c++) {
								int val = alwaysColor[c] * 255;
								val += (jamColor[c]   * inJam);
								val += (losColor[c]   * totalLos);
								val += (radarColor[c] * inRadar);
								infoTexMem[index + c] = (val / losColorScale);
							}
						}
					}
				}
				else {
					for (int y = starty; y < endy; ++y) {
						for (int x = 0; x < endx; ++x) {
							int a = (y * pwr2mapx) + x;
							const int inLos = InterpolateLos(myLos,    losSizeX, losSizeY, losMipLevel + lowRes, 64, x, y);
							const int inAir = InterpolateLos(myAirLos, airSizeX, airSizeY, airMipLevel + lowRes, 64, x, y);
							const int totalLos = (inLos + inAir) / 2;
							const int index = (a * 4);
							const int value = (64 + totalLos);
							infoTexMem[index]     = value;
							infoTexMem[index + 1] = value;
							infoTexMem[index + 2] = value;
						}
					}
				}
				break;
			}
			case drawNormal:
				break;
		} // switch (drawMode)
	} // if (updateTextureState < 50)

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


void CBaseGroundDrawer::SetTexGen(float scalex,float scaley, float offsetx, float offsety) const
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
