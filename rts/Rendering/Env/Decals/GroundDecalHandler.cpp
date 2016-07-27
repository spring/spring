/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <cctype>

#include "GroundDecalHandler.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Lua/LuaParser.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Projectiles/ExplosionListener.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/Util.h"
#include "System/FileSystem/FileSystem.h"


using std::min;
using std::max;


CONFIG(int, GroundScarAlphaFade).defaultValue(0);

CGroundDecalHandler::CGroundDecalHandler()
	: CEventClient("[CGroundDecalHandler]", 314159, false)
{
	if (!GetDrawDecals())
		return;

	eventHandler.AddClient(this);
	CExplosionCreator::AddExplosionListener(this);

	groundScarAlphaFade = (configHandler->GetInt("GroundScarAlphaFade") != 0);

	std::vector<unsigned char> buf(512 * 512 * 4);

	LuaParser resourcesParser("gamedata/resources.lua",
	                          SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!resourcesParser.Execute()) {
		LOG_L(L_ERROR, "Failed to load resources: %s",
				resourcesParser.GetErrorLog().c_str());
	}

	const LuaTable scarsTable = resourcesParser.GetRoot().SubTable("graphics").SubTable("scars");
	LoadScar("bitmaps/" + scarsTable.GetString(2, "scars/scar2.bmp"), buf.data(), 0,   0);
	LoadScar("bitmaps/" + scarsTable.GetString(3, "scars/scar3.bmp"), buf.data(), 256, 0);
	LoadScar("bitmaps/" + scarsTable.GetString(1, "scars/scar1.bmp"), buf.data(), 0,   256);
	LoadScar("bitmaps/" + scarsTable.GetString(4, "scars/scar4.bmp"), buf.data(), 256, 256);

	glGenTextures(1, &scarTex);
	glBindTexture(GL_TEXTURE_2D, scarTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glBuildMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,512, 512, GL_RGBA, GL_UNSIGNED_BYTE, buf.data());

	scarFieldX = mapDims.mapx / 32;
	scarFieldY = mapDims.mapy / 32;
	scarField.resize(scarFieldX * scarFieldY);

	lastTest = 0;
	maxOverlap = decalLevel + 1;

	LoadDecalShaders();
}



CGroundDecalHandler::~CGroundDecalHandler()
{
	eventHandler.RemoveClient(this);

	for (SolidObjectDecalType* dctype: objectDecalTypes) {
		for (SolidObjectGroundDecal* dc: dctype->objectDecals) {
			if (dc->owner)
				dc->owner->groundDecal = nullptr;
			if (dc->gbOwner)
				dc->gbOwner->decal = nullptr;
			delete dc;
		}
		glDeleteTextures(1, &dctype->texture);
		delete dctype;
	}

	for (auto& scar: scars)
		delete scar;

	for (auto& scar: scarsToBeAdded)
		delete scar;

	glDeleteTextures(1, &scarTex);

	shaderHandler->ReleaseProgramObjects("[GroundDecalHandler]");
	decalShaders.clear();
}

void CGroundDecalHandler::LoadDecalShaders() {
	#define sh shaderHandler
	decalShaders.resize(DECAL_SHADER_LAST, NULL);

	// SM3 maps have no baked lighting, so decals blend differently
	const bool haveShadingTexture = (readMap->GetShadingTexture() != 0);
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
			decalShaders[DECAL_SHADER_GLSL]->SetUniform2f(3, 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE));
			decalShaders[DECAL_SHADER_GLSL]->SetUniform1f(7, sky->GetLight()->GetGroundShadowDensity());
			decalShaders[DECAL_SHADER_GLSL]->Disable();
			decalShaders[DECAL_SHADER_GLSL]->Validate();

			decalShaders[DECAL_SHADER_CURR] = decalShaders[DECAL_SHADER_GLSL];
		}
	}

	#undef sh
}

void CGroundDecalHandler::SunChanged() {
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
	const float* hm = readMap->GetCornerHeightMapUnsynced();

	const int gsmx  = mapDims.mapx;
	const int gsmx1 = mapDims.mapxp1;
	const int gsmy  = mapDims.mapy;

	SColor color(255, 255, 255, int(decal->alpha * 255));

	#ifndef DEBUG
	#define HEIGHT(z, x) (hm[((z) * gsmx1) + (x)])
	#else
	#define HEIGHT(z, x) (assert((z) <= gsmy), assert((x) <= gsmx), (hm[((z) * gsmx1) + (x)]))
	#endif

	CVertexArray*& va = decal->va;

	if (va == nullptr) {
		// NOTE: this really needs CLOD'ing
		va = new CVertexArray();
		va->Initialize();

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

				AddQuadVertices(va, px, yv, pz, uv, color);
			}
		}
	} else {
		const int numVerts = va->drawIndex() / VA_SIZE_TC;

		va->ResetPos();
		VA_TYPE_TC* mem = va->GetTypedVertexArray<VA_TYPE_TC>(numVerts);

		for (int i = 0; i < numVerts; ++i) {
			const int x = int(mem[i].p.x) >> 3;
			const int z = int(mem[i].p.z) >> 3;

			// update the height and alpha
			mem[i].p.y = hm[z * gsmx1 + x];
			mem[i].c   = color;
		}

		// pos{x,y} are multiples of SQUARE_SIZE, but pos might not be
		// shift the decal visually in the latter case so it is aligned
		// with the object on top of it
		glPushMatrix();
		glTranslatef(int(decal->pos.x) % SQUARE_SIZE, 0.0f, int(decal->pos.z) % SQUARE_SIZE);
		va->DrawArrayTC(GL_QUADS);
		glPopMatrix();
	}

	#undef HEIGHT
}


inline void CGroundDecalHandler::DrawGroundScar(CGroundDecalHandler::Scar* scar, bool fade)
{
	// TODO: do we want LOS-checks for decals?
	if (!camera->InView(scar->pos, scar->radius + 16))
		return;

	SColor color(255, 255, 255, 255);

	if (scar->va == NULL) {
		scar->va = new CVertexArray();
		scar->va->Initialize();

		float3 pos = scar->pos;
		const float radius = scar->radius;
		const float radius4 = radius * 4.0f;
		const float tx = scar->texOffsetX;
		const float ty = scar->texOffsetY;

		int sx = (int) max(0.0f,                 (pos.x - radius) * 0.0625f);
		int ex = (int) min(float(mapDims.hmapx - 1), (pos.x + radius) * 0.0625f);
		int sz = (int) max(0.0f,                 (pos.z - radius) * 0.0625f);
		int ez = (int) min(float(mapDims.hmapy - 1), (pos.z + radius) * 0.0625f);

		// create the scar texture-quads
		float px1 = sx * 16;
		for (int x = sx; x <= ex; ++x) {
			float px2 = px1 + 16;
			float pz1 = sz * 16;

			for (int z = sz; z <= ez; ++z) {
				float pz2 = pz1 + 16;
				float tx1 = min(0.5f, (pos.x - px1) / radius4 + 0.25f);
				float tx2 = max(0.0f, (pos.x - px2) / radius4 + 0.25f);
				float tz1 = min(0.5f, (pos.z - pz1) / radius4 + 0.25f);
				float tz2 = max(0.0f, (pos.z - pz2) / radius4 + 0.25f);
				float h1 = CGround::GetHeightReal(px1, pz1, false);
				float h2 = CGround::GetHeightReal(px2, pz1, false);
				float h3 = CGround::GetHeightReal(px2, pz2, false);
				float h4 = CGround::GetHeightReal(px1, pz2, false);

				scar->va->AddVertexTC(float3(px1, h1, pz1), tx1 + tx, tz1 + ty, color);
				scar->va->AddVertexTC(float3(px2, h2, pz1), tx2 + tx, tz1 + ty, color);
				scar->va->AddVertexTC(float3(px2, h3, pz2), tx2 + tx, tz2 + ty, color);
				scar->va->AddVertexTC(float3(px1, h4, pz2), tx1 + tx, tz2 + ty, color);
				pz1 = pz2;
			}

			px1 = px2;
		}
	} else {
		if (fade) {
			if ((scar->creationTime + 10) > gs->frameNum) {
				color[3] = (int) (scar->startAlpha * (gs->frameNum - scar->creationTime) * 0.1f);
			} else {
				color[3] = (int) (scar->startAlpha - (gs->frameNum - scar->creationTime) * scar->alphaFalloff);
			}

			const int gsmx1 = mapDims.mapx + 1;
			const float* hm = readMap->GetCornerHeightMapUnsynced();

			const int num = scar->va->drawIndex() / VA_SIZE_TC;
			scar->va->ResetPos();
			VA_TYPE_TC* mem = scar->va->GetTypedVertexArray<VA_TYPE_TC>(num);

			for (int i = 0; i < num; ++i) {
				const int x = int(mem[i].p.x) >> 3;
				const int z = int(mem[i].p.z) >> 3;

				// update the height and alpha
				mem[i].p.y = hm[z * gsmx1 + x];
				mem[i].c   = color;
			}
		}

		scar->va->DrawArrayTC(GL_QUADS);
	}
}



void CGroundDecalHandler::GatherDecalsForType(CGroundDecalHandler::SolidObjectDecalType* decalType) {
	decalsToDraw.clear();

	auto& objectDecals = decalType->objectDecals;

	for (int i = 0; i < objectDecals.size();) {
		SolidObjectGroundDecal* decal = objectDecals[i];
		CSolidObject* decalOwner = decal->owner;

		if (decalOwner == NULL) {
			if (decal->gbOwner == NULL) {
				decal->alpha -= (decal->alphaFalloff * globalRendering->lastFrameTime * 0.001f * gs->speedFactor);
			} else if (decal->gbOwner->lastDrawFrame < (globalRendering->drawFrame - 1)) {
				++i;
				continue;
			}
			if (decal->alpha < 0.0f) {
				// make sure RemoveSolidObject() won't try to modify this decal
				if (decalOwner != NULL) {
					decalOwner->groundDecal = NULL;
				}

				objectDecals[i] = objectDecals.back();
				objectDecals.pop_back();

				delete decal;
				continue;
			}
			++i;
		} else {
			++i;
			if (decalOwner->GetBlockingMapID() < unitHandler->MaxUnits()) {
				const CUnit* decalOwnerUnit = static_cast<const CUnit*>(decalOwner);
				if (decalOwnerUnit->isIcon)
					continue;
				if (!gu->spectatingFullView &&
					(decalOwnerUnit->losStatus[gu->myAllyTeam] & LOS_INLOS) == 0 &&
					(!gameSetup->ghostedBuildings || (decalOwnerUnit->losStatus[gu->myAllyTeam] & LOS_PREVLOS) == 0))
					continue;

				decal->alpha = std::max(0.0f, decalOwnerUnit->buildProgress);
			} else {
				const CFeature* decalOwnerFeature = static_cast<const CFeature*>(decalOwner);
				if (!decalOwnerFeature->IsInLosForAllyTeam(gu->myAllyTeam))
					continue;
				if (decalOwnerFeature->drawAlpha < 0.01f)
					continue;
				decal->alpha = decalOwnerFeature->drawAlpha;
			}
		}
		if (!camera->InView(decal->pos, decal->radius))
			continue;
		decalsToDraw.push_back(decal);
	}
}

void CGroundDecalHandler::DrawObjectDecals() {
	// create and draw the quads for each building decal
	for (SolidObjectDecalType* decalType: objectDecalTypes) {
		if (decalType->objectDecals.empty())
			continue;

		{
			GatherDecalsForType(decalType);
		}

		if (!decalsToDraw.empty()) {
			glBindTexture(GL_TEXTURE_2D, decalType->texture);
			for (SolidObjectGroundDecal* decal: decalsToDraw) {
				DrawObjectDecal(decal);
			}
		}

		// glBindTexture(GL_TEXTURE_2D, 0);
	}
}


void CGroundDecalHandler::AddScars()
{
	for (Scar* s: scarsToBeAdded) {
		TestOverlaps(s);

		int x1 = s->x1 / 16;
		int x2 = min(scarFieldX - 1, s->x2 / 16);
		int y1 = s->y1 / 16;
		int y2 = min(scarFieldY - 1, s->y2 / 16);

		for (int y = y1; y <= y2; ++y) {
			for (int x = x1; x <= x2; ++x) {
				VectorInsertUnique(scarField[y * scarFieldX + x], s);
			}
		}

		VectorInsertUnique(scars, s);
	}

	scarsToBeAdded.clear();
}

void CGroundDecalHandler::DrawScars() {
	// create and draw the 16x16 quads for each ground scar
	for (int i = 0; i < scars.size();) {
		Scar* scar = scars[i];

		if (scar->lifeTime < gs->frameNum) {
			RemoveScar(scar, false);
			scars[i] = scars.back();
			scars.pop_back();
			continue;
		}

		DrawGroundScar(scar, groundScarAlphaFade);
		++i;
	}
}




void CGroundDecalHandler::Draw()
{
	trackHandler.Draw();

	if (!GetDrawDecals())
		return;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glDepthMask(0);

	BindTextures();
	BindShader(sunLighting->groundAmbientColor * CGlobalRendering::SMF_INTENSITY_MULT);
	DrawDecals();
	KillTextures();

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_BLEND);
}

void CGroundDecalHandler::BindTextures()
{
	{
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, readMap->GetShadingTexture());
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

		glMultiTexCoord4f(GL_TEXTURE1_ARB, 1.0f,1.0f,1.0f,1.0f); // workaround a nvidia bug with TexGen
		SetTexGen(1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE), 0, 0);
	}

	if (shadowHandler->ShadowsLoaded()) {
		shadowHandler->SetupShadowTexSampler(GL_TEXTURE2, true);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); //??
	}

	if (infoTextureHandler->IsEnabled()) {
		glActiveTexture(GL_TEXTURE3);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

		glMultiTexCoord4f(GL_TEXTURE3_ARB, 1.0f,1.0f,1.0f,1.0f); // workaround a nvidia bug with TexGen
		SetTexGen(1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE), 0, 0);

		glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());
	}

	glActiveTexture(GL_TEXTURE0);
}

void CGroundDecalHandler::KillTextures()
{
	{
		glActiveTexture(GL_TEXTURE3); // infotex
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

	if (shadowHandler->ShadowsLoaded()) {
		shadowHandler->ResetShadowTexSampler(GL_TEXTURE2, true);

		glActiveTexture(GL_TEXTURE1);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
	}

	{
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

	glActiveTexture(GL_TEXTURE0);
}

void CGroundDecalHandler::BindShader(const float3& ambientColor)
{
	decalShaders[DECAL_SHADER_CURR]->Enable();

	if (decalShaders[DECAL_SHADER_CURR] == decalShaders[DECAL_SHADER_ARB]) {
		decalShaders[DECAL_SHADER_CURR]->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
		decalShaders[DECAL_SHADER_CURR]->SetUniform4f(10, 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE), 0.0f, 1.0f);
		decalShaders[DECAL_SHADER_CURR]->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
		decalShaders[DECAL_SHADER_CURR]->SetUniform4f(10, ambientColor.x, ambientColor.y, ambientColor.z, 1.0f);
		decalShaders[DECAL_SHADER_CURR]->SetUniform4f(11, 0.0f, 0.0f, 0.0f, sky->GetLight()->GetGroundShadowDensity());

		glMatrixMode(GL_MATRIX0_ARB);
		glLoadMatrixf(shadowHandler->GetShadowMatrixRaw());
		glMatrixMode(GL_MODELVIEW);
	} else {
		decalShaders[DECAL_SHADER_CURR]->SetUniform4f(4, ambientColor.x, ambientColor.y, ambientColor.z, 1.0f);
		decalShaders[DECAL_SHADER_CURR]->SetUniformMatrix4fv(5, false, shadowHandler->GetShadowMatrixRaw());
		decalShaders[DECAL_SHADER_CURR]->SetUniform4fv(6, &(shadowHandler->GetShadowParams().x));
	}
}

void CGroundDecalHandler::DrawDecals()
{
	// draw building decals
	glPolygonOffset(-10, -200);
	DrawObjectDecals();

	// draw explosion decals
	glBindTexture(GL_TEXTURE_2D, scarTex);
	glPolygonOffset(-10, -400);
	AddScars();
	DrawScars();

	decalShaders[DECAL_SHADER_CURR]->Disable();
}


void CGroundDecalHandler::AddDecal(CUnit* unit, const float3& newPos)
{
	if (!GetDrawDecals())
		return;

	MoveSolidObject(unit, newPos);
}


void CGroundDecalHandler::AddExplosion(float3 pos, float damage, float radius)
{
	if (!GetDrawDecals())
		return;

	const float altitude = pos.y - CGround::GetHeightReal(pos.x, pos.z, false);

	// no decals for below-ground explosions
	if (altitude <= -1.0f)
		return;
	if (altitude >= radius)
		return;

	pos.y -= altitude;
	radius -= altitude;

	if (radius < 5.0f)
		return;

	damage = std::min(damage, radius * 30.0f);
	damage *= (radius / (radius + altitude));
	radius = std::min(radius, damage * 0.25f);

	if (damage > 400.0f)
		damage = 400.0f + std::sqrt(damage - 399.0f);

	const int ttl = std::max(1.0f, decalLevel * damage * 3.0f);

	Scar* s = new Scar();
	s->pos = pos.cClampInBounds();
	s->radius = radius * 1.4f;
	s->creationTime = gs->frameNum;
	s->startAlpha = std::max(50.0f, std::min(255.0f, damage));
	s->lifeTime = int(gs->frameNum + ttl);
	s->alphaFalloff = s->startAlpha / ttl;
	// atlas contains 2x2 textures, pick one of them
	s->texOffsetX = (gu->RandInt() & 128)? 0: 0.5f;
	s->texOffsetY = (gu->RandInt() & 128)? 0: 0.5f;

	s->x1 = int(std::max(0.f,                  (s->pos.x - radius) / (SQUARE_SIZE * 2)    ));
	s->x2 = int(std::min(float(mapDims.hmapx - 1), (s->pos.x + radius) / (SQUARE_SIZE * 2) + 1));
	s->y1 = int(std::max(0.f,                  (s->pos.z - radius) / (SQUARE_SIZE * 2)    ));
	s->y2 = int(std::min(float(mapDims.hmapy - 1), (s->pos.z + radius) / (SQUARE_SIZE * 2) + 1));

	s->basesize = (s->x2 - s->x1) * (s->y2 - s->y1);
	s->overdrawn = 0;
	s->lastTest = 0;

	scarsToBeAdded.push_back(s);
}


void CGroundDecalHandler::LoadScar(const std::string& file, unsigned char* buf,
                                   int xoffset, int yoffset)
{
	CBitmap bm;
	if (!bm.Load(file)) {
		throw content_error("Could not load scar from file " + file);
	}
	if (bm.ysize != 256 || bm.xsize != 256) {
		bm = bm.CreateRescaled(256,256);
	}

	if (FileSystem::GetExtension(file) == "bmp") {
		// bitmaps don't have an alpha channel
		// so use: red := brightness & green := alpha
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
		// we copy into an atlas, so we need to copy line by line
		for (int y = 0; y < bm.ysize; ++y) {
			const int memIndex = (y * bm.xsize) * 4;
			const int bufIndex = (((y + yoffset) * 512) + xoffset) * 4;
			memcpy(&buf[bufIndex], &bm.mem[memIndex], bm.xsize * sizeof(SColor));
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
	const int x1 = scar->x1 / 16;
	const int y1 = scar->y1 / 16;
	const int x2 = min(scarFieldX - 1, scar->x2 / 16);
	const int y2 = min(scarFieldY - 1, scar->y2 / 16);

	++lastTest;

	for (int y = y1; y <= y2; ++y) {
		for (int x = x1; x <= x2; ++x) {
			auto& quad = scarField[y * scarFieldX+ x];

			for (int i = 0; i < quad.size(); ) {
				Scar* tested = quad[i];

				if (lastTest == tested->lastTest) {
					i++; continue;
				}
				if (scar->lifeTime < tested->lifeTime) {
					i++; continue;
				}

				tested->lastTest = lastTest;

				const int overlap = OverlapSize(scar, tested);

				if (overlap == 0 || tested->basesize == 0) {
					i++; continue;
				}

				tested->overdrawn += (overlap / tested->basesize);

				if (tested->overdrawn <= maxOverlap) {
					i++; continue;
				}

				RemoveScar(tested, true);
			}
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
			VectorErase(scarField[y * scarFieldX + x], scar);
		}
	}

	if (removeFromScars)
		VectorErase(scars, scar);

	delete scar;
}

int CGroundDecalHandler::GetSolidObjectDecalType(const std::string& name)
{
	if (!GetDrawDecals())
		return -2;

	const std::string& lowerName = StringToLower(name);
	const std::string& fullName = "unittextures/" + lowerName;

	int decalType = 0;
	for (auto& dt: objectDecalTypes) {
		if (dt->name == lowerName) {
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
	tt->texture = bm.CreateMipMapTexture();

	objectDecalTypes.push_back(tt);
	return (objectDecalTypes.size() - 1);
}


SolidObjectGroundDecal::~SolidObjectGroundDecal() {
	SafeDelete(va);
}

CGroundDecalHandler::Scar::~Scar() {
	SafeDelete(va);
}








void CGroundDecalHandler::MoveSolidObject(CSolidObject* object, const float3& pos)
{
	if (!GetDrawDecals())
		return;

	const SolidObjectDecalDef& decalDef = object->GetDef()->decalDef;

	if (!decalDef.useGroundDecal || decalDef.groundDecalType < -1)
		return;

	if (decalDef.groundDecalType < 0) {
		const_cast<SolidObjectDecalDef&>(decalDef).groundDecalType = GetSolidObjectDecalType(decalDef.groundDecalTypeName);

		if (decalDef.groundDecalType < -1)
			return;
	}

	SolidObjectGroundDecal* olddecal = object->groundDecal;
	if (olddecal != nullptr) {
		olddecal->owner = nullptr;
		olddecal->gbOwner = nullptr;
	}

	const int sizex = decalDef.groundDecalSizeX;
	const int sizey = decalDef.groundDecalSizeY;

	SolidObjectGroundDecal* decal = new SolidObjectGroundDecal();

	decal->owner = object;
	decal->gbOwner = nullptr;
	decal->alphaFalloff = decalDef.groundDecalDecaySpeed;
	decal->alpha = 0.0f;
	decal->pos = pos;
	decal->radius = std::sqrt(float(sizex * sizex + sizey * sizey)) * SQUARE_SIZE + 20.0f;
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
	objectDecalTypes[decalDef.groundDecalType]->objectDecals.push_back(decal);
}


void CGroundDecalHandler::RemoveSolidObject(CSolidObject* object, GhostSolidObject* gb)
{
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
	SolidObjectGroundDecal* decal = object->groundDecal;

	if (decal == NULL)
		return;

	decal->owner = NULL;
	decal->alpha = 0.0f;
	object->groundDecal = NULL;
}













void CGroundDecalHandler::UnitMoved(const CUnit* unit) { AddDecal(const_cast<CUnit*>(unit), unit->pos); }

void CGroundDecalHandler::GhostDestroyed(GhostSolidObject* gb) {
	if (gb->decal) {
		gb->decal->gbOwner = NULL;

		//If a ghost wasn't drawn, remove the decal
		if (gb->lastDrawFrame < (globalRendering->drawFrame - 1))
			gb->decal->alpha = 0.0f;
	}
}






void CGroundDecalHandler::GhostCreated(CSolidObject* object, GhostSolidObject* gb) { RemoveSolidObject(object, gb); }
void CGroundDecalHandler::FeatureMoved(const CFeature* feature, const float3& oldpos) { MoveSolidObject(const_cast<CFeature*>(feature), feature->pos); }

void CGroundDecalHandler::ExplosionOccurred(const CExplosionParams& event) {
	if ((event.weaponDef != nullptr) && !event.weaponDef->visuals.explosionScar)
		return;

	AddExplosion(event.pos, event.damages.GetDefault(), event.craterAreaOfEffect);
}

void CGroundDecalHandler::RenderUnitCreated(const CUnit* unit, int cloaked) { MoveSolidObject(const_cast<CUnit*>(unit), unit->pos); }
void CGroundDecalHandler::RenderUnitDestroyed(const CUnit* unit) {
	RemoveSolidObject(const_cast<CUnit*>(unit), nullptr);
}

void CGroundDecalHandler::RenderFeatureCreated(const CFeature* feature) { MoveSolidObject(const_cast<CFeature*>(feature), feature->pos); }
void CGroundDecalHandler::RenderFeatureDestroyed(const CFeature* feature) { RemoveSolidObject(const_cast<CFeature*>(feature), nullptr); }

// FIXME: Add a RenderUnitLoaded event
void CGroundDecalHandler::UnitLoaded(const CUnit* unit, const CUnit* transport) { ForceRemoveSolidObject(const_cast<CUnit*>(unit)); }
void CGroundDecalHandler::UnitUnloaded(const CUnit* unit, const CUnit* transport) { MoveSolidObject(const_cast<CUnit*>(unit), unit->pos); }

