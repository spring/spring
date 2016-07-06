/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacyInfoTextureHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/UI/GuiHandler.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/HeightLinePalette.h"
#include "Map/MapInfo.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Rendering/IPathDrawer.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"
#include "System/Config/ConfigHandler.h"


CONFIG(bool, HighResLos).defaultValue(false).description("Controls whether LOS (\"L view\") edges are rendered in high resolution. Resource heavy!");
CONFIG(int, ExtraTextureUpdateRate).defaultValue(45).description("EXTREME CPU-HEAVY ON MEDIUM/BIG MAPS! DON'T CHANGE DEFAULT!");


static CLegacyInfoTextureHandler::BaseGroundDrawMode NameToDrawMode(const std::string& name)
{
	if (name == "los") {
		return CLegacyInfoTextureHandler::drawLos;
	} else
	if (name == "metal") {
		return CLegacyInfoTextureHandler::drawMetal;
	} else
	if (name == "height") {
		return CLegacyInfoTextureHandler::drawHeight;
	} else
	if (name == "path") {
		return CLegacyInfoTextureHandler::drawPathTrav;
	} else
	if (name == "heat") {
		return CLegacyInfoTextureHandler::drawPathHeat;
	} else
	if (name == "flow") {
		return CLegacyInfoTextureHandler::drawPathFlow;
	} else
	if (name == "pathcost") {
		return CLegacyInfoTextureHandler::drawPathCost;
	}

	// maps to 0
	return CLegacyInfoTextureHandler::drawNormal;
}


static std::string DrawModeToName(const CLegacyInfoTextureHandler::BaseGroundDrawMode mode)
{
	switch (mode) {
		case CLegacyInfoTextureHandler::drawNormal:   return "";
		case CLegacyInfoTextureHandler::drawLos:      return "los";
		case CLegacyInfoTextureHandler::drawMetal:    return "metal";
		case CLegacyInfoTextureHandler::drawHeight:   return "height";
		case CLegacyInfoTextureHandler::drawPathTrav: return "path";
		case CLegacyInfoTextureHandler::drawPathHeat: return "heat";
		case CLegacyInfoTextureHandler::drawPathFlow: return "flow";
		case CLegacyInfoTextureHandler::drawPathCost: return "pathcost";
		default: return "";
	}
}




CLegacyInfoTextureHandler::CLegacyInfoTextureHandler()
: drawMode(drawNormal)
, updateTextureState(0)
, extraTextureUpdateRate(0)
, returnToLOS(false)
, highResLosTex(false)
, highResInfoTexWanted(false)
{
	if (!infoTextureHandler)
		infoTextureHandler = this;

	infoTexPBO.Bind();
	infoTexPBO.New(mapDims.pwr2mapx * mapDims.pwr2mapy * 4);
	infoTexPBO.Unbind();

	highResLosTex = configHandler->GetBool("HighResLos");
	extraTextureUpdateRate = std::max(4, configHandler->GetInt("ExtraTextureUpdateRate") - 1);

	memset(&infoTextureIDs[0], 0, sizeof(infoTextureIDs));
	glGenTextures(NUM_INFOTEXTURES - 1, &infoTextureIDs[1]);

	infoTextures.reserve(NUM_INFOTEXTURES);
	infoTextures.emplace_back("", 0, int2(0,0));
	for (unsigned int i = 1; i < NUM_INFOTEXTURES; i++) {
		BaseGroundDrawMode e = (BaseGroundDrawMode)i;

		bool hiresTex = false;
		if (e == drawLos && highResLosTex)
			hiresTex = true;
		if (e == drawHeight)
			hiresTex = true;
		int2 texSize(mapDims.pwr2mapx>>1, mapDims.pwr2mapy>>1);
		if (hiresTex) texSize = int2(mapDims.pwr2mapx, mapDims.pwr2mapy);

		glBindTexture(GL_TEXTURE_2D, infoTextureIDs[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glSpringTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, texSize.x, texSize.y);

		infoTextures.emplace_back(DrawModeToName(e), infoTextureIDs[i], texSize);
	}

	Update();
}


CLegacyInfoTextureHandler::~CLegacyInfoTextureHandler()
{
	glDeleteTextures(NUM_INFOTEXTURES, &infoTextureIDs[0]);
}


bool CLegacyInfoTextureHandler::IsEnabled() const
{
	return (drawMode != drawNormal);
}


void CLegacyInfoTextureHandler::DisableCurrentMode()
{
	highResInfoTexWanted = (returnToLOS && highResLosTex);

	if (returnToLOS && (drawMode != drawLos)) {
		// return to LOS-mode if it was active before
		SetMode("los");
	} else {
		returnToLOS = false;
		SetMode("");
	}
}


void CLegacyInfoTextureHandler::SetMode(const std::string& name)
{
	drawMode = NameToDrawMode(name);
	switch (drawMode) {
		case drawLos: {
			returnToLOS = true;
			highResInfoTexWanted = highResLosTex;
		} break;
		case drawHeight: {
			highResInfoTexWanted = true;
		} break;
		case drawMetal:
		case drawPathTrav:
		case drawPathHeat:
		case drawPathFlow:
		case drawPathCost: {
			highResInfoTexWanted = false;
		} break;
		case drawNormal: break; //TODO?
		case NUM_INFOTEXTURES: break; //make compiler happy
	}

	updateTextureState = 0;
	while (!UpdateExtraTexture(drawMode));
}


void CLegacyInfoTextureHandler::ToggleMode(const std::string& name)
{
	if (drawMode == NameToDrawMode(name))
		return DisableCurrentMode();

	SetMode(name);
}


const std::string& CLegacyInfoTextureHandler::GetMode() const
{
	//return DrawModeToName(drawMode); (warning: returning reference to temporary)
	return infoTextures[drawMode].GetName();
}


GLuint CLegacyInfoTextureHandler::GetCurrentInfoTexture() const
{
	return infoTextureIDs[drawMode];
}


int2 CLegacyInfoTextureHandler::GetCurrentInfoTextureSize() const
{
	if (highResInfoTexWanted)
		return (int2(mapDims.pwr2mapx, mapDims.pwr2mapy));

	return (int2(mapDims.pwr2mapx >> 1, mapDims.pwr2mapy >> 1));
}


const CInfoTexture* CLegacyInfoTextureHandler::GetInfoTextureConst(const std::string& name) const
{
	return &infoTextures[NameToDrawMode(name)];
}

CInfoTexture* CLegacyInfoTextureHandler::GetInfoTexture(const std::string& name)
{
	return &infoTextures[NameToDrawMode(name)];
}


/********************************************************************/
/********************************************************************/

static inline int InterpolateLos(const unsigned short* p, int2 size, int mip, int factor, int2 pos)
{
	const int x1 = pos.x >> mip;
	const int y1 = pos.y >> mip;
	const int s1 = (p[(y1 * size.x) + x1] != 0); // top left
	if (mip > 0) {
		int x2 = x1 + 1;
		int y2 = y1 + 1;
		if (x2 >= size.x) { x2 = size.x - 1; }
		if (y2 >= size.y) { y2 = size.y - 1; }
		const int s2 = (p[(y1 * size.x) + x2] != 0); // top right
		const int s3 = (p[(y2 * size.x) + x1] != 0); // bottom left
		const int s4 = (p[(y2 * size.x) + x2] != 0); // bottom right
		const int size_  = (1 << mip);
		const float fracx = float(pos.x % size_) / size_;
		const float fracy = float(pos.y % size_) / size_;
		const float c1 = (s2 - s1) * fracx + s1;
		const float c2 = (s4 - s3) * fracx + s3;
		return factor * ((c2 - c1) * fracy + c1);
	}
	return factor * s1;
}



void CLegacyInfoTextureHandler::Update()
{
	UpdateExtraTexture(drawMode);
}


// Gradually calculate the extra texture based on updateTextureState:
//   updateTextureState < extraTextureUpdateRate:   Calculate the texture color values and copy them into buffer
//   updateTextureState = extraTextureUpdateRate:   Copy the buffer into a texture
// NOTE:
//   when switching TO an info-texture drawing mode
//   the texture is calculated in its entirety, not
//   over multiple frames
bool CLegacyInfoTextureHandler::UpdateExtraTexture(BaseGroundDrawMode texDrawMode)
{
	if (texDrawMode == drawNormal)
		return true;

	if (mapInfo->map.voidWater && readMap->IsUnderWater())
		return true;

	if (updateTextureState < extraTextureUpdateRate) {
		const int pwr2mapx_half = mapDims.pwr2mapx >> 1;

		CBaseGroundDrawer* gd = readMap->GetGroundDrawer();

		int starty;
		int endy;
		int offset;

		// pointer to GPU-memory mapped into our address space
		unsigned char* infoTexMem = NULL;

		infoTexPBO.Bind();

		if (highResInfoTexWanted) {
			starty = updateTextureState * mapDims.mapy / extraTextureUpdateRate;
			endy = (updateTextureState + 1) * mapDims.mapy / extraTextureUpdateRate;

			offset = starty * mapDims.pwr2mapx * 4;
			infoTexMem = reinterpret_cast<unsigned char*>(infoTexPBO.MapBuffer(offset, (endy - starty) * mapDims.pwr2mapx * 4));
		} else {
			starty = updateTextureState * mapDims.hmapy / extraTextureUpdateRate;
			endy = (updateTextureState + 1) * mapDims.hmapy / extraTextureUpdateRate;

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

				const unsigned short* myLos           = &losHandler->los.losMaps[gu->myAllyTeam].front();
				const unsigned  char* extraTex        = metalMap->GetDistributionMap();
				const unsigned  char* extraTexPal     = metalMap->GetTexturePalette();
				const          float* extractDepthMap = metalMap->GetExtractionMap();

				for (int y = starty; y < endy; ++y) {
					const int y_pwr2mapx_half = y*pwr2mapx_half;
					const int y_hmapx = y * mapDims.hmapx;

					for (int x = 0; x < mapDims.hmapx; ++x) {
						const int a   = (y_pwr2mapx_half + x) * 4 - offset;
						const int alx = ((x*2) >> losHandler->los.mipLevel);
						const int aly = ((y*2) >> losHandler->los.mipLevel);

						if (myLos[alx + (aly * losHandler->los.size.x)]) {
							infoTexMem[a + COLOR_R] = (unsigned char)std::min(255.0f, 900.0f * fastmath::apxsqrt(fastmath::apxsqrt(extractDepthMap[y_hmapx + x])));
						} else {
							infoTexMem[a + COLOR_R] = 0;
						}

						infoTexMem[a + COLOR_G] = (extraTexPal[extraTex[y_hmapx + x]*3 + 1]);
						infoTexMem[a + COLOR_B] = (extraTexPal[extraTex[y_hmapx + x]*3 + 2]);
						infoTexMem[a + COLOR_A] = 255;
					}
				}
			} break;

			case drawHeight: {
				const SColor* extraTexPal = CHeightLinePalette::GetData();

				// NOTE:
				//   PBO/ExtraTexture resolution is always mapDims.pwr2mapx * mapDims.pwr2mapy
				//   (we don't use it fully) while the CORNER heightmap has dimensions
				//   (mapDims.mapx + 1) * (mapDims.mapy + 1). In case POT(mapDims.mapx) == mapDims.mapx
				//   we would therefore get a buffer overrun in our PBO when iterating
				//   over column (mapDims.mapx + 1) or row (mapDims.mapy + 1) so we select the
				//   easiest solution and just skip those squares.
				const float* heightMap = readMap->GetCornerHeightMapUnsynced();

				for (int y = starty; y < endy; ++y) {
					const int y_pwr2mapx = y * mapDims.pwr2mapx;
					const int y_mapx     = y * mapDims.mapxp1;

					for (int x = 0; x < mapDims.mapx; ++x) {
						const float height = heightMap[y_mapx + x];
						const unsigned int value = ((unsigned int)(height * 8.0f)) % 255;
						const int i = (y_pwr2mapx + x) * 4 - offset;

						infoTexMem[i + COLOR_R] = extraTexPal[value].r;
						infoTexMem[i + COLOR_G] = extraTexPal[value].g;
						infoTexMem[i + COLOR_B] = extraTexPal[value].b;
						infoTexMem[i + COLOR_A] = extraTexPal[value].a;
					}
				}
			} break;

			case drawLos: {
				const int jammerAllyTeam = modInfo.separateJammers ? gu->myAllyTeam : 0;
				const unsigned short* myLos         = &losHandler->los.losMaps[gu->myAllyTeam].front();
				const unsigned short* myAirLos      = &losHandler->airLos.losMaps[gu->myAllyTeam].front();
				const unsigned short* myRadar       = &losHandler->radar.losMaps[gu->myAllyTeam].front();
				const unsigned short* myJammer      = &losHandler->jammer.losMaps[jammerAllyTeam].front();

				const int lowRes = highResInfoTexWanted ? 0 : -1;
				const int endx = highResInfoTexWanted ? mapDims.mapx : mapDims.hmapx;
				const int pwr2mapx = mapDims.pwr2mapx >> (-lowRes);
				const int losMipLevel = losHandler->los.mipLevel + lowRes;
				const int airMipLevel = losHandler->airLos.mipLevel + lowRes;
				const int radarMipLevel = losHandler->radar.mipLevel + lowRes;
				const int jammerMipLevel = losHandler->jammer.mipLevel + lowRes;

				for (int y = starty; y < endy; ++y) {
					for (int x = 0; x < endx; ++x) {
						int totalLos = 255;
						int2 pos(x,y);

						if (!losHandler->globalLOS[gu->myAllyTeam]) {
							const int inLos = InterpolateLos(myLos,    losHandler->los.size, losMipLevel, 128, pos);
							const int inAir = InterpolateLos(myAirLos, losHandler->airLos.size, airMipLevel, 127, pos);
							totalLos = inLos + inAir;
						}

						const int inRadar = InterpolateLos(myRadar, losHandler->radar.size, radarMipLevel, 255, pos);
						const int inJam   = (totalLos == 255) ? InterpolateLos(myJammer, losHandler->jammer.size, jammerMipLevel, 255, pos) : 0;

						const int a = ((y * pwr2mapx) + x) * 4 - offset;

						for (int c = 0; c < 3; c++) {
							int val = gd->alwaysColor[c] * 255;
							val += (gd->jamColor[c]   * inJam); //FIXME
							val += (gd->losColor[c]   * totalLos);
							val += (gd->radarColor[c] * inRadar);
							infoTexMem[a + (2 - c)] = (val / gd->losColorScale);
						}

						infoTexMem[a + COLOR_A] = 255;
					}
				}
				break;
			}

			case drawNormal: {
				assert(false);
			} break;
			case NUM_INFOTEXTURES: break; //make compiler happy
		}

		infoTexPBO.UnmapBuffer();
		/*
		glBindTexture(GL_TEXTURE_2D, infoTextureIDs[texDrawMode]);

		if (highResInfoTexWanted) {
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, starty, mapDims.pwr2mapx, (endy-starty), GL_BGRA, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr(offset));
		} else {
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, starty, mapDims.pwr2mapx>>1, (endy-starty), GL_BGRA, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr(offset));
		}
		*/

		infoTexPBO.Unbind();
	}


	if (updateTextureState == extraTextureUpdateRate) {
		// entire texture has been updated, now check if it
		// needs to be regenerated as well (eg. if switching
		// between textures of different resolutions)

		infoTexPBO.Bind();
			glBindTexture(GL_TEXTURE_2D, infoTextureIDs[texDrawMode]);

			if (highResInfoTexWanted) {
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mapDims.pwr2mapx, mapDims.pwr2mapy, GL_BGRA, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr());
			} else {
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mapDims.pwr2mapx>>1, mapDims.pwr2mapy>>1, GL_BGRA, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr());
			}
		infoTexPBO.Invalidate();
		infoTexPBO.Unbind();

		updateTextureState = 0;
		return true;
	}

	updateTextureState++;
	return false;
}
