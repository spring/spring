/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Path/PathManager.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "ConfigHandler.h"
#include "FastMath.h"
#include "myMath.h"

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

	extraTexPBO.Bind();
	extraTexPBO.Resize(gs->pwr2mapx * gs->pwr2mapy * 4);
	extraTexPBO.Unbind(false);

	highResInfoTexWanted = false;

	highResLosTex = !!configHandler->Get("HighResLos", 0);
	extraTextureUpdateRate = std::max(4, configHandler->Get("ExtraTextureUpdateRate", 45) - 1);

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
	if (drawMode == drawPath) {
		DisableExtraTexture();
	} else {
		SetDrawMode(drawPath);
		extraTex = 0;
		highResInfoTexWanted = false;
		updateTextureState = 0;
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


void CBaseGroundDrawer::ToggleHeatMapTexture()
{
	if (drawMode == drawHeat) {
		DisableExtraTexture();
	} else {
		SetDrawMode(drawHeat);
		extraTex = 0;
		highResInfoTexWanted = false;
		updateTextureState = 0;
		while(!UpdateExtraTexture());
	}
}


static inline int InterpolateLos(const unsigned short* p, int xsize, int ysize,
                                 int mip, int factor, int x, int y)
{
	const int x1 = x >> mip;
	const int y1 = y >> mip;
	const int s1 = (p[(y1 * xsize) + x1] != 0); // top left
	if (mip > 0) {
		int x2 = x1 + 1;
		int y2 = y1 + 1;
		if (x2 >= xsize) { x2 = xsize - 1; }
		if (y2 >= ysize) { y2 = ysize - 1; }
		const int s2 = (p[(y1 * xsize) + x2] != 0); // top right
		const int s3 = (p[(y2 * xsize) + x1] != 0); // bottom left
		const int s4 = (p[(y2 * xsize) + x2] != 0); // bottom right
		const int size  = (1 << mip);
		const float fracx = float(x % size) / size;
		const float fracy = float(y % size) / size;
		const float c1 = (s2 - s1) * fracx + s1;
		const float c2 = (s4 - s3) * fracx + s3;
		return factor * ((c2 - c1) * fracy + c1);
	}
	return factor * s1;
}


// Gradually calculate the extra texture based on updateTextureState:
//   updateTextureState < extraTextureUpdateRate:   Calculate the texture color values and copy them in a buffer
//   updateTextureState = extraTextureUpdateRate:   Copy the buffer into a texture
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

	if (updateTextureState < extraTextureUpdateRate) {
		const int pwr2mapx_half = gs->pwr2mapx / 2;

		int starty;
		int endy;
		int offset;
		GLbyte* infoTexMem;

		extraTexPBO.Bind();
		if (highResInfoTexWanted) {
			starty = updateTextureState * gs->mapy / extraTextureUpdateRate;
			endy = (updateTextureState + 1) * gs->mapy / extraTextureUpdateRate;

			offset = starty * gs->pwr2mapx * 4;
			infoTexMem = (GLbyte*)extraTexPBO.MapBuffer(offset, (endy - starty) * gs->pwr2mapx * 4);
		} else {
			starty = updateTextureState * gs->hmapy / extraTextureUpdateRate;
			endy = (updateTextureState + 1) * gs->hmapy / extraTextureUpdateRate;

			offset = starty * pwr2mapx_half * 4;
			infoTexMem = (GLbyte*)extraTexPBO.MapBuffer(offset, (endy - starty) * pwr2mapx_half * 4);
		}

#define COLOR_R 2
#define COLOR_G 1
#define COLOR_B 0
#define COLOR_A 3

		switch(drawMode) {
			case drawPath: {
				if (guihandler->inCommand > 0 &&
				    static_cast<size_t>(guihandler->inCommand) < guihandler->commands.size() &&
				    guihandler->commands[guihandler->inCommand].type == CMDTYPE_ICON_BUILDING)
				{
					// use the current build order
					for (int y = starty; y < endy; ++y) {
						const int y_pwr2mapx_half = y*pwr2mapx_half;
						const int y_16            = y*16;
						for (int x = 0; x < gs->hmapx; ++x) {
							float m;
							if (!loshandler->InLos(float3(x*16+8, 0, y_16+8), gu->myAllyTeam)) {
								m = 0.25f;
							} else {
								const UnitDef* unitdef = unitDefHandler->GetUnitDefByID(-guihandler->commands[guihandler->inCommand].id);
								CFeature* f;

								GML_RECMUTEX_LOCK(quad); // UpdateExtraTexture - testunitbuildsquare accesses features in the quadfield

								if (uh->TestUnitBuildSquare(BuildInfo(unitdef, float3(x*16+8, 0, y_16+8), guihandler->buildFacing), f, gu->myAllyTeam)) {
									if (f) {
										m = 0.5f;
									} else {
										m = 1.0f;
									}
								} else {
									m = 0.0f;
								}
							}
							m = (int) (m*255.0f);
							const int a = (y_pwr2mapx_half + x) * 4 - offset;
							infoTexMem[a + COLOR_R] = 255 - m;
							infoTexMem[a + COLOR_G] = m;
							infoTexMem[a + COLOR_B] = 0;
							infoTexMem[a + COLOR_A] = 255;
						}
					}
				} else {
					const MoveData* md = NULL;
					{
						GML_RECMUTEX_LOCK(sel); // UpdateExtraTexture
						if (!selectedUnits.selectedUnits.empty()) {
							md = (*selectedUnits.selectedUnits.begin())->unitDef->movedata;
						}
					}

					if (md != NULL) {
						// use the first selected unit, if it has the ability to move
						const bool showBlockedMap = (gs->cheatEnabled || gu->spectating);
						for (int y = starty; y < endy; ++y) {
							const int y_pwr2mapx_half = y*pwr2mapx_half;
							const int y_2             = y*2;
							for (int x = 0; x < gs->hmapx; ++x) {
								float m = md->moveMath->SpeedMod(*md, x*2, y_2);
								if (showBlockedMap && (md->moveMath->IsBlocked2(*md, x*2+1, y_2+1) & (CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN))) {
									m = 0.0f;
								}
								m = std::min(1.0f, fastmath::apxsqrt(m));
								m = (int) (m*255.0f);
								const int a = (y_pwr2mapx_half + x) * 4 - offset;
								infoTexMem[a + COLOR_R] = 255 - m;
								infoTexMem[a + COLOR_G] = m;
								infoTexMem[a + COLOR_B] = 0;
								infoTexMem[a + COLOR_A] = 255;
							}
						}
					} else {
						// we have nothing to show
						// -> draw a dark red overlay
						for (int y = starty; y < endy; ++y) {
							const int y_pwr2mapx_half = y*pwr2mapx_half;
							for (int x = 0; x < gs->hmapx; ++x) {
								const int a = (y_pwr2mapx_half + x) * 4 - offset;
								infoTexMem[a + COLOR_R] = 100;
								infoTexMem[a + COLOR_G] = 0;
								infoTexMem[a + COLOR_B] = 0;
								infoTexMem[a + COLOR_A] = 255;
							}
						}
					}
				}
				break;
			}
			case drawMetal: {
				for (int y = starty; y < endy; ++y) {
					const int y_pwr2mapx_half = y*pwr2mapx_half;
					const int y_2 = y*2;
					const int y_hmapx = y * gs->hmapx;
					for (int x = 0; x < gs->hmapx; ++x) {
						const int a   = (y_pwr2mapx_half + x) * 4 - offset;
						const int alx = ((x*2) >> loshandler->airMipLevel);
						const int aly = ((y_2) >> loshandler->airMipLevel);
						if (myAirLos[alx + (aly * loshandler->airSizeX)]) {
							float extractDepth = extractDepthMap[y_hmapx + x];
							infoTexMem[a + COLOR_R] = (unsigned char)std::min(255.0f, 900.0f * fastmath::apxsqrt(fastmath::apxsqrt(extractDepth)));
						} else {
							infoTexMem[a + COLOR_R] = 0;
						}
						infoTexMem[a + COLOR_G] = (extraTexPal[extraTex[y_hmapx + x]*3 + 1]);
						infoTexMem[a + COLOR_B] = (extraTexPal[extraTex[y_hmapx + x]*3 + 2]);
						infoTexMem[a + COLOR_A] = 255;
					}
				}
				break;
			}
			case drawHeat: {
				for (int y = starty; y < endy; ++y) {
					const int y_pwr2hmapx = y * pwr2mapx_half;
					for (int x = 0; x  < gs->hmapx; ++x) {
						const int i = (y_pwr2hmapx + x) * 4 - offset;
						infoTexMem[i + COLOR_R] = (unsigned char)Clamp(8*pathManager->GetHeatOnSquare(x<<1, y<<1), 32, 255);
						infoTexMem[i + COLOR_G] = 32;
						infoTexMem[i + COLOR_B] = 32;
						infoTexMem[i + COLOR_A] = 255;
					}
				}
				break;
			}
			case drawHeight: {
				extraTexPal = heightLinePal->GetData();
				for (int y = starty; y < endy; ++y) {
					const int y_pwr2mapx = y * gs->pwr2mapx;
					const int y_mapx     = y * gs->mapx;
					for (int x = 0; x  < gs->mapx; ++x) {
						const float height = readmap->centerheightmap[y_mapx + x];
						const unsigned int value = (((unsigned int)(height * 8.0f)) % 255) * 3;
						const int i = (y_pwr2mapx + x) * 4 - offset;
						infoTexMem[i + COLOR_R] = 64 + (extraTexPal[value]     >> 1);
						infoTexMem[i + COLOR_G] = 64 + (extraTexPal[value + 1] >> 1);
						infoTexMem[i + COLOR_B] = 64 + (extraTexPal[value + 2] >> 1);
						infoTexMem[i + COLOR_A] = 255;
					}
				}
				break;
			}
			case drawLos: {
				const int lowRes = highResInfoTexWanted ? 0 : -1;
				const int endx = highResInfoTexWanted ? gs->mapx : gs->hmapx;
				const int pwr2mapx = gs->pwr2mapx >> (-lowRes);
				const int losSizeX = loshandler->losSizeX;
				const int losSizeY = loshandler->losSizeY;
				const int airSizeX = loshandler->airSizeX;
				const int airSizeY = loshandler->airSizeY;
				const int losMipLevel = loshandler->losMipLevel + lowRes;
				const int airMipLevel = loshandler->airMipLevel + lowRes;
				if (drawRadarAndJammer) {
					const int rxsize = radarhandler->xsize;
					const int rzsize = radarhandler->zsize;
					for (int y = starty; y < endy; ++y) {
						for (int x = 0; x < endx; ++x) {
							int totalLos = 255;
							if (!gs->globalLOS) {
								const int inLos = InterpolateLos(myLos,    losSizeX, losSizeY, losMipLevel, 128, x, y);
								const int inAir = InterpolateLos(myAirLos, airSizeX, airSizeY, airMipLevel, 128, x, y);
								totalLos = inLos + inAir;
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

							const int a = ((y * pwr2mapx) + x) * 4 - offset;
							for (int c = 0; c < 3; c++) {
								int val = alwaysColor[c] * 255;
								val += (jamColor[c]   * inJam);
								val += (losColor[c]   * totalLos);
								val += (radarColor[c] * inRadar);
								infoTexMem[a + (2 - c)] = (val / losColorScale);
							}
							infoTexMem[a + COLOR_A] = 255;
						}
					}
				}
				else {
					for (int y = starty; y < endy; ++y) {
						const int y_pwr2mapx = y * pwr2mapx;
						for (int x = 0; x < endx; ++x) {
							const int inLos = InterpolateLos(myLos,    losSizeX, losSizeY, losMipLevel, 32, x, y);
							const int inAir = InterpolateLos(myAirLos, airSizeX, airSizeY, airMipLevel, 32, x, y);
							const int value = 64 + inLos + inAir;
							const int a = (y_pwr2mapx + x) * 4 - offset;
							infoTexMem[a + COLOR_R] = value;
							infoTexMem[a + COLOR_G] = value;
							infoTexMem[a + COLOR_B] = value;
							infoTexMem[a + COLOR_A] = 255;
						}
					}
				}
				break;
			}
			case drawNormal:
				break;
		} // switch (drawMode)

#undef COLOR_R
#undef COLOR_G
#undef COLOR_B
#undef COLOR_A

		extraTexPBO.UnmapBuffer();
		/*
		glBindTexture(GL_TEXTURE_2D, infoTex);
		if(highResInfoTex)
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, starty, gs->pwr2mapx, (endy-starty), GL_BGRA, GL_UNSIGNED_BYTE, extraTexPBO.GetPtr(offset));
		else
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, starty, gs->pwr2mapx>>1, (endy-starty), GL_BGRA, GL_UNSIGNED_BYTE, extraTexPBO.GetPtr(offset));
		*/
		extraTexPBO.Unbind(false);

	} // if (updateTextureState < extraTextureUpdateRate)

	if (updateTextureState == extraTextureUpdateRate) {
		if (infoTex != 0 && highResInfoTexWanted != highResInfoTex) {
			glDeleteTextures(1,&infoTex);
			infoTex=0;
		}
		if (infoTex == 0) {
			extraTexPBO.Bind();
			glGenTextures(1,&infoTex);
			glBindTexture(GL_TEXTURE_2D, infoTex);

			//todo: maybe use GL_RGB5 as internalformat?
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
			if(highResInfoTexWanted)
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gs->pwr2mapx, gs->pwr2mapy, 0, GL_BGRA, GL_UNSIGNED_BYTE, extraTexPBO.GetPtr());
			else
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gs->pwr2mapx>>1, gs->pwr2mapy>>1, 0, GL_BGRA, GL_UNSIGNED_BYTE, extraTexPBO.GetPtr());
			extraTexPBO.Unbind(false);

			highResInfoTex = highResInfoTexWanted;
			updateTextureState = 0;
			return true;
		}

		extraTexPBO.Bind();
			glBindTexture(GL_TEXTURE_2D, infoTex);
			if(highResInfoTex)
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, gs->pwr2mapx, gs->pwr2mapy, GL_BGRA, GL_UNSIGNED_BYTE, extraTexPBO.GetPtr());
			else
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, gs->pwr2mapx>>1, gs->pwr2mapy>>1, GL_BGRA, GL_UNSIGNED_BYTE, extraTexPBO.GetPtr());
		extraTexPBO.Unbind(false);

		updateTextureState=0;
		return true;
	}

	updateTextureState++;
	return false;
}
