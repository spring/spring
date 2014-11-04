/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BaseGroundDrawer.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/UI/GuiHandler.h"
#include "Ground.h"
#include "HeightLinePalette.h"
#include "MetalMap.h"
#include "ReadMap.h"
#include "MapInfo.h"
#include "Rendering/IPathDrawer.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/FastMath.h"
#include "System/myMath.h"

CONFIG(float, GroundLODScaleReflection).defaultValue(1.0f).headlessValue(0.0f);
CONFIG(float, GroundLODScaleRefraction).defaultValue(1.0f).headlessValue(0.0f);
CONFIG(float, GroundLODScaleTerrainReflection).defaultValue(1.0f);
CONFIG(bool, HighResLos).defaultValue(false).description("Controls whether LOS (\"L view\") edges are rendered in high resolution. Resource heavy!");
CONFIG(int, ExtraTextureUpdateRate).defaultValue(45);

CBaseGroundDrawer::CBaseGroundDrawer()
{
	LODScaleReflection = configHandler->GetFloat("GroundLODScaleReflection");
	LODScaleRefraction = configHandler->GetFloat("GroundLODScaleRefraction");
	LODScaleTerrainReflection = configHandler->GetFloat("GroundLODScaleTerrainReflection");

	memset(&infoTextureIDs[0], 0, sizeof(infoTextureIDs));

	drawMode = drawNormal;
	drawLineOfSight = false;
	drawMapEdges = false;
	drawDeferred = false;
	wireframe = false;
	advShading = false;
	highResInfoTex = false;
	updateTextureState = 0;

	infoTexPBO.Bind();
	infoTexPBO.New(gs->pwr2mapx * gs->pwr2mapy * 4);
	infoTexPBO.Unbind();

	highResInfoTexWanted = false;

	highResLosTex = configHandler->GetBool("HighResLos");
	extraTextureUpdateRate = std::max(4, configHandler->GetInt("ExtraTextureUpdateRate") - 1);

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

	groundTextures = NULL;
}


CBaseGroundDrawer::~CBaseGroundDrawer()
{
	for (unsigned int n = 0; n < NUM_INFOTEXTURES; n++) {
		glDeleteTextures(1, &infoTextureIDs[n]);
	}
}



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
			glBindTexture(GL_TEXTURE_2D, infoTextureIDs[drawMode]);
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



// XXX this part of extra textures is a mess really ...
void CBaseGroundDrawer::DisableExtraTexture()
{
	highResInfoTexWanted = (drawLineOfSight && highResLosTex);
	updateTextureState = 0;

	if (drawLineOfSight) {
		// return to LOS-mode if it was active before
		SetDrawMode(drawLos);

		while (!UpdateExtraTexture(drawMode));
	} else {
		SetDrawMode(drawNormal);
	}
}


void CBaseGroundDrawer::SetHeightTexture()
{
	if (drawMode == drawHeight) {
		DisableExtraTexture();
	} else {
		SetDrawMode(drawHeight);

		highResInfoTexWanted = true;
		updateTextureState = 0;

		while (!UpdateExtraTexture(drawMode));
	}
}



void CBaseGroundDrawer::SetMetalTexture()
{
	if (drawMode == drawMetal) {
		DisableExtraTexture();
	} else {
		SetDrawMode(drawMetal);

		highResInfoTexWanted = false;
		updateTextureState = 0;

		while (!UpdateExtraTexture(drawMode));
	}
}


void CBaseGroundDrawer::TogglePathTexture(BaseGroundDrawMode mode)
{
	switch (mode) {
		case drawPathTrav:
		case drawPathHeat:
		case drawPathFlow:
		case drawPathCost: {
			if (drawMode == mode) {
				DisableExtraTexture();
			} else {
				SetDrawMode(mode);

				highResInfoTexWanted = false;
				updateTextureState = 0;

				while (!UpdateExtraTexture(drawMode));
			}
		} break;

		default: {
		} break;
	}
}


void CBaseGroundDrawer::ToggleLosTexture()
{
	if (drawMode == drawLos) {
		drawLineOfSight = false;
		DisableExtraTexture();
	} else {
		drawLineOfSight = true;

		SetDrawMode(drawLos);
		highResInfoTexWanted = highResLosTex;
		updateTextureState = 0;

		while (!UpdateExtraTexture(drawMode));
	}
}



static inline int InterpolateLos(
	const unsigned short* p,
	int xsize,
	int ysize,
	int mip,
	int factor,
	int x,
	int y
) {
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
//   updateTextureState < extraTextureUpdateRate:   Calculate the texture color values and copy them into buffer
//   updateTextureState = extraTextureUpdateRate:   Copy the buffer into a texture
// NOTE:
//   when switching TO an info-texture drawing mode
//   the texture is calculated in its entirety, not
//   over multiple frames
bool CBaseGroundDrawer::UpdateExtraTexture(unsigned int texDrawMode)
{
	assert(texDrawMode != drawNormal);

	if (mapInfo->map.voidWater && readMap->IsUnderWater())
		return true;

	if (updateTextureState < extraTextureUpdateRate) {
		const int pwr2mapx_half = gs->pwr2mapx >> 1;

		int starty;
		int endy;
		int offset;

		// pointer to GPU-memory mapped into our address space
		unsigned char* infoTexMem = NULL;

		infoTexPBO.Bind();

		if (highResInfoTexWanted) {
			starty = updateTextureState * gs->mapy / extraTextureUpdateRate;
			endy = (updateTextureState + 1) * gs->mapy / extraTextureUpdateRate;

			offset = starty * gs->pwr2mapx * 4;
			infoTexMem = reinterpret_cast<unsigned char*>(infoTexPBO.MapBuffer(offset, (endy - starty) * gs->pwr2mapx * 4));
		} else {
			starty = updateTextureState * gs->hmapy / extraTextureUpdateRate;
			endy = (updateTextureState + 1) * gs->hmapy / extraTextureUpdateRate;

			offset = starty * pwr2mapx_half * 4;
			infoTexMem = reinterpret_cast<unsigned char*>(infoTexPBO.MapBuffer(offset, (endy - starty) * pwr2mapx_half * 4));
		}

		switch (texDrawMode) {
			case drawPathTrav:
			case drawPathHeat:
			case drawPathFlow:
			case drawPathCost: {
				pathDrawer->UpdateExtraTexture(texDrawMode, starty, endy, offset, infoTexMem);
			} break;

			case drawMetal: {
				const CMetalMap* metalMap = readMap->metalMap;

				const unsigned short* myAirLos        = &losHandler->airLosMaps[gu->myAllyTeam].front();
				const unsigned  char* extraTex        = metalMap->GetDistributionMap();
				const unsigned  char* extraTexPal     = metalMap->GetTexturePalette();
				const          float* extractDepthMap = metalMap->GetExtractionMap();

				for (int y = starty; y < endy; ++y) {
					const int y_pwr2mapx_half = y*pwr2mapx_half;
					const int y_hmapx = y * gs->hmapx;

					for (int x = 0; x < gs->hmapx; ++x) {
						const int a   = (y_pwr2mapx_half + x) * 4 - offset;
						const int alx = ((x*2) >> losHandler->airMipLevel);
						const int aly = ((y*2) >> losHandler->airMipLevel);

						if (myAirLos[alx + (aly * losHandler->airSizeX)]) {
							infoTexMem[a + COLOR_R] = (unsigned char)std::min(255.0f, 900.0f * fastmath::apxsqrt(fastmath::apxsqrt(extractDepthMap[y_hmapx + x])));
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

			case drawHeight: {
				const SColor* extraTexPal = CHeightLinePalette::GetData();

				// NOTE:
				//   PBO/ExtraTexture resolution is always gs->pwr2mapx * gs->pwr2mapy
				//   (we don't use it fully) while the CORNER heightmap has dimensions
				//   (gs->mapx + 1) * (gs->mapy + 1). In case POT(gs->mapx) == gs->mapx
				//   we would therefore get a buffer overrun in our PBO when iterating
				//   over column (gs->mapx + 1) or row (gs->mapy + 1) so we select the
				//   easiest solution and just skip those squares.
				const float* heightMap = readMap->GetCornerHeightMapUnsynced();

				for (int y = starty; y < endy; ++y) {
					const int y_pwr2mapx = y * gs->pwr2mapx;
					const int y_mapx     = y * gs->mapxp1;

					for (int x = 0; x < gs->mapx; ++x) {
						const float height = heightMap[y_mapx + x];
						const unsigned int value = ((unsigned int)(height * 8.0f)) % 255;
						const int i = (y_pwr2mapx + x) * 4 - offset;

						infoTexMem[i + COLOR_R] = extraTexPal[value].r;
						infoTexMem[i + COLOR_G] = extraTexPal[value].g;
						infoTexMem[i + COLOR_B] = extraTexPal[value].b;
						infoTexMem[i + COLOR_A] = extraTexPal[value].a;
					}
				}

				break;
			}

			case drawLos: {
				const unsigned short* myLos         = &losHandler->losMaps[gu->myAllyTeam].front();
				const unsigned short* myAirLos      = &losHandler->airLosMaps[gu->myAllyTeam].front();
				const unsigned short* myRadar       = &radarHandler->radarMaps[gu->myAllyTeam].front();
				const unsigned short* myJammer      = &radarHandler->jammerMaps[gu->myAllyTeam].front();
			#ifdef RADARHANDLER_SONAR_JAMMER_MAPS
				const unsigned short* mySonar       = &radarHandler->sonarMaps[gu->myAllyTeam].front();
				const unsigned short* mySonarJammer = &radarHandler->sonarJammerMaps[gu->myAllyTeam].front();
			#endif

				const int lowRes = highResInfoTexWanted ? 0 : -1;
				const int endx = highResInfoTexWanted ? gs->mapx : gs->hmapx;
				const int pwr2mapx = gs->pwr2mapx >> (-lowRes);
				const int losSizeX = losHandler->losSizeX;
				const int losSizeY = losHandler->losSizeY;
				const int airSizeX = losHandler->airSizeX;
				const int airSizeY = losHandler->airSizeY;
				const int losMipLevel = losHandler->losMipLevel + lowRes;
				const int airMipLevel = losHandler->airMipLevel + lowRes;
				const int rxsize = radarHandler->xsize;
				const int rzsize = radarHandler->zsize;

				for (int y = starty; y < endy; ++y) {
					for (int x = 0; x < endx; ++x) {
						int totalLos = 255;

						if (!gs->globalLOS[gu->myAllyTeam]) {
							const int inLos = InterpolateLos(myLos,    losSizeX, losSizeY, losMipLevel, 128, x, y);
							const int inAir = InterpolateLos(myAirLos, airSizeX, airSizeY, airMipLevel, 128, x, y);
							totalLos = inLos + inAir;
						}
					#ifdef RADARHANDLER_SONAR_JAMMER_MAPS
						const bool useRadar = (CGround::GetHeightReal(xPos, zPos, false) >= 0.0f);
						const unsigned short* radarMap  = useRadar ? myRadar  : mySonar;
						const unsigned short* jammerMap = useRadar ? myJammer : mySonarJammer;
					#else
						const unsigned short* radarMap  = myRadar;
						const unsigned short* jammerMap = myJammer;
					#endif
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
				break;
			}

			case drawNormal: {
				assert(false);
			} break;
		}

		infoTexPBO.UnmapBuffer();
		/*
		glBindTexture(GL_TEXTURE_2D, infoTextureIDs[texDrawMode]);

		if (highResInfoTex) {
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, starty, gs->pwr2mapx, (endy-starty), GL_BGRA, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr(offset));
		} else {
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, starty, gs->pwr2mapx>>1, (endy-starty), GL_BGRA, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr(offset));
		}
		*/

		infoTexPBO.Unbind();
	}


	if (updateTextureState == extraTextureUpdateRate) {
		// entire texture has been updated, now check if it
		// needs to be regenerated as well (eg. if switching
		// between textures of different resolutions)
		//
		if (infoTextureIDs[texDrawMode] != 0 && highResInfoTexWanted != highResInfoTex) {
			glDeleteTextures(1, &infoTextureIDs[texDrawMode]);
			infoTextureIDs[texDrawMode] = 0;
		}

		if (infoTextureIDs[texDrawMode] == 0) {
			// generate new texture
			infoTexPBO.Bind();
			glGenTextures(1, &infoTextureIDs[texDrawMode]);
			glBindTexture(GL_TEXTURE_2D, infoTextureIDs[texDrawMode]);

			// XXX maybe use GL_RGB5 as internalformat?
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			if (highResInfoTexWanted) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gs->pwr2mapx, gs->pwr2mapy, 0, GL_BGRA, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr());
			} else {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gs->pwr2mapx>>1, gs->pwr2mapy>>1, 0, GL_BGRA, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr());
			}

			infoTexPBO.Invalidate();
			infoTexPBO.Unbind();

			highResInfoTex = highResInfoTexWanted;
			updateTextureState = 0;
			return true;
		}

		infoTexPBO.Bind();
			glBindTexture(GL_TEXTURE_2D, infoTextureIDs[texDrawMode]);

			if (highResInfoTex) {
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, gs->pwr2mapx, gs->pwr2mapy, GL_BGRA, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr());
			} else {
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, gs->pwr2mapx>>1, gs->pwr2mapy>>1, GL_BGRA, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr());
			}
		infoTexPBO.Invalidate();
		infoTexPBO.Unbind();

		updateTextureState = 0;
		return true;
	}

	updateTextureState++;
	return false;
}



int2 CBaseGroundDrawer::GetInfoTexSize() const
{
	if (highResInfoTex)
		return (int2(gs->pwr2mapx, gs->pwr2mapy));

	return (int2(gs->pwr2mapx >> 1, gs->pwr2mapy >> 1));
}


void CBaseGroundDrawer::UpdateCamRestraints(CCamera* cam)
{
	// add restraints for camera sides
	cam->GetFrustumSides(readMap->GetCurrMinHeight() - 100.0f, readMap->GetCurrMaxHeight() + 30.0f,  SQUARE_SIZE);

	// CAMERA DISTANCE IS ALREADY CHECKED IN CGroundDrawer::GridVisibility()!
/*
	// add restraint for maximum view distance (use flat z-dir as side)
	// this is supposed to prevent (far) terrain from first being drawn
	// and then immediately z-clipped away
	const float3& camDir3D = cam->forward;

	// prevent colinearity in top-down view
	if (math::fabs(camDir3D.dot(UpVector)) < 0.95f) {
		float3 camDir2D  = float3(camDir3D.x, 0.0f, camDir3D.z).SafeANormalize();
		float3 camOffset = camDir2D * globalRendering->viewRange * 1.05f;

		// FIXME magic constants
		static const float miny = 0.0f;
		static const float maxy = 255.0f / 3.5f;
		cam->GetFrustumSide(camDir2D, camOffset, miny, maxy, SQUARE_SIZE, (camDir3D.y > 0.0f), false);
	}
*/
}
