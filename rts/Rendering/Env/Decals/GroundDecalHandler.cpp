/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <cctype>

#include "GroundDecalHandler.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Lua/LuaParser.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "Sim/Projectiles/ExplosionListener.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/Util.h"
#include "System/TimeProfiler.h"
#include "System/FileSystem/FileSystem.h"

using std::list;
using std::min;
using std::max;

CGroundDecalHandler* groundDecals_ = NULL;

CONFIG(int, GroundScarAlphaFade).defaultValue(0);

CGroundDecalHandler::CGroundDecalHandler()
	: CEventClient("[CGroundDecalHandler]", 314159, false)
{
	scarField = NULL;
	if (!GetDrawDecals())
		return;

	eventHandler.AddClient(this);
	CExplosionCreator::AddExplosionListener(this);

	groundScarAlphaFade = (configHandler->GetInt("GroundScarAlphaFade") != 0);

	unsigned char* buf=new unsigned char[512*512*4];
	memset(buf,0,512*512*4);

	LuaParser resourcesParser("gamedata/resources.lua",
	                          SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!resourcesParser.Execute()) {
		LOG_L(L_ERROR, "Failed to load resources: %s",
				resourcesParser.GetErrorLog().c_str());
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
	glBuildMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,512, 512, GL_RGBA, GL_UNSIGNED_BYTE, buf);

	scarFieldX=gs->mapx/32;
	scarFieldY=gs->mapy/32;
	scarField=new std::set<Scar*>[scarFieldX*scarFieldY];

	lastTest=0;
	maxOverlap=decalLevel+1;

	delete[] buf;

	LoadDecalShaders();
}



CGroundDecalHandler::~CGroundDecalHandler()
{
	eventHandler.RemoveClient(this);

	for (std::vector<TrackType*>::iterator tti = trackTypes.begin(); tti != trackTypes.end(); ++tti) {
		for (set<UnitTrackStruct*>::iterator ti = (*tti)->tracks.begin(); ti != (*tti)->tracks.end(); ++ti) {
			delete *ti;
		}
		glDeleteTextures(1, &(*tti)->texture);
		delete *tti;
	}
	for (std::vector<TrackToAdd>::iterator ti = tracksToBeAdded.begin(); ti != tracksToBeAdded.end(); ++ti) {
		delete (*ti).tp;
		if ((*ti).unit == NULL)
			tracksToBeDeleted.push_back((*ti).ts);
	}
	for (std::vector<UnitTrackStruct *>::iterator ti = tracksToBeDeleted.begin(); ti != tracksToBeDeleted.end(); ++ti) {
		delete *ti;
	}

	for (std::vector<SolidObjectDecalType*>::iterator tti = objectDecalTypes.begin(); tti != objectDecalTypes.end(); ++tti) {
		for (set<SolidObjectGroundDecal*>::iterator ti = (*tti)->objectDecals.begin(); ti != (*tti)->objectDecals.end(); ++ti) {
			if ((*ti)->owner)
				(*ti)->owner->groundDecal = 0;
			if ((*ti)->gbOwner)
				(*ti)->gbOwner->decal = 0;
			delete *ti;
		}
		glDeleteTextures (1, &(*tti)->texture);
		delete *tti;
	}
	for (std::list<Scar*>::iterator si = scars.begin(); si != scars.end(); ++si) {
		delete *si;
	}
	for (std::vector<Scar*>::iterator si = scarsToBeAdded.begin(); si != scarsToBeAdded.end(); ++si) {
		delete *si;
	}
	if (scarField != NULL) {
		delete[] scarField;

		glDeleteTextures(1, &scarTex);
	}

	shaderHandler->ReleaseProgramObjects("[GroundDecalHandler]");
	decalShaders.clear();
}

void CGroundDecalHandler::LoadDecalShaders() {
	#define sh shaderHandler
	decalShaders.resize(DECAL_SHADER_LAST, NULL);

	// SM3 maps have no baked lighting, so decals blend differently
	const bool haveShadingTexture = (readmap->GetShadingTexture() != 0);
	const char* fragmentProgramNameARB = haveShadingTexture?
		"ARB/GroundDecalsSMF.fp":
		"ARB/GroundDecalsSM3.fp";
	const std::string extraDef = haveShadingTexture?
		"#define HAVE_SHADING_TEX 1\n":
		"#define HAVE_SHADING_TEX 0\n";

	decalShaders[DECAL_SHADER_ARB ] = sh->CreateProgramObject("[GroundDecalHandler]", "DecalShaderARB",  true);
	decalShaders[DECAL_SHADER_GLSL] = sh->CreateProgramObject("[GroundDecalHandler]", "DecalShaderGLSL", false);
	decalShaders[DECAL_SHADER_CURR] = decalShaders[DECAL_SHADER_ARB];

	if (globalRendering->haveARB && !globalRendering->haveGLSL) {
		decalShaders[DECAL_SHADER_ARB]->AttachShaderObject(sh->CreateShaderObject("ARB/GroundDecals.vp", "", GL_VERTEX_PROGRAM_ARB));
		decalShaders[DECAL_SHADER_ARB]->AttachShaderObject(sh->CreateShaderObject(fragmentProgramNameARB, "", GL_FRAGMENT_PROGRAM_ARB));
		decalShaders[DECAL_SHADER_ARB]->Link();
	} else {
		if (globalRendering->haveGLSL) {
			decalShaders[DECAL_SHADER_GLSL]->AttachShaderObject(sh->CreateShaderObject("GLSL/GroundDecalsVertProg.glsl", "",       GL_VERTEX_SHADER));
			decalShaders[DECAL_SHADER_GLSL]->AttachShaderObject(sh->CreateShaderObject("GLSL/GroundDecalsFragProg.glsl", extraDef, GL_FRAGMENT_SHADER));
			decalShaders[DECAL_SHADER_GLSL]->Link();

			decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("decalTex");           // idx 0
			decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("shadeTex");           // idx 1
			decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("shadowTex");          // idx 2
			decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("mapSizePO2");         // idx 3
			decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("groundAmbientColor"); // idx 4
			decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("shadowMatrix");       // idx 5
			decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("shadowParams");       // idx 6
			decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("shadowDensity");      // idx 7

			decalShaders[DECAL_SHADER_GLSL]->Enable();
			decalShaders[DECAL_SHADER_GLSL]->SetUniform1i(0, 0); // decalTex  (idx 0, texunit 0)
			decalShaders[DECAL_SHADER_GLSL]->SetUniform1i(1, 1); // shadeTex  (idx 1, texunit 1)
			decalShaders[DECAL_SHADER_GLSL]->SetUniform1i(2, 2); // shadowTex (idx 2, texunit 2)
			decalShaders[DECAL_SHADER_GLSL]->SetUniform2f(3, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE));
			decalShaders[DECAL_SHADER_GLSL]->SetUniform1f(7, sky->GetLight()->GetGroundShadowDensity());
			decalShaders[DECAL_SHADER_GLSL]->Disable();
			decalShaders[DECAL_SHADER_GLSL]->Validate();

			decalShaders[DECAL_SHADER_CURR] = decalShaders[DECAL_SHADER_GLSL];
		}
	}

	#undef sh
}

void CGroundDecalHandler::SunChanged(const float3& sunDir) {
	if (globalRendering->haveGLSL && decalShaders.size() > DECAL_SHADER_GLSL) {
		decalShaders[DECAL_SHADER_GLSL]->Enable();
		decalShaders[DECAL_SHADER_GLSL]->SetUniform1f(7, sky->GetLight()->GetGroundShadowDensity());
		decalShaders[DECAL_SHADER_GLSL]->Disable();
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


inline void CGroundDecalHandler::DrawObjectDecal(SolidObjectGroundDecal* decal)
{
	if (!camera->InView(decal->pos, decal->radius)) {
		return;
	}

	const float* hm = readmap->GetCornerHeightMapUnsynced();
	const int gsmx = gs->mapx;
	const int gsmx1 = gsmx + 1;
	const int gsmy = gs->mapy;

	unsigned char color[4] = {255, 255, 255, (unsigned char)(decal->alpha * 255)};

	#ifndef DEBUG
	#define HEIGHT(z, x) (hm[((z) * gsmx1) + (x)])
	#else
	#define HEIGHT(z, x) (assert((z) <= gsmy), assert((x) <= gsmx), (hm[((z) * gsmx1) + (x)]))
	#endif

	if (!decal->va) {
		// NOTE: this really needs CLOD'ing
		decal->va = new CVertexArray();
		decal->va->Initialize();

		const int
			dxsize = decal->xsize,
			dzsize = decal->ysize,
			dxpos  = decal->posx,              // top-left quad x-coordinate
			dzpos  = decal->posy,              // top-left quad z-coordinate
			dxoff  = (dxpos < 0)? -(dxpos): 0, // offset from left map edge
			dzoff  = (dzpos < 0)? -(dzpos): 0; // offset from top map edge

		const float xts = 1.0f / dxsize;
		const float zts = 1.0f / dzsize;

		float yv[4] = {0.0f}; // heights at each sub-quad vertex (tl, tr, br, bl)
		float uv[8] = {0.0f}; // tex-coors at each sub-quad vertex

		// clipped decal dimensions
		int cxsize = dxsize - dxoff;
		int czsize = dzsize - dzoff;

		if ((dxpos + dxsize) > gsmx) { cxsize -= ((dxpos + dxsize) - gsmx); }
		if ((dzpos + dzsize) > gsmy) { czsize -= ((dzpos + dzsize) - gsmy); }

		for (int vx = 0; vx < cxsize; vx++) {
			for (int vz = 0; vz < czsize; vz++) {
				const int rx = dxoff + vx;  // x-coor in decal-space
				const int rz = dzoff + vz;  // z-coor in decal-space
				const int px = dxpos + rx;  // x-coor in heightmap-space
				const int pz = dzpos + rz;  // z-coor in heightmap-space

				yv[0] = HEIGHT(pz,     px    ); yv[1] = HEIGHT(pz,     px + 1);
				yv[2] = HEIGHT(pz + 1, px + 1); yv[3] = HEIGHT(pz + 1, px    );

				switch (decal->facing) {
					case FACING_SOUTH: {
						uv[0] = (rx    ) * xts; uv[1] = (rz    ) * zts; // uv = (0, 0)
						uv[2] = (rx + 1) * xts; uv[3] = (rz    ) * zts; // uv = (1, 0)
						uv[4] = (rx + 1) * xts; uv[5] = (rz + 1) * zts; // uv = (1, 1)
						uv[6] = (rx    ) * xts; uv[7] = (rz + 1) * zts; // uv = (0, 1)
					} break;
					case FACING_NORTH: {
						uv[0] = (dxsize - rx    ) * xts; uv[1] = (dzsize - rz    ) * zts; // uv = (1, 1)
						uv[2] = (dxsize - rx - 1) * xts; uv[3] = (dzsize - rz    ) * zts; // uv = (0, 1)
						uv[4] = (dxsize - rx - 1) * xts; uv[5] = (dzsize - rz - 1) * zts; // uv = (0, 0)
						uv[6] = (dxsize - rx    ) * xts; uv[7] = (dzsize - rz - 1) * zts; // uv = (1, 0)
					} break;

					case FACING_EAST: {
						uv[0] = 1.0f - (rz    ) * zts; uv[1] = (rx    ) * xts; // uv = (1, 0)
						uv[2] = 1.0f - (rz    ) * zts; uv[3] = (rx + 1) * xts; // uv = (1, 1)
						uv[4] = 1.0f - (rz + 1) * zts; uv[5] = (rx + 1) * xts; // uv = (0, 1)
						uv[6] = 1.0f - (rz + 1) * zts; uv[7] = (rx    ) * xts; // uv = (0, 0)
					} break;
					case FACING_WEST: {
						uv[0] = (rz    ) * zts; uv[1] = 1.0f - (rx    ) * xts; // uv = (0, 1)
						uv[2] = (rz    ) * zts; uv[3] = 1.0f - (rx + 1) * xts; // uv = (0, 0)
						uv[4] = (rz + 1) * zts; uv[5] = 1.0f - (rx + 1) * xts; // uv = (1, 0)
						uv[6] = (rz + 1) * zts; uv[7] = 1.0f - (rx    ) * xts; // uv = (1, 1)
					} break;
				}

				AddQuadVertices(decal->va, px, yv, pz, uv, color);
			}
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


inline void CGroundDecalHandler::DrawGroundScar(CGroundDecalHandler::Scar* scar, bool fade)
{
	if (!camera->InView(scar->pos, scar->radius + 16)) {
		return;
	}

	const float* hm = readmap->GetCornerHeightMapUnsynced();

	const int gsmx = gs->mapx;
	const int gsmx1 = gsmx + 1;

	unsigned char color[4] = {255, 255, 255, 255};

	if (!scar->va) {
		scar->va = new CVertexArray();
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
				float h1 = ground->GetHeightReal(px1, pz1, false);
				float h2 = ground->GetHeightReal(px2, pz1, false);
				float h3 = ground->GetHeightReal(px2, pz2, false);
				float h4 = ground->GetHeightReal(px1, pz2, false);

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



void CGroundDecalHandler::DrawObjectDecals() {
	// create and draw the quads for each building decal
	for (std::vector<SolidObjectDecalType*>::iterator bdti = objectDecalTypes.begin(); bdti != objectDecalTypes.end(); ++bdti) {
		SolidObjectDecalType* bdt = *bdti;

		if (!bdt->objectDecals.empty()) {

			glBindTexture(GL_TEXTURE_2D, bdt->texture);

			decalsToDraw.clear();
			{
				GML_STDMUTEX_LOCK(decal); // Draw

				set<SolidObjectGroundDecal*>::iterator bgdi = bdt->objectDecals.begin();

				while (bgdi != bdt->objectDecals.end()) {
					SolidObjectGroundDecal* decal = *bgdi;
					CSolidObject* decalOwner = decal->owner;
					CUnit* decalOwnerUnit = dynamic_cast<CUnit*>(decalOwner);
					CFeature* decalOwnerFeature = (decalOwnerUnit != NULL) ? NULL : static_cast<CFeature *>(decalOwner); // can only be feature or unit

					if (decalOwnerUnit != NULL) {
						if (decalOwnerUnit->buildProgress >= 0.0f)
							decal->alpha = decalOwnerUnit->buildProgress;
					} else if (decalOwner == NULL && decal->gbOwner == NULL) {
						decal->alpha -= (decal->alphaFalloff * globalRendering->lastFrameTime * gs->speedFactor);
					}

					if (decal->alpha < 0.0f) {
						// make sure RemoveSolidObject() won't try to modify this decal
						if (decalOwner != NULL) {
							decalOwner->groundDecal = NULL;
						}

						delete decal;

						bgdi = set_erase(bdt->objectDecals, bgdi);

						continue;
					}

					if (decalOwner == NULL || 
						(decalOwnerUnit != NULL && (decalOwnerUnit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_PREVLOS))) || 
						(decalOwnerFeature != NULL && decalOwnerFeature->IsInLosForAllyTeam(gu->myAllyTeam)) || // FIXME: Support "Prev Los" stuff for features?
						gu->spectatingFullView) {
						decalsToDraw.push_back(decal);
					}

					++bgdi;
				}
			}

			for (std::vector<SolidObjectGroundDecal*>::iterator di = decalsToDraw.begin(); di != decalsToDraw.end(); ++di) {
				DrawObjectDecal(*di);
			}

			// glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
}



void CGroundDecalHandler::AddTracks() {
	{
		GML_STDMUTEX_LOCK(track); // Draw
		// Delayed addition of new tracks
		for (std::vector<TrackToAdd>::iterator ti = tracksToBeAdded.begin(); ti != tracksToBeAdded.end(); ++ti) {
			TrackToAdd *tta = &(*ti);
			if (tta->ts->owner == NULL) {
				delete tta->tp;
				if (tta->unit == NULL)
					tracksToBeDeleted.push_back(tta->ts);
				continue; // unit removed
			}

			CUnit* unit = tta->unit;
			if (unit == NULL) {
				unit = tta->ts->owner;
				trackTypes[unit->unitDef->decalDef.trackDecalType]->tracks.insert(tta->ts);
			}

			TrackPart* tp = tta->tp;

			// if the unit is moving in a straight line only place marks at half the rate by replacing old ones
			bool replace = false;

			if (unit->myTrack->parts.size() > 1) {
				list<TrackPart *>::iterator pi = --unit->myTrack->parts.end();
				list<TrackPart *>::iterator pi2 = pi--;

				if (((tp->pos1 + (*pi)->pos1) * 0.5f).SqDistance((*pi2)->pos1) < 1.0f)
					replace = true;
			}

			if (replace) {
				delete unit->myTrack->parts.back();
				unit->myTrack->parts.back() = tp;
			} else {
				unit->myTrack->parts.push_back(tp);
			}
		}
		tracksToBeAdded.clear();
	}

	for (std::vector<UnitTrackStruct *>::iterator ti = tracksToBeDeleted.begin(); ti != tracksToBeDeleted.end(); ++ti) {
		delete *ti;
	}

	tracksToBeDeleted.clear();
	tracksToBeCleaned.clear();
}

void CGroundDecalHandler::DrawTracks() {
	unsigned char color[4] = {255, 255, 255, 255};
	unsigned char color2[4] = {255, 255, 255, 255};

	// create and draw the unit footprint quads
	for (std::vector<TrackType*>::iterator tti = trackTypes.begin(); tti != trackTypes.end(); ++tti) {
		TrackType* tt = *tti;

		if (!tt->tracks.empty()) {
			set<UnitTrackStruct*>::iterator utsi = tt->tracks.begin();

			CVertexArray* va = GetVertexArray();
			va->Initialize();
			glBindTexture(GL_TEXTURE_2D, tt->texture);

			while (utsi != tt->tracks.end()) {
				UnitTrackStruct* track = *utsi;

				if (track->parts.empty()) {
					tracksToBeCleaned.push_back(TrackToClean(track, &(tt->tracks)));
					continue;
				}

				if (gs->frameNum > (track->parts.front()->creationTime + track->lifeTime)) {
					tracksToBeCleaned.push_back(TrackToClean(track, &(tt->tracks)));
				}

				if (camera->InView((track->parts.front()->pos1 + track->parts.back()->pos1) * 0.5f, track->parts.front()->pos1.distance(track->parts.back()->pos1) + 500)) {
					list<TrackPart *>::iterator ppi = track->parts.begin();
					color2[3] = std::max(0, track->trackAlpha - (int) ((gs->frameNum - (*ppi)->creationTime) * track->alphaFalloff));

					va->EnlargeArrays(track->parts.size()*4,0,VA_SIZE_TC);
					for (list<TrackPart *>::iterator pi = ++track->parts.begin(); pi != track->parts.end(); ++pi) {
						color[3] = std::max(0, track->trackAlpha - (int) ((gs->frameNum - (*ppi)->creationTime) * track->alphaFalloff));
						if ((*pi)->connected) {
							va->AddVertexQTC((*ppi)->pos1, (*ppi)->texPos, 0, color2);
							va->AddVertexQTC((*ppi)->pos2, (*ppi)->texPos, 1, color2);
							va->AddVertexQTC((*pi)->pos2, (*pi)->texPos, 1, color);
							va->AddVertexQTC((*pi)->pos1, (*pi)->texPos, 0, color);
						}
						color2[3] = color[3];
						ppi = pi;
					}
				}
				++utsi;
			}
			va->DrawArrayTC(GL_QUADS);
		}
	}
}

void CGroundDecalHandler::CleanTracks() {
	GML_STDMUTEX_LOCK(track); // Draw

	// Cleanup old tracks
	for (std::vector<TrackToClean>::iterator ti = tracksToBeCleaned.begin(); ti != tracksToBeCleaned.end(); ++ti) {
		TrackToClean *ttc = &(*ti);
		UnitTrackStruct *track = ttc->track;

		while (!track->parts.empty()) {
			// stop at the first part that is still too young for deletion
			if (gs->frameNum <= (track->parts.front()->creationTime + track->lifeTime))
				break;

			delete track->parts.front();
			track->parts.pop_front();
		}

		if (track->parts.empty()) {
			if (track->owner != NULL) {
				track->owner->myTrack = NULL;
				track->owner = NULL;
			}
			ttc->tracks->erase(track);
			tracksToBeDeleted.push_back(track);
		}
	}
}



void CGroundDecalHandler::AddScars() {
	scarsToBeChecked.clear();

	{
		GML_STDMUTEX_LOCK(scar); // Draw

		for (std::vector<Scar*>::iterator si = scarsToBeAdded.begin(); si != scarsToBeAdded.end(); ++si)
			scarsToBeChecked.push_back(*si);

		scarsToBeAdded.clear();
	}

	for (std::vector<Scar*>::iterator si = scarsToBeChecked.begin(); si != scarsToBeChecked.end(); ++si) {
		Scar* s = *si;
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
}

void CGroundDecalHandler::DrawScars() {
	// create and draw the 16x16 quads for each ground scar
	for (std::list<Scar*>::iterator si = scars.begin(); si != scars.end(); ) {
		Scar* scar = *si;

		if (scar->lifeTime < gs->frameNum) {
			RemoveScar(*si, false);
			si = scars.erase(si);
			continue;
		}

		DrawGroundScar(scar, groundScarAlphaFade);
		++si;
	}
}




void CGroundDecalHandler::Draw()
{
	if (!drawDecals) {
		return;
	}

	SCOPED_TIMER("GroundDecals");

	const float3 ambientColor = mapInfo->light.groundAmbientColor * CGlobalRendering::SMF_INTENSITY_MULT;
	const CBaseGroundDrawer* gd = readmap->GetGroundDrawer();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-10, -200);
	glDepthMask(0);

	glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, readmap->GetShadingTexture());
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

		glMultiTexCoord4f(GL_TEXTURE1_ARB, 1.0f,1.0f,1.0f,1.0f); // workaround a nvidia bug with TexGen
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);

	if (gd->DrawExtraTex()) {
		glActiveTexture(GL_TEXTURE3);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

		glMultiTexCoord4f(GL_TEXTURE3_ARB, 1.0f,1.0f,1.0f,1.0f); // workaround a nvidia bug with TexGen
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);

		glBindTexture(GL_TEXTURE_2D, gd->infoTex);
	}

	if (shadowHandler->shadowsLoaded) {
		glActiveTexture(GL_TEXTURE2);
			glEnable(GL_TEXTURE_2D);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);

		decalShaders[DECAL_SHADER_CURR]->Enable();

		if (decalShaders[DECAL_SHADER_CURR] == decalShaders[DECAL_SHADER_ARB]) {
			decalShaders[DECAL_SHADER_CURR]->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
			decalShaders[DECAL_SHADER_CURR]->SetUniform4f(10, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0.0f, 1.0f);
			decalShaders[DECAL_SHADER_CURR]->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
			decalShaders[DECAL_SHADER_CURR]->SetUniform4f(10, ambientColor.x, ambientColor.y, ambientColor.z, 1.0f);
			decalShaders[DECAL_SHADER_CURR]->SetUniform4f(11, 0.0f, 0.0f, 0.0f, sky->GetLight()->GetGroundShadowDensity());

			glMatrixMode(GL_MATRIX0_ARB);
			glLoadMatrixf(shadowHandler->shadowMatrix.m);
			glMatrixMode(GL_MODELVIEW);
		} else {
			decalShaders[DECAL_SHADER_CURR]->SetUniform4f(4, ambientColor.x, ambientColor.y, ambientColor.z, 1.0f);
			decalShaders[DECAL_SHADER_CURR]->SetUniformMatrix4fv(5, false, &shadowHandler->shadowMatrix.m[0]);
			decalShaders[DECAL_SHADER_CURR]->SetUniform4fv(6, &(shadowHandler->GetShadowParams().x));
		}
	}

	glActiveTexture(GL_TEXTURE0);
	DrawObjectDecals();


	if (shadowHandler->shadowsLoaded) {
		decalShaders[DECAL_SHADER_CURR]->Disable();

		glActiveTexture(GL_TEXTURE2);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
			glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE1);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);

		glActiveTexture(GL_TEXTURE0);
	}



	glPolygonOffset(-10, -20);

	AddTracks();
	DrawTracks();
	CleanTracks();

	glBindTexture(GL_TEXTURE_2D, scarTex);
	glPolygonOffset(-10, -400);

	AddScars();
	DrawScars();

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_BLEND);

	glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glActiveTexture(GL_TEXTURE3); //! infotex
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glActiveTexture(GL_TEXTURE0);
}


void CGroundDecalHandler::RenderUnitMoved(const CUnit* unit, const float3& newpos)
{
	if (decalLevel == 0)
		return;

	AddDecalAndTrack(const_cast<CUnit*>(unit), newpos);
}


void CGroundDecalHandler::AddDecalAndTrack(CUnit* unit, const float3& newpos)
{
	SolidObjectDecalDef& decalDef = *const_cast<SolidObjectDecalDef*>(&unit->unitDef->decalDef);

	if (decalDef.useGroundDecal)
		MoveSolidObject(const_cast<CUnit *>(unit), newpos);

	if (!unit->leaveTracks)
		return;

	if (!unit->unitDef->IsGroundUnit())
		return;

	if (decalDef.trackDecalType < -1)
		return;

	if (decalDef.trackDecalType < 0) {
		decalDef.trackDecalType = GetTrackType(decalDef.trackDecalTypeName);
		if (decalDef.trackDecalType < -1)
			return;
	}

	if (unit->myTrack != NULL && unit->myTrack->lastUpdate >= (gs->frameNum - 7))
		return;

	if (!((unit->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectatingFullView))
		return;

	const int zp = (int(newpos.z) / SQUARE_SIZE * 2);
	const int xp = (int(newpos.x) / SQUARE_SIZE * 2);
	const int mp = Clamp(zp * gs->hmapx + xp, 0, (gs->mapSquares / 4) - 1);

	if (!mapInfo->terrainTypes[readmap->GetTypeMapSynced()[mp]].receiveTracks)
		return;

	const float trackLifeTime = GAME_SPEED * decalLevel * decalDef.trackDecalStrength;
	if (trackLifeTime <= 0)
		return;

	const float3 pos = newpos + unit->frontdir * decalDef.trackDecalOffset;

	TrackPart* tp = new TrackPart;
	tp->pos1 = pos + unit->rightdir * decalDef.trackDecalWidth * 0.5f;
	tp->pos2 = pos - unit->rightdir * decalDef.trackDecalWidth * 0.5f;
	tp->pos1.y = ground->GetHeightReal(tp->pos1.x, tp->pos1.z, false);
	tp->pos2.y = ground->GetHeightReal(tp->pos2.x, tp->pos2.z, false);
	tp->creationTime = gs->frameNum;

	TrackToAdd tta;
	tta.tp = tp;
	tta.unit = unit;

	GML_STDMUTEX_LOCK(track); // Update

	if (unit->myTrack == NULL) {
		unit->myTrack = new UnitTrackStruct(unit);
		unit->myTrack->lifeTime = trackLifeTime;
		unit->myTrack->trackAlpha = (decalDef.trackDecalStrength * 25);
		unit->myTrack->alphaFalloff = unit->myTrack->trackAlpha / trackLifeTime;

		tta.unit = NULL; // signal new trackstruct

		tp->texPos = 0;
		tp->connected = false;
	} else {
		const TrackPart* prevPart = unit->myTrack->lastAdded;

		tp->texPos = prevPart->texPos + (tp->pos1.distance(prevPart->pos1) / decalDef.trackDecalWidth) * decalDef.trackDecalStretch;
		tp->connected = (prevPart->creationTime == (gs->frameNum - 8));
	}

	unit->myTrack->lastUpdate = gs->frameNum;
	unit->myTrack->lastAdded = tp;

	tta.ts = unit->myTrack;
	tracksToBeAdded.push_back(tta);
}


int CGroundDecalHandler::GetTrackType(const std::string& name)
{
	if (decalLevel == 0) {
		return -2;
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

	TrackType* tt = new TrackType;
	tt->name = lowerName;
	tt->texture = LoadTexture(lowerName);

//	GML_STDMUTEX_LOCK(tracktype); // GetTrackType

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
	bool isBitmap = (FileSystem::GetExtension(fullName) == "bmp");

	CBitmap bm;
	if (!bm.Load(fullName)) {
		throw content_error("Could not load ground decal from file " + fullName);
	}
	if (isBitmap) {
		//! bitmaps don't have an alpha channel
		//! so use: red := brightness & green := alpha
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
	}

	return bm.CreateTexture(true);
}


void CGroundDecalHandler::AddExplosion(float3 pos, float damage, float radius, bool addScar)
{
	if (decalLevel == 0 || !addScar)
		return;

	const float lifeTime = decalLevel * damage * 3.0f;
	const float altitude = pos.y - ground->GetHeightReal(pos.x, pos.z, false);

	// no decals for below-ground explosions
	if (altitude <= -1.0f) { return; }
	if (altitude >= radius) { return; }

	pos.y -= altitude;
	radius -= altitude;

	if (radius < 5.0f)
		return;

	if (damage > radius * 30)
		damage = radius * 30;

	damage *= (radius) / (radius + altitude);
	if (radius > damage * 0.25f)
		radius = damage * 0.25f;

	if (damage > 400)
		damage = 400 + math::sqrt(damage - 399);

	pos.ClampInBounds();

	Scar* s = new Scar;
	s->pos = pos;
	s->radius = radius * 1.4f;
	s->creationTime = gs->frameNum;
	s->startAlpha = max(50.f, min(255.f, damage));
	s->alphaFalloff = s->startAlpha / (lifeTime);
	s->lifeTime = (int) (gs->frameNum + lifeTime);
	// atlas contains 2x2 textures, pick one of them
	s->texOffsetX = (gu->RandInt() & 128)? 0: 0.5f;
	s->texOffsetY = (gu->RandInt() & 128)? 0: 0.5f;

	s->x1 = (int) max(0.f,                  (pos.x - radius) / 16.0f    );
	s->x2 = (int) min(float(gs->hmapx - 1), (pos.x + radius) / 16.0f + 1);
	s->y1 = (int) max(0.f,                  (pos.z - radius) / 16.0f    );
	s->y2 = (int) min(float(gs->hmapy - 1), (pos.z + radius) / 16.0f + 1);

	s->basesize = (s->x2 - s->x1) * (s->y2 - s->y1);
	s->overdrawn = 0;
	s->lastTest = 0;

	GML_STDMUTEX_LOCK(scar); // AddExplosion

	scarsToBeAdded.push_back(s);
}


void CGroundDecalHandler::LoadScar(const std::string& file, unsigned char* buf,
                                   int xoffset, int yoffset)
{
	CBitmap bm;
	if (!bm.Load(file)) {
		throw content_error("Could not load scar from file " + file);
	}
	bool isBitmap = (FileSystem::GetExtension(file) == "bmp");
	if (isBitmap) {
		//! bitmaps don't have an alpha channel
		//! so use: red := brightness & green := alpha
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
	} else {
		for (int y = 0; y < bm.ysize; ++y) {
			for (int x = 0; x < bm.xsize; ++x) {
				const int memIndex = ((y * bm.xsize) + x) * 4;
				const int bufIndex = (((y + yoffset) * 512) + x + xoffset) * 4;
				buf[bufIndex + 0]    = bm.mem[memIndex + 0];
				buf[bufIndex + 1]    = bm.mem[memIndex + 1];
				buf[bufIndex + 2]    = bm.mem[memIndex + 2];
				buf[bufIndex + 3]    = bm.mem[memIndex + 3];
			}
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


void CGroundDecalHandler::MoveSolidObject(CSolidObject* object, const float3& pos)
{
	if (decalLevel == 0)
		return;

	SolidObjectDecalDef& decalDef = *const_cast<SolidObjectDecalDef*>(&object->objectDef->decalDef);
	if (!decalDef.useGroundDecal || decalDef.groundDecalType < -1)
		return;

	if (decalDef.groundDecalType < 0) {
		decalDef.groundDecalType = GetSolidObjectDecalType(decalDef.groundDecalTypeName);
		if (!decalDef.useGroundDecal || decalDef.groundDecalType < -1)
			return;
	}

	GML_STDMUTEX_LOCK(decal); // AddSolidObject

	SolidObjectGroundDecal* olddecal = object->groundDecal;
	if (olddecal != NULL) {
		olddecal->owner = NULL;
		olddecal->gbOwner = NULL;
	}

	const int sizex = decalDef.groundDecalSizeX;
	const int sizey = decalDef.groundDecalSizeY;

	SolidObjectGroundDecal* decal = new SolidObjectGroundDecal();

	decal->owner = object;
	decal->gbOwner = 0;
	decal->alphaFalloff = decalDef.groundDecalDecaySpeed;
	decal->alpha = 0.0f;
	decal->pos = pos;
	decal->radius = math::sqrtf(float(sizex * sizex + sizey * sizey)) * SQUARE_SIZE + 20.0f;
	decal->facing = object->buildFacing;
	// convert to heightmap coors
	decal->xsize = sizex << 1;
	decal->ysize = sizey << 1;

	if (object->buildFacing == FACING_EAST || object->buildFacing == FACING_WEST) {
		// swap xsize and ysize if object faces East or West
		std::swap(decal->xsize, decal->ysize);
	}

	// position of top-left corner
	decal->posx = (pos.x / SQUARE_SIZE) - (decal->xsize >> 1);
	decal->posy = (pos.z / SQUARE_SIZE) - (decal->ysize >> 1);

	object->groundDecal = decal;
	objectDecalTypes[decalDef.groundDecalType]->objectDecals.insert(decal);
}


void CGroundDecalHandler::RemoveSolidObject(CSolidObject* object, GhostSolidObject* gb)
{
	if (decalLevel == 0)
		return;

	GML_STDMUTEX_LOCK(decal); // RemoveSolidObject

	assert(object);
	SolidObjectGroundDecal* decal = object->groundDecal;

	if (decal == NULL)
		return;

	if (gb != NULL)
		gb->decal = decal;

	decal->owner = NULL;
	decal->gbOwner = gb;
	object->groundDecal = NULL;
}


/**
 * @brief immediately remove an object's ground decal, if any (without fade out)
 */
void CGroundDecalHandler::ForceRemoveSolidObject(CSolidObject* object)
{
	if (decalLevel == 0)
		return;

	GML_STDMUTEX_LOCK(decal); // ForcedRemoveSolidObject

	SolidObjectGroundDecal* decal = object->groundDecal;

	if (decal == NULL)
		return;

	decal->owner = NULL;
	decal->alpha = 0.0f;
	object->groundDecal = NULL;
}


int CGroundDecalHandler::GetSolidObjectDecalType(const std::string& name)
{
	if (decalLevel == 0)
		return -2;

	const std::string& lowerName = StringToLower(name);
	const std::string& fullName = "unittextures/" + lowerName;

	int decalType = 0;

	GML_STDMUTEX_LOCK(decal); // GetSolidObjectDecalType

	std::vector<SolidObjectDecalType*>::iterator bi;
	for (bi = objectDecalTypes.begin(); bi != objectDecalTypes.end(); ++bi) {
		if ((*bi)->name == lowerName) {
			return decalType;
		}
		++decalType;
	}

	CBitmap bm;
	if (!bm.Load(fullName)) {
		LOG_L(L_ERROR, "[%s] Could not load object-decal from file \"%s\"", __FUNCTION__, fullName.c_str());
		return -2;
	}

	SolidObjectDecalType* tt = new SolidObjectDecalType();
	tt->name = lowerName;
	tt->texture = bm.CreateTexture(true);

	objectDecalTypes.push_back(tt);
	return (objectDecalTypes.size() - 1);
}

void CGroundDecalHandler::GhostCreated(CSolidObject* object, GhostSolidObject* gb) {
	if (object->objectDef->decalDef.useGroundDecal)
		RemoveSolidObject(object, gb);
}

void CGroundDecalHandler::GhostDestroyed(GhostSolidObject* gb) {
	if (gb->decal)
		gb->decal->gbOwner = NULL;
}


SolidObjectGroundDecal::~SolidObjectGroundDecal() {
	SafeDelete(va);
}

CGroundDecalHandler::Scar::~Scar() {
	SafeDelete(va);
}

void CGroundDecalHandler::ExplosionOccurred(const CExplosionEvent& event) {
	AddExplosion(event.GetPos(), event.GetDamage(), event.GetRadius(), ((event.GetWeaponDef() != NULL) && event.GetWeaponDef()->visuals.explosionScar));
}

void CGroundDecalHandler::RenderUnitCreated(const CUnit* unit, int cloaked) {
	if (unit->unitDef->decalDef.useGroundDecal)
		MoveSolidObject(const_cast<CUnit*>(unit), unit->pos);
}

void CGroundDecalHandler::RenderUnitDestroyed(const CUnit* unit) {
	if (decalLevel == 0)
		return;

	GML_STDMUTEX_LOCK(track); // RemoveUnit

	CUnit* u = const_cast<CUnit*>(unit);
	RemoveSolidObject(u, NULL);

	if (unit->myTrack != NULL) {
		u->myTrack->owner = NULL;
		u->myTrack = NULL;
	}
}

void CGroundDecalHandler::RenderFeatureCreated(const CFeature* feature)
{
	if (feature->objectDef->decalDef.useGroundDecal)
		MoveSolidObject(const_cast<CFeature*>(feature), feature->pos);
}

void CGroundDecalHandler::RenderFeatureMoved(const CFeature* feature, const float3& oldpos, const float3& newpos) {
	if (feature->objectDef->decalDef.useGroundDecal && (feature->def->drawType == DRAWTYPE_MODEL))
		MoveSolidObject(const_cast<CFeature *>(feature), newpos);
}

void CGroundDecalHandler::UnitLoaded(const CUnit* unit, const CUnit* transport) {
	if (unit->unitDef->decalDef.useGroundDecal)
		RemoveSolidObject(const_cast<CUnit *>(unit), NULL); // FIXME: Add a RenderUnitLoaded event
}

void CGroundDecalHandler::UnitUnloaded(const CUnit* unit, const CUnit* transport) {
	if (unit->unitDef->decalDef.useGroundDecal)
		MoveSolidObject(const_cast<CUnit *>(unit), unit->pos); // FIXME: Add a RenderUnitUnloaded event
}
