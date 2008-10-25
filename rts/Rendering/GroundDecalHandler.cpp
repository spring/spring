#include "StdAfx.h"
#include <algorithm>
#include <cctype>
#include "mmgr.h"

#include "GroundDecalHandler.h"
#include "Game/Camera.h"
#include "Lua/LuaParser.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Platform/ConfigHandler.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"
#include "System/Util.h"
#include "System/Exceptions.h"

using std::list;
using std::min;
using std::max;

CGroundDecalHandler* groundDecals = NULL;


CGroundDecalHandler::CGroundDecalHandler(void)
{
	drawDecals = false;
	decalLevel = std::max(0, std::min(5, configHandler.GetInt("GroundDecals", 1)));
	groundScarAlphaFade = configHandler.GetInt("GroundScarAlphaFade", 0);

	if (decalLevel == 0) {
		return;
	}

	drawDecals = true;

	unsigned char* buf=SAFE_NEW unsigned char[512*512*4];
	memset(buf,0,512*512*4);

	LuaParser resourcesParser("gamedata/resources.lua",
	                          SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!resourcesParser.Execute()) {
		logOutput.Print(resourcesParser.GetErrorLog());
	}

	const LuaTable scarsTable = resourcesParser.GetRoot().SubTable("graphics").SubTable("scars");
	LoadScar("bitmaps/" + scarsTable.GetString(2, "scars/scar2.bmp"), buf, 0,   0);
	LoadScar("bitmaps/" + scarsTable.GetString(3, "scars/scar3.bmp"), buf, 256, 0);
	LoadScar("bitmaps/" + scarsTable.GetString(1, "scars/scar1.bmp"), buf, 0,   256);
	LoadScar("bitmaps/" + scarsTable.GetString(4, "scars/scar4.bmp"), buf, 256, 256);

	glGenTextures(1, &scarTex);
	glBindTexture(GL_TEXTURE_2D, scarTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBuildMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,512, 512, GL_RGBA, GL_UNSIGNED_BYTE, buf);

	scarFieldX=gs->mapx/32;
	scarFieldY=gs->mapy/32;
	scarField=SAFE_NEW std::set<Scar*>[scarFieldX*scarFieldY];

	lastTest=0;
	maxOverlap=decalLevel+1;

	delete[] buf;

	if(shadowHandler->canUseShadows){
		decalVP=LoadVertexProgram("grounddecals.vp");
		decalFP=LoadFragmentProgram("grounddecals.fp");
	}
}


CGroundDecalHandler::~CGroundDecalHandler(void)
{
	for(std::vector<TrackType*>::iterator tti=trackTypes.begin();tti!=trackTypes.end();++tti){
		for(set<UnitTrackStruct*>::iterator ti=(*tti)->tracks.begin();ti!=(*tti)->tracks.end();++ti){
			delete *ti;
		}
		glDeleteTextures (1, &(*tti)->texture);
		delete *tti;
	}
	for(std::vector<BuildingDecalType*>::iterator tti=buildingDecalTypes.begin();tti!=buildingDecalTypes.end();++tti){
		for(set<BuildingGroundDecal*>::iterator ti=(*tti)->buildingDecals.begin();ti!=(*tti)->buildingDecals.end();++ti){
			if((*ti)->owner)
				(*ti)->owner->buildingDecal=0;
			if((*ti)->gbOwner)
				(*ti)->gbOwner->decal=0;
			delete *ti;
		}
		glDeleteTextures (1, &(*tti)->texture);
		delete *tti;
	}
	for(std::list<Scar*>::iterator si=scars.begin();si!=scars.end();++si){
		delete *si;
	}
	if(decalLevel!=0){
		delete[] scarField;

		glDeleteTextures(1,&scarTex);
	}
	if(shadowHandler->canUseShadows){
		glSafeDeleteProgram(decalVP);
		glSafeDeleteProgram(decalFP);
	}
}



static inline void AddQuadVertices(CVertexArray* va, int x, float* yv, int z, const float* uv, unsigned char* color)
{
	#define HEIGHT2WORLD(x) ((x) << 3)
	#define VERTEX(x, y, z) float3(HEIGHT2WORLD((x)), (y), HEIGHT2WORLD((z)))
	va->AddVertexTC( VERTEX(x    , yv[0], z    ),   uv[0], uv[1],   color);
	va->AddVertexTC( VERTEX(x + 1, yv[1], z    ),   uv[2], uv[3],   color);
	va->AddVertexTC( VERTEX(x + 1, yv[2], z + 1),   uv[4], uv[5],   color);
	va->AddVertexTC( VERTEX(x    , yv[3], z + 1),   uv[6], uv[7],   color);
	#undef VERTEX
	#undef HEIGHT2WORLD
}

static inline void DrawBuildingDecal(BuildingGroundDecal* decal)
{
	const float* hm = readmap->GetHeightmap();
	const int gsmx = gs->mapx;
	const int gsmx1 = gsmx + 1;
	const int gsmy = gs->mapy;

	float yv[4] = {0.0f};
	float uv[8] = {0.0f};
	unsigned char color[4] = {255, 255, 255, int(decal->alpha * 255)};

	#define HEIGHT(z, x) (hm[((z) * gsmx1) + (x)])

	if (!decal->va) {
		// NOTE: this really needs CLOD'ing
		decal->va = SAFE_NEW CVertexArray();
		decal->va->Initialize();

		const float xts = 1.0f / decal->xsize;
		const float yts = 1.0f / decal->ysize;

		int xMin = 0, xMax = decal->xsize;			// x-extends in local decal-space (heightmap scale)
		int zMin = 0, zMax = decal->ysize;			// z-extends in local decal-space (heightmap scale)
		int tlx = decal->posx + xMin, blx = tlx;	// heightmap x-coor of {top, bottom}-left   quad vertex
		int trx = decal->posx + xMax, brx = trx;	// heightmap x-coor of {top, bottom}-right  quad vertex
		int tlz = decal->posy + zMin, trz = tlz;	// heightmap z-coor of top-{left, right}    quad vertex
		int brz = decal->posy + zMax, blz = brz;	// heightmap z-coor of bottom-{left, right} quad vertex

		switch (decal->facing) {
			case 0: { // South (determines our reference texcoors)
				// clip the quad vertices and texcoors against the map boundaries
				if (tlx <    0) { xMin -= tlx       ;   tlx =    0; blx = tlx; }
				if (trx > gsmx) { xMax -= trx - gsmx;   trx = gsmx; brx = trx; }
				if (tlz <    0) { zMin -= tlz       ;   tlz =    0; trz = tlz; }
				if (brz > gsmy) { zMax -= brz - gsmy;   brz = gsmy; blz = brz; }

				for (int x = xMin; x < xMax; x++) {
					const int xh = tlx + x;

					for (int z = zMin; z < zMax; z++) {
						const int zh = tlz + z;

						// (htl, htr, hbr, hbl)
						yv[0] = HEIGHT(zh,     xh    ); yv[1] = HEIGHT(zh,     xh + 1);
						yv[2] = HEIGHT(zh + 1, xh + 1); yv[3] = HEIGHT(zh + 1, xh    );

						// (utl, vtl),  (utr, vtr)
						// (ubr, vbr),  (ubl, vbl)
						uv[0] = (x    ) * xts; uv[1] = (z    ) * yts; // uv = (0, 0)
						uv[2] = (x + 1) * xts; uv[3] = (z    ) * yts; // uv = (1, 0)
						uv[4] = (x + 1) * xts; uv[5] = (z + 1) * yts; // uv = (1, 1)
						uv[6] = (x    ) * xts; uv[7] = (z + 1) * yts; // uv = (0, 1)

						AddQuadVertices(decal->va, xh, yv, zh, uv, color);
					}
				}
			} break;

			case 1: { // East
				if (tlx <    0) { zMin -= tlx       ; tlx =    0; blx = tlx; }
				if (trx > gsmx) { zMax -= trx - gsmx; trx = gsmx; brx = trx; }
				if (tlz <    0) { xMax += tlz       ; tlz =    0; trz = tlz; }
				if (brz > gsmy) { xMin += brz - gsmy; brz = gsmy; blz = brz; }

				for (int x = xMin; x < xMax; x++) {
					const int xh = tlx + x;

					for (int z = zMin; z < zMax; z++) {
						const int zh = tlz + z;

						yv[0] = HEIGHT(zh,     xh    ); yv[1] = HEIGHT(zh,     xh + 1);
						yv[2] = HEIGHT(zh + 1, xh + 1); yv[3] = HEIGHT(zh + 1, xh    );

						uv[0] = 1.0f - (z    ) * yts; uv[1] = (x    ) * xts; // uv = (1, 0)
						uv[2] = 1.0f - (z    ) * yts; uv[3] = (x + 1) * xts; // uv = (1, 1)
						uv[4] = 1.0f - (z + 1) * yts; uv[5] = (x + 1) * xts; // uv = (0, 1)
						uv[6] = 1.0f - (z + 1) * yts; uv[7] = (x    ) * xts; // uv = (0, 0)

						AddQuadVertices(decal->va, xh, yv, zh, uv, color);
					}
				}
			} break;

			case 2: { // North
				if (tlx <    0) { xMax += tlx       ; tlx =    0; blx = tlx; }
				if (trx > gsmx) { xMin += trx - gsmx; trx = gsmx; brx = trx; }
				if (tlz <    0) { zMax += tlz       ; tlz =    0; trz = tlz; }
				if (brz > gsmy) { zMin += brz - gsmy; brz = gsmy; blz = brz; }

				for (int x = xMin; x < xMax; x++) {
					const int xh = tlx + x;

					for (int z = zMin; z < zMax; z++) {
						const int zh = tlz + z;

						yv[0] = HEIGHT(zh,     xh    ); yv[1] = HEIGHT(zh,     xh + 1);
						yv[2] = HEIGHT(zh + 1, xh + 1); yv[3] = HEIGHT(zh + 1, xh    );

						uv[0] = (xMax - x    ) * xts; uv[1] = (zMax - z    ) * yts; // uv = (1, 1)
						uv[2] = (xMax - x - 1) * xts; uv[3] = (zMax - z    ) * yts; // uv = (0, 1)
						uv[4] = (xMax - x - 1) * xts; uv[5] = (zMax - z - 1) * yts; // uv = (0, 0)
						uv[6] = (xMax - x    ) * xts; uv[7] = (zMax - z - 1) * yts; // uv = (1, 0)

						AddQuadVertices(decal->va, xh, yv, zh, uv, color);
					}
				}
			} break;

			case 3: { // West
				if (tlx <    0) { zMax += tlx       ; tlx =    0; blx = tlx; }
				if (trx > gsmx) { zMin += trx - gsmx; trx = gsmx; brx = trx; }
				if (tlz <    0) { xMin -= tlz       ; tlz =    0; trz = tlz; }
				if (brz > gsmy) { xMax -= brz - gsmy; brz = gsmy; blz = brz; }

				for (int x = xMin; x < xMax; x++) {
					const int xh = tlx + x;

					for (int z = zMin; z < zMax; z++) {
						const int zh = tlz + z;

						yv[0] = HEIGHT(zh,     xh    ); yv[1] = HEIGHT(zh,     xh + 1);
						yv[2] = HEIGHT(zh + 1, xh + 1); yv[3] = HEIGHT(zh + 1, xh    );

						uv[0] = (z    ) * yts; uv[1] = 1.0f - (x    ) * xts; // uv = (0, 1)
						uv[2] = (z    ) * yts; uv[3] = 1.0f - (x + 1) * xts; // uv = (0, 0)
						uv[4] = (z + 1) * yts; uv[5] = 1.0f - (x + 1) * xts; // uv = (1, 0)
						uv[6] = (z + 1) * yts; uv[7] = 1.0f - (x    ) * xts; // uv = (1, 1)

						AddQuadVertices(decal->va, xh, yv, zh, uv, color);
					}
				}
			} break;
		}
	} else {
		const float c = *((float*) (color));
		const int start = 0;
		const int stride = 6;
		const int sdi = decal->va->drawIndex();

		for (int i = start; i < sdi; i += stride) {
			const int x = int(decal->va->drawArray[i + 0]) >> 3;
			const int z = int(decal->va->drawArray[i + 2]) >> 3;
			const float h = hm[z * gsmx1 + x];

			// update the height and alpha
			decal->va->drawArray[i + 1] = h;
			decal->va->drawArray[i + 5] = c;
		}

		decal->va->DrawArrayTC(GL_QUADS);
	}

	#undef HEIGHT
}

static inline void DrawGroundScar(CGroundDecalHandler::Scar* scar, bool fade)
{
	const float* hm = readmap->GetHeightmap();
	const int gsmx = gs->mapx;
	const int gsmx1 = gsmx + 1;

	unsigned char color[4] = {255, 255, 255, 255};

	if (!scar->va) {
		scar->va = SAFE_NEW CVertexArray();
		scar->va->Initialize();

		float3 pos = scar->pos;
		const float radius = scar->radius;
		const float radius4 = radius * 4.0f;
		const float tx = scar->texOffsetX;
		const float ty = scar->texOffsetY;

		int sx = (int) max(0.0f,                 (pos.x - radius) * 0.0625f);
		int ex = (int) min(float(gs->hmapx - 1), (pos.x + radius) * 0.0625f);
		int sz = (int) max(0.0f,                 (pos.z - radius) * 0.0625f);
		int ez = (int) min(float(gs->hmapy - 1), (pos.z + radius) * 0.0625f);

		// create the scar texture-quads
		float px1 = sx * 16;
		for (int x = sx; x <= ex; ++x) {
			//const float* hm2 = hm;
			float px2 = px1 + 16;
			float pz1 = sz * 16;

			for (int z = sz; z <= ez; ++z) {
				float pz2 = pz1 + 16;
				float tx1 = min(0.5f, (pos.x - px1) / radius4 + 0.25f);
				float tx2 = max(0.0f, (pos.x - px2) / radius4 + 0.25f);
				float tz1 = min(0.5f, (pos.z - pz1) / radius4 + 0.25f);
				float tz2 = max(0.0f, (pos.z - pz2) / radius4 + 0.25f);
				float h1 = ground->GetHeight2(px1, pz1);
				float h2 = ground->GetHeight2(px2, pz1);
				float h3 = ground->GetHeight2(px2, pz2);
				float h4 = ground->GetHeight2(px1, pz2);

				scar->va->AddVertexTC(float3(px1, h1, pz1), tx1 + tx, tz1 + ty, color);
				scar->va->AddVertexTC(float3(px2, h2, pz1), tx2 + tx, tz1 + ty, color);
				scar->va->AddVertexTC(float3(px2, h3, pz2), tx2 + tx, tz2 + ty, color);
				scar->va->AddVertexTC(float3(px1, h4, pz2), tx1 + tx, tz2 + ty, color);
				//hm2 += gsmx12;
				pz1 = pz2;
			}

			//hm2 += 2;
			px1 = px2;
		}
	} else {
		if (fade) {
			if (scar->creationTime + 10 > gs->frameNum) {
				color[3] = (int) (scar->startAlpha * (gs->frameNum - scar->creationTime) * 0.1f);
			} else {
				color[3] = (int) (scar->startAlpha - (gs->frameNum - scar->creationTime) * scar->alphaFalloff);
			}

			const float c = *((float*) (color));
			const int start = 0;
			const int stride = 6;
			const int sdi = scar->va->drawIndex();

			for (int i = start; i < sdi; i += stride) {
				const int x = int(scar->va->drawArray[i + 0]) >> 3;
				const int z = int(scar->va->drawArray[i + 2]) >> 3;
				const float h = hm[z * gsmx1 + x];

				// update the height and alpha
				scar->va->drawArray[i + 1] = h;
				scar->va->drawArray[i + 5] = c;
			}
		}

		scar->va->DrawArrayTC(GL_QUADS);
	}
}




void CGroundDecalHandler::Draw(void)
{
	if (!drawDecals) {
		return;
	}

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.01f);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-10, -200);
	glDepthMask(0);

	glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, readmap->GetShadingTexture());
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	glActiveTextureARB(GL_TEXTURE0_ARB);


	if (shadowHandler && shadowHandler->drawShadows) {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, decalFP);
		glEnable(GL_FRAGMENT_PROGRAM_ARB );
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, decalVP);
		glEnable(GL_VERTEX_PROGRAM_ARB );
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 10, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 1);
		float3 ac = mapInfo->light.groundAmbientColor * (210.0f / 255.0f);
		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 10, ac.x, ac.y, ac.z, 1);
		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 11, 0, 0, 0, mapInfo->light.groundShadowDensity);
		glMatrixMode(GL_MATRIX0_ARB);
		glLoadMatrixf(shadowHandler->shadowMatrix.m);
		glMatrixMode(GL_MODELVIEW);
	}

	GML_STDMUTEX_LOCK(decal); // Draw

	// create and draw the quads for each building decal
	for (std::vector<BuildingDecalType*>::iterator bdti = buildingDecalTypes.begin(); bdti != buildingDecalTypes.end(); ++bdti) {
		BuildingDecalType* bdt = *bdti;

		if (!bdt->buildingDecals.empty()) {
			glBindTexture(GL_TEXTURE_2D, bdt->texture);

			set<BuildingGroundDecal*>::iterator bgdi = bdt->buildingDecals.begin();

			while (bgdi != bdt->buildingDecals.end()) {
				BuildingGroundDecal* decal = *bgdi;

				if (decal->owner && decal->owner->buildProgress >= 0) {
					decal->alpha = decal->owner->buildProgress;
				} else if (!decal->gbOwner) {
					decal->alpha -= decal->AlphaFalloff * gu->lastFrameTime * gs->speedFactor;
				}

				if (decal->alpha < 0.0f) {
					// make sure RemoveBuilding() won't try to modify this decal
					if (decal->owner) {
						decal->owner->buildingDecal = 0;
					}

					delete decal->va;
					decal->va = 0x0;
					delete decal;

					set<BuildingGroundDecal*>::iterator next(bgdi); ++next;
					bdt->buildingDecals.erase(bgdi);
					bgdi = next;

					continue;
				}

				if (camera->InView(decal->pos, decal->radius) &&
					(!decal->owner || (decal->owner->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_PREVLOS)) || gu->spectatingFullView)) {

					DrawBuildingDecal(decal);
				}

				bgdi++;
			}

		}
	}

	if (shadowHandler && shadowHandler->drawShadows) {
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
		glDisable(GL_VERTEX_PROGRAM_ARB);
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
		glDisable(GL_TEXTURE_2D);

		glActiveTextureARB(GL_TEXTURE1_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);

		glActiveTextureARB(GL_TEXTURE0_ARB);
	}



	glPolygonOffset(-10, -20);

	unsigned char color[4] = {255, 255, 255, 255};
	unsigned char color2[4] = {255, 255, 255, 255};

	// create and draw the unit footprint quads
	for (std::vector<TrackType*>::iterator tti = trackTypes.begin(); tti != trackTypes.end(); ++tti) {
		if (!(*tti)->tracks.empty()) {
			TrackType* tt = *tti;
			set<UnitTrackStruct*>::iterator utsi = tt->tracks.begin();

			CVertexArray* va = GetVertexArray();
			va->Initialize();
			glBindTexture(GL_TEXTURE_2D, tt->texture);


			while (utsi != tt->tracks.end()) {
				UnitTrackStruct* track = *utsi;

				while (!track->parts.empty() && track->parts.front().creationTime < gs->frameNum - track->lifeTime) {
					track->parts.pop_front();
				}

				if (track->parts.empty()) {
					if (track->owner)
						track->owner->myTrack = 0;

					delete track;

					set<UnitTrackStruct*>::iterator next(utsi); ++next;
					tt->tracks.erase(utsi);
					utsi = next;

					continue;
				}

				if (camera->InView((track->parts.front().pos1 + track->parts.back().pos1) * 0.5f, track->parts.front().pos1.distance(track->parts.back().pos1) + 500)) {
					list<TrackPart>::iterator ppi = track->parts.begin();
					color2[3] = track->trackAlpha - (int) ((gs->frameNum - ppi->creationTime) * track->alphaFalloff);

					va->EnlargeArrays(track->parts.size()*4,0,VA_SIZE_TC);
					for (list<TrackPart>::iterator pi = ++track->parts.begin(); pi != track->parts.end(); ++pi) {
						color[3] = track->trackAlpha - (int) ((gs->frameNum - ppi->creationTime) * track->alphaFalloff);
						if (pi->connected) {
							va->AddVertexQTC(ppi->pos1, ppi->texPos, 0, color2);
							va->AddVertexQTC(ppi->pos2, ppi->texPos, 1, color2);
							va->AddVertexQTC(pi->pos2, pi->texPos, 1, color);
							va->AddVertexQTC(pi->pos1, pi->texPos, 0, color);
						}
						color2[3] = color[3];
						ppi = pi;
					}
				}

				utsi++;
			}

			va->DrawArrayTC(GL_QUADS);
		}
	}


	glBindTexture(GL_TEXTURE_2D, scarTex);
	glPolygonOffset(-10, -400);

	// create and draw the 16x16 quads for each ground scar
	for (std::list<Scar*>::iterator si = scars.begin(); si != scars.end(); ) {
		Scar* scar = *si;

		if (scar->lifeTime < gs->frameNum) {
			RemoveScar(*si, false);
			si = scars.erase(si);
			continue;
		}

		if (camera->InView(scar->pos, scar->radius + 16)) {
			DrawGroundScar(scar, groundScarAlphaFade);
		}

		++si;
	}



	glDisable(GL_POLYGON_OFFSET_FILL);
	glDepthMask(1);
	glDisable(GL_BLEND);

	glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glActiveTextureARB(GL_TEXTURE0_ARB);
}


void CGroundDecalHandler::Update(void)
{
	GML_STDMUTEX_LOCK(decal); // Update
	for(std::vector<CUnit *>::iterator i=moveUnits.begin(); i!=moveUnits.end(); ++i)
		UnitMovedNow(*i);
	moveUnits.clear();
}


void CGroundDecalHandler::UnitMoved(CUnit* unit)
{
	moveUnits.push_back(unit);
}


void CGroundDecalHandler::UnitMovedNow(CUnit* unit)
{
	if(decalLevel==0)
		return;

	int zp=(int(unit->pos.z)/SQUARE_SIZE*2);
	int xp=(int(unit->pos.x)/SQUARE_SIZE*2);
	int mp=zp*gs->hmapx+xp;
	if(mp<0)
		mp=0;
	if(mp>=gs->mapSquares/4)
		mp=gs->mapSquares/4-1;
	if(!mapInfo->terrainTypes[readmap->typemap[mp]].receiveTracks)
		return;

	TrackType* type=trackTypes[unit->unitDef->trackType];
	if(!unit->myTrack){
		UnitTrackStruct* ts=SAFE_NEW UnitTrackStruct;
		ts->owner=unit;
		ts->lifeTime=(int)(30*decalLevel*unit->unitDef->trackStrength);
		ts->trackAlpha=(int)(unit->unitDef->trackStrength*25);
		ts->alphaFalloff=float(ts->trackAlpha)/float(ts->lifeTime);
		type->tracks.insert(ts);
		unit->myTrack=ts;
	}
	float3 pos=unit->pos+unit->frontdir*unit->unitDef->trackOffset;

	TrackPart tp;
	tp.pos1=pos+unit->rightdir*unit->unitDef->trackWidth*0.5f;
	tp.pos1.y=ground->GetHeight2(tp.pos1.x,tp.pos1.z);
	tp.pos2=pos-unit->rightdir*unit->unitDef->trackWidth*0.5f;
	tp.pos2.y=ground->GetHeight2(tp.pos2.x,tp.pos2.z);
	tp.creationTime=gs->frameNum;

	if(unit->myTrack->parts.empty()){
		tp.texPos=0;
		tp.connected=false;
	} else {
		tp.texPos=unit->myTrack->parts.back().texPos+tp.pos1.distance(unit->myTrack->parts.back().pos1)/unit->unitDef->trackWidth*unit->unitDef->trackStretch;
		tp.connected=unit->myTrack->parts.back().creationTime==gs->frameNum-8;
	}

	//if the unit is moving in a straight line only place marks at half the rate by replacing old ones
	if(unit->myTrack->parts.size()>1){
		list<TrackPart>::iterator pi=unit->myTrack->parts.end();
		--pi;
		list<TrackPart>::iterator pi2=pi;
		--pi;
		if(((tp.pos1+pi->pos1)*0.5f).distance(pi2->pos1)<1){
			unit->myTrack->parts.back()=tp;
			return;
		}
	}

	unit->myTrack->parts.push_back(tp);
}


void CGroundDecalHandler::RemoveUnit(CUnit* unit)
{
	if(decalLevel==0)
		return;

	if(unit->myTrack){
		unit->myTrack->owner=0;
		unit->myTrack=0;
	}
}


int CGroundDecalHandler::GetTrackType(const std::string& name)
{

	if (decalLevel == 0) {
		return 0;
	}

	const std::string lowerName = StringToLower(name);

	int a = 0;
	std::vector<TrackType*>::iterator ti;
	for(ti = trackTypes.begin(); ti != trackTypes.end(); ++ti) {
		if ((*ti)->name == lowerName) {
			return a;
		}
		++a;
	}

	GML_STDMUTEX_LOCK(decal); // GetTrackType

	TrackType* tt = SAFE_NEW TrackType;
	tt->name = lowerName;
	tt->texture = LoadTexture(lowerName);
	trackTypes.push_back(tt);

	return trackTypes.size() - 1;
}


unsigned int CGroundDecalHandler::LoadTexture(const std::string& name)
{
	std::string fullName = name;
	if (fullName.find_first_of('.') == string::npos) {
		fullName += ".bmp";
	}
	if ((fullName.find_first_of('\\') == string::npos) &&
	    (fullName.find_first_of('/')  == string::npos)) {
		fullName = string("bitmaps/tracks/") + fullName;
	}

	CBitmap bm;
	if (!bm.Load(fullName)) {
		throw content_error("Could not load ground decal from file " + fullName);
	}
	for (int y = 0; y < bm.ysize; ++y) {
		for (int x = 0; x < bm.xsize; ++x) {
			const int index = ((y * bm.xsize) + x) * 4;
			bm.mem[index + 3]    = bm.mem[index + 1];
			const int brightness = bm.mem[index + 0];
			bm.mem[index + 0] = (brightness * 90) / 255;
			bm.mem[index + 1] = (brightness * 60) / 255;
			bm.mem[index + 2] = (brightness * 30) / 255;
		}
	}

	return bm.CreateTexture(true);
}


void CGroundDecalHandler::AddExplosion(float3 pos, float damage, float radius)
{
	if (decalLevel == 0)
		return;

	GML_STDMUTEX_LOCK(decal); // AddExplosion

	float height = pos.y - ground->GetHeight2(pos.x, pos.z);
	if (height >= radius)
		return;

	pos.y -= height;
	radius -= height;

	if (radius < 5)
		return;

	if (damage > radius * 30)
		damage = radius * 30;

	damage *= (radius) / (radius + height);
	if (radius > damage * 0.25f)
		radius = damage * 0.25f;

	if (damage > 400)
		damage = 400 + sqrt(damage - 399);

	pos.CheckInBounds();

	Scar* s = SAFE_NEW Scar;
	s->pos = pos;
	s->radius = radius * 1.4f;
	s->creationTime = gs->frameNum;
	s->startAlpha = max(50.f, min(255.f, damage));
	float lifeTime = decalLevel * (damage) * 3;
	s->alphaFalloff = s->startAlpha / (lifeTime);
	s->lifeTime = (int) (gs->frameNum + lifeTime);
	s->texOffsetX = (gu->usRandInt() & 128)? 0: 0.5f;
	s->texOffsetY = (gu->usRandInt() & 128)? 0: 0.5f;

	s->x1 = (int) max(0.f,                  (pos.x - radius) / 16.0f    );
	s->x2 = (int) min(float(gs->hmapx - 1), (pos.x + radius) / 16.0f + 1);
	s->y1 = (int) max(0.f,                  (pos.z - radius) / 16.0f    );
	s->y2 = (int) min(float(gs->hmapy - 1), (pos.z + radius) / 16.0f + 1);

	s->basesize = (s->x2 - s->x1) * (s->y2 - s->y1);
	s->overdrawn = 0;
	s->lastTest = 0;

	TestOverlaps(s);

	int x1 = s->x1 / 16;
	int x2 = min(scarFieldX - 1, s->x2 / 16);
	int y1 = s->y1 / 16;
	int y2 = min(scarFieldY - 1, s->y2 / 16);

	for (int y = y1; y <= y2; ++y) {
		for (int x = x1; x <= x2; ++x) {
			std::set<Scar*>* quad = &scarField[y * scarFieldX + x];
			quad->insert(s);
		}
	}

	scars.push_back(s);
}


void CGroundDecalHandler::LoadScar(const std::string& file, unsigned char* buf,
                                   int xoffset, int yoffset)
{
	CBitmap bm;
	if (!bm.Load(file)) {
		throw content_error("Could not load scar from file " + file);
	}
	for (int y = 0; y < bm.ysize; ++y) {
		for (int x = 0; x < bm.xsize; ++x) {
			const int memIndex = ((y * bm.xsize) + x) * 4;
			const int bufIndex = (((y + yoffset) * 512) + x + xoffset) * 4;
			buf[bufIndex + 3]    = bm.mem[memIndex + 1];
			const int brightness = bm.mem[memIndex + 0];
			buf[bufIndex + 0] = (brightness * 90) / 255;
			buf[bufIndex + 1] = (brightness * 60) / 255;
			buf[bufIndex + 2] = (brightness * 30) / 255;
		}
	}
}


int CGroundDecalHandler::OverlapSize(Scar* s1, Scar* s2)
{
	if(s1->x1>=s2->x2 || s1->x2<=s2->x1)
		return 0;
	if(s1->y1>=s2->y2 || s1->y2<=s2->y1)
		return 0;

	int xs;
	if(s1->x1<s2->x1)
		xs=s1->x2-s2->x1;
	else
		xs=s2->x2-s1->x1;

	int ys;
	if(s1->y1<s2->y1)
		ys=s1->y2-s2->y1;
	else
		ys=s2->y2-s1->y1;

	return xs*ys;
}


void CGroundDecalHandler::TestOverlaps(Scar* scar)
{
	int x1=scar->x1/16;
	int x2=min(scarFieldX-1,scar->x2/16);
	int y1=scar->y1/16;
	int y2=min(scarFieldY-1,scar->y2/16);

	++lastTest;

	for(int y=y1;y<=y2;++y){
		for(int x=x1;x<=x2;++x){
			std::set<Scar*>* quad=&scarField[y*scarFieldX+x];
			bool redoScan=false;
			do {
				redoScan=false;
				for(std::set<Scar*>::iterator si=quad->begin();si!=quad->end();++si){
					if(lastTest!=(*si)->lastTest && scar->lifeTime>=(*si)->lifeTime){
						Scar* tested=*si;
						tested->lastTest=lastTest;
						int overlap=OverlapSize(scar,tested);
						if(overlap>0 && tested->basesize>0){
							float part=overlap/tested->basesize;
							tested->overdrawn+=part;
							if(tested->overdrawn>maxOverlap){
								RemoveScar(tested,true);
								redoScan=true;
								break;
							}
						}
					}
				}
			} while(redoScan);
		}
	}
}


void CGroundDecalHandler::RemoveScar(Scar* scar, bool removeFromScars)
{
	int x1 = scar->x1 / 16;
	int x2 = min(scarFieldX - 1, scar->x2 / 16);
	int y1 = scar->y1 / 16;
	int y2 = min(scarFieldY - 1, scar->y2 / 16);

	for (int y = y1;y <= y2; ++y) {
		for (int x = x1; x <= x2; ++x) {
			std::set<Scar*>* quad = &scarField[y * scarFieldX + x];
			quad->erase(scar);
		}
	}

	if (removeFromScars)
		scars.remove(scar);

	delete scar;
}


void CGroundDecalHandler::AddBuilding(CBuilding* building)
{
	if (decalLevel == 0)
		return;

	GML_STDMUTEX_LOCK(decal); // AddBuilding

	BuildingDecalType* type = buildingDecalTypes[building->unitDef->buildingDecalType];
	BuildingGroundDecal* decal = SAFE_NEW BuildingGroundDecal;

	int posx = int(building->pos.x / 8);
	int posy = int(building->pos.z / 8);
	int sizex = building->unitDef->buildingDecalSizeX;
	int sizey = building->unitDef->buildingDecalSizeY;

	decal->owner = building;
	decal->gbOwner = 0;
	decal->AlphaFalloff = building->unitDef->buildingDecalDecaySpeed;
	decal->alpha = 0;
	decal->pos = building->pos;
	decal->radius = sqrt((float) (sizex * sizex + sizey * sizey)) * 8 + 20;
	decal->facing = building->buildFacing;

	decal->xsize = sizex * 2;
	decal->ysize = sizey * 2;

	if (building->buildFacing == 1 || building->buildFacing == 3) {
		// swap xsize and ysize if building faces East or West
		std::swap(decal->xsize, decal->ysize);
	}

	decal->posx = posx - (decal->xsize / 2);
	decal->posy = posy - (decal->ysize / 2);

	building->buildingDecal = decal;
	type->buildingDecals.insert(decal);
}


void CGroundDecalHandler::RemoveBuilding(CBuilding* building, CUnitDrawer::GhostBuilding* gb)
{
	GML_STDMUTEX_LOCK(decal); // RemoveBuilding

	BuildingGroundDecal* decal = building->buildingDecal;
	if (!decal)
		return;

	decal->owner = 0;
	decal->gbOwner = gb;
	building->buildingDecal = 0;
}


int CGroundDecalHandler::GetBuildingDecalType(const std::string& name)
{
	if (decalLevel == 0) {
		return 0;
	}

	const std::string lowerName = StringToLower(name);

	int a = 0;
	std::vector<BuildingDecalType*>::iterator bi;
	for (bi = buildingDecalTypes.begin(); bi != buildingDecalTypes.end(); ++bi) {
		if ((*bi)->name == lowerName) {
			return a;
		}
		++a;
	}

	GML_STDMUTEX_LOCK(decal); // GetBuildingDecalType

	BuildingDecalType* tt = SAFE_NEW BuildingDecalType;
	tt->name = lowerName;
	const std::string fullName = "unittextures/" + lowerName;
	CBitmap bm;
	if (!bm.Load(fullName)) {
		throw content_error("Could not load building decal from file " + fullName);
	}

	tt->texture = bm.CreateTexture(true);
	buildingDecalTypes.push_back(tt);

	return (buildingDecalTypes.size() - 1);
}


void CGroundDecalHandler::SetTexGen(float scalex,float scaley, float offsetx, float offsety)
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
