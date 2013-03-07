/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "UnitDrawer.h"

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Player.h"
#include "Game/UI/MiniMap.h"
#include "Lua/LuaMaterial.h"
#include "Lua/LuaUnitMaterial.h"
#include "Lua/LuaRules.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"

#include "Rendering/Env/ISky.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/FarTextureHandler.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/IconHandler.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Models/WorldObjectModelRenderer.h"

#include "Sim/Features/Feature.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Weapons/Weapon.h"

#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/Platform/Watchdog.h"
#include "System/TimeProfiler.h"
#include "System/Util.h"

#ifdef USE_GML
#include "lib/gml/gmlsrv.h"
extern gmlClientServer<void, int, CUnit*> *gmlProcessor;
#endif

#define UNIT_SHADOW_ALPHA_MASKING

CUnitDrawer* unitDrawer;

CONFIG(int, UnitLodDist).defaultValue(1000);
CONFIG(int, UnitIconDist).defaultValue(10000);
CONFIG(float, UnitTransparency).defaultValue(0.7f);
CONFIG(bool, ShowHealthBars).defaultValue(true);
CONFIG(bool, MultiThreadDrawUnit).defaultValue(true);
CONFIG(bool, MultiThreadDrawUnitShadow).defaultValue(true);

CONFIG(int, MaxDynamicModelLights)
	.defaultValue(1)
	.minimumValue(0);

CONFIG(bool, AdvUnitShading).defaultValue(true);
CONFIG(float, LODScale).defaultValue(1.0f);
CONFIG(float, LODScaleShadow).defaultValue(1.0f);
CONFIG(float, LODScaleReflection).defaultValue(1.0f);
CONFIG(float, LODScaleRefraction).defaultValue(1.0f);

static bool LUA_DRAWING = false; // FIXME
static float UNIT_GLOBAL_LOD_FACTOR = 1.0f;

inline static void SetUnitGlobalLODFactor(float value)
{
	UNIT_GLOBAL_LOD_FACTOR = (value * camera->GetLPPScale());
}

static float GetLODFloat(const string& name)
{
	// NOTE: the inverse of the value is used
	const float value = configHandler->GetFloat(name);
	if (value <= 0.0f) {
		return 1.0f;
	}
	return (1.0f / value);
}


CUnitDrawer::CUnitDrawer(): CEventClient("[CUnitDrawer]", 271828, false)
{
	eventHandler.AddClient(this);

	SetUnitDrawDist((float)configHandler->GetInt("UnitLodDist"));
	SetUnitIconDist((float)configHandler->GetInt("UnitIconDist"));

	LODScale           = GetLODFloat("LODScale");
	LODScaleShadow     = GetLODFloat("LODScaleShadow");
	LODScaleReflection = GetLODFloat("LODScaleReflection");
	LODScaleRefraction = GetLODFloat("LODScaleRefraction");

	unitAmbientColor = mapInfo->light.unitAmbientColor;
	unitSunColor = mapInfo->light.unitSunColor;

	cloakAlpha  = std::max(0.11f, std::min(1.0f, 1.0f - configHandler->GetFloat("UnitTransparency")));
	cloakAlpha1 = std::min(1.0f, cloakAlpha + 0.1f);
	cloakAlpha2 = std::min(1.0f, cloakAlpha + 0.2f);
	cloakAlpha3 = std::min(1.0f, cloakAlpha + 0.4f);

	// load unit explosion generators and decals
	for (size_t unitDefID = 1; unitDefID < unitDefHandler->unitDefs.size(); unitDefID++) {
		UnitDef* ud = unitDefHandler->unitDefs[unitDefID];

		for (std::vector<std::string>::const_iterator it = ud->modelCEGTags.begin(); it != ud->modelCEGTags.end(); ++it) {
			ud->sfxExplGens.push_back(explGenHandler->LoadGenerator(*it));
		}
	}


	deadGhostBuildings.resize(MODELTYPE_OTHER);
	liveGhostBuildings.resize(MODELTYPE_OTHER);

	opaqueModelRenderers.resize(MODELTYPE_OTHER, NULL);
	cloakedModelRenderers.resize(MODELTYPE_OTHER, NULL);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		opaqueModelRenderers[modelType] = IWorldObjectModelRenderer::GetInstance(modelType);
		cloakedModelRenderers[modelType] = IWorldObjectModelRenderer::GetInstance(modelType);
	}

	unitRadarIcons.resize(teamHandler->ActiveAllyTeams());

#ifdef USE_GML
	showHealthBars = GML::Enabled() && configHandler->GetBool("ShowHealthBars");
	multiThreadDrawUnit = configHandler->GetBool("MultiThreadDrawUnit");
	multiThreadDrawUnitShadow = configHandler->GetBool("MultiThreadDrawUnitShadow");
#endif

	lightHandler.Init(2U, configHandler->GetInt("MaxDynamicModelLights"));

	advFade = GLEW_NV_vertex_program2;
	advShading = (LoadModelShaders() && cubeMapHandler->Init());
}

CUnitDrawer::~CUnitDrawer()
{
	eventHandler.RemoveClient(this);

	shaderHandler->ReleaseProgramObjects("[UnitDrawer]");
	cubeMapHandler->Free();

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		std::set<GhostSolidObject*>& ghostSet = deadGhostBuildings[modelType];
		std::set<GhostSolidObject*>::iterator ghostSetIt;

		for (ghostSetIt = ghostSet.begin(); ghostSetIt != ghostSet.end(); ++ghostSetIt) {
			delete *ghostSetIt;
		}

		deadGhostBuildings[modelType].clear();
		liveGhostBuildings[modelType].clear();
	}

	deadGhostBuildings.clear();
	liveGhostBuildings.clear();

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		delete opaqueModelRenderers[modelType];
		delete cloakedModelRenderers[modelType];
	}

	opaqueModelRenderers.clear();
	cloakedModelRenderers.clear();
	modelShaders.clear();

	unitRadarIcons.clear();
	unsortedUnits.clear();

	lightHandler.Kill();
}



bool CUnitDrawer::LoadModelShaders()
{
	#define sh shaderHandler

	if (!globalRendering->haveARB) {
		// not possible to do (ARB) shader-based model rendering
		LOG_L(L_WARNING,
				"[%s] OpenGL ARB extensions missing for advanced unit shading",
				__FUNCTION__);
		return false;
	}
	if (!configHandler->GetBool("AdvUnitShading")) {
		// not allowed to do shader-based model rendering
		return false;
	}

	modelShaders.resize(MODEL_SHADER_S3O_LAST, NULL);

	modelShaders[MODEL_SHADER_S3O_BASIC ] = sh->CreateProgramObject("[UnitDrawer]", "S3OShaderDefARB", true);
	modelShaders[MODEL_SHADER_S3O_SHADOW] = modelShaders[MODEL_SHADER_S3O_BASIC];
	modelShaders[MODEL_SHADER_S3O_ACTIVE] = modelShaders[MODEL_SHADER_S3O_BASIC];

	// with advFade, submerged transparent objects are clipped against GL_CLIP_PLANE3
	const char* vertexProgNameARB = (advFade)? "ARB/units3o2.vp": "ARB/units3o.vp";
	const std::string extraDefs =
		("#define BASE_DYNAMIC_MODEL_LIGHT " + IntToString(lightHandler.GetBaseLight()) + "\n") +
		("#define MAX_DYNAMIC_MODEL_LIGHTS " + IntToString(lightHandler.GetMaxLights()) + "\n");

	modelShaders[MODEL_SHADER_S3O_BASIC]->AttachShaderObject(sh->CreateShaderObject(vertexProgNameARB, "", GL_VERTEX_PROGRAM_ARB));
	modelShaders[MODEL_SHADER_S3O_BASIC]->AttachShaderObject(sh->CreateShaderObject("ARB/units3o.fp", "", GL_FRAGMENT_PROGRAM_ARB));
	modelShaders[MODEL_SHADER_S3O_BASIC]->Link();

	if (!globalRendering->haveGLSL) {
		modelShaders[MODEL_SHADER_S3O_SHADOW] = sh->CreateProgramObject("[UnitDrawer]", "S3OShaderAdvARB", true);
		modelShaders[MODEL_SHADER_S3O_SHADOW]->AttachShaderObject(sh->CreateShaderObject(vertexProgNameARB, "", GL_VERTEX_PROGRAM_ARB));
		modelShaders[MODEL_SHADER_S3O_SHADOW]->AttachShaderObject(sh->CreateShaderObject("ARB/units3o_shadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
		modelShaders[MODEL_SHADER_S3O_SHADOW]->Link();
	} else {
		modelShaders[MODEL_SHADER_S3O_SHADOW] = sh->CreateProgramObject("[UnitDrawer]", "S3OShaderAdvGLSL", false);
		modelShaders[MODEL_SHADER_S3O_SHADOW]->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelVertProg.glsl", extraDefs, GL_VERTEX_SHADER));
		modelShaders[MODEL_SHADER_S3O_SHADOW]->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelFragProg.glsl", extraDefs, GL_FRAGMENT_SHADER));
		modelShaders[MODEL_SHADER_S3O_SHADOW]->Link();
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("diffuseTex");        // idx  0 (t1: diffuse + team-color)
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("shadingTex");        // idx  1 (t2: spec/refl + self-illum)
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("shadowTex");         // idx  2
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("reflectTex");        // idx  3 (cube)
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("specularTex");       // idx  4 (cube)
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("sunDir");            // idx  5
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("cameraPos");         // idx  6
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("cameraMat");         // idx  7
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("cameraMatInv");      // idx  8
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("teamColor");         // idx  9
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("sunAmbient");        // idx 10
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("sunDiffuse");        // idx 11
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("shadowDensity");     // idx 12
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("shadowMatrix");      // idx 13
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("shadowParams");      // idx 14
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniformLocation("numModelDynLights"); // idx 15

		modelShaders[MODEL_SHADER_S3O_SHADOW]->Enable();
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniform1i(0, 0); // diffuseTex  (idx 0, texunit 0)
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniform1i(1, 1); // shadingTex  (idx 1, texunit 1)
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniform1i(2, 2); // shadowTex   (idx 2, texunit 2)
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniform1i(3, 3); // reflectTex  (idx 3, texunit 3)
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniform1i(4, 4); // specularTex (idx 4, texunit 4)
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniform3fv(5, &sky->GetLight()->GetLightDir().x);
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniform3fv(10, &unitAmbientColor[0]);
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniform3fv(11, &unitSunColor[0]);
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniform1f(12, sky->GetLight()->GetUnitShadowDensity());
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniform1i(15, 0); // numModelDynLights
		modelShaders[MODEL_SHADER_S3O_SHADOW]->Disable();
		modelShaders[MODEL_SHADER_S3O_SHADOW]->Validate();
	}

	if (shadowHandler->shadowsLoaded)
		modelShaders[MODEL_SHADER_S3O_ACTIVE] = modelShaders[MODEL_SHADER_S3O_SHADOW];

	#undef sh
	return true;
}



void CUnitDrawer::SetUnitDrawDist(float dist)
{
	unitDrawDist = dist;
	unitDrawDistSqr = dist * dist;
}

void CUnitDrawer::SetUnitIconDist(float dist)
{
	unitIconDist = dist;
	iconLength = 750 * unitIconDist * unitIconDist;
}

void CUnitDrawer::Update()
{
	{
		GML_STDMUTEX_LOCK(temp); // Update

		while (!tempDrawUnits.empty() && tempDrawUnits.begin()->first < gs->frameNum - 1) {
			tempDrawUnits.erase(tempDrawUnits.begin());
		}
		while (!tempTransparentDrawUnits.empty() && tempTransparentDrawUnits.begin()->first <= gs->frameNum) {
			tempTransparentDrawUnits.erase(tempTransparentDrawUnits.begin());
		}
	}

	eventHandler.UpdateDrawUnits();

	{
		GML_RECMUTEX_LOCK(unit); // Update

		drawIcon.clear();
#ifdef USE_GML
		drawStat.clear();
#endif
		for (std::set<CUnit*>::iterator usi = unsortedUnits.begin(); usi != unsortedUnits.end(); ++usi) {
			CUnit* unit = *usi;

			UpdateUnitIconState(unit);
			UpdateUnitDrawPos(unit);
		}
	}

	useDistToGroundForIcons = (camHandler->GetCurrentController()).GetUseDistToGroundForIcons();

	if (useDistToGroundForIcons) {
		const float3& camPos = camera->pos;
		// use the height at the current camera position
		//const float groundHeight = ground->GetHeightAboveWater(camPos.x, camPos.z, false);
		// use the middle between the highest and lowest position on the map as average
		const float groundHeight = (readmap->currMinHeight + readmap->currMaxHeight) / 2;
		const float overGround = camPos.y - groundHeight;

		sqCamDistToGroundForIcons = overGround * overGround;
	}
}






//! only called by DrawOpaqueUnit
inline bool CUnitDrawer::DrawUnitLOD(CUnit* unit)
{
	GML_LODMUTEX_LOCK(unit); // DrawUnitLOD

	if (unit->lodCount > 0) {
		if (unit->isCloaked) {
			const LuaMatType matType = (water->IsDrawReflection())?
				LUAMAT_ALPHA_REFLECT: LUAMAT_ALPHA;
			LuaUnitMaterial& unitMat = unit->luaMats[matType];
			const unsigned lod = CalcUnitLOD(unit, unitMat.GetLastLOD());
			unit->currentLOD = lod;
			LuaUnitLODMaterial* lodMat = unitMat.GetMaterial(lod);

			if ((lodMat != NULL) && lodMat->IsActive()) {
				lodMat->AddUnit(unit); return true;
			}
		} else {
			const LuaMatType matType =
				(water->IsDrawReflection()) ? LUAMAT_OPAQUE_REFLECT : LUAMAT_OPAQUE;
			LuaUnitMaterial& unitMat = unit->luaMats[matType];
			const unsigned lod = CalcUnitLOD(unit, unitMat.GetLastLOD());
			unit->currentLOD = lod;
			LuaUnitLODMaterial* lodMat = unitMat.GetMaterial(lod);

			if ((lodMat != NULL) && lodMat->IsActive()) {
				lodMat->AddUnit(unit); return true;
			}
		}
	}

	return false;
}

inline void CUnitDrawer::DrawOpaqueUnit(CUnit* unit, const CUnit* excludeUnit, bool drawReflection, bool drawRefraction)
{
	if (unit == excludeUnit) {
		return;
	}
	if (unit->noDraw) {
		return;
	}
	if (!camera->InView(unit->drawMidPos, unit->drawRadius)) {
		return;
	}

	if ((unit->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectatingFullView) {
		if (drawReflection) {
			float3 zeroPos;

			if (unit->drawMidPos.y < 0.0f) {
				zeroPos = unit->drawMidPos;
			} else {
				const float dif = unit->drawMidPos.y - camera->pos.y;
				zeroPos =
					camera->pos  * (unit->drawMidPos.y / dif) +
					unit->drawMidPos * (-camera->pos.y / dif);
			}
			if (ground->GetApproximateHeight(zeroPos.x, zeroPos.z, false) > unit->drawRadius) {
				return;
			}
		}
		else if (drawRefraction) {
			if (unit->pos.y > 0.0f) {
				return;
			}
		}
#ifdef USE_GML
		else
			unit->lastDrawFrame = gs->frameNum;
#endif

		if (!unit->isIcon) {
			if ((unit->pos).SqDistance(camera->pos) > (unit->sqRadius * unitDrawDistSqr)) {
				farTextureHandler->Queue(unit);
			} else {
				if (!DrawUnitLOD(unit)) {
					SetTeamColour(unit->team);
					DrawUnitNow(unit);
				}
			}
		}
	}
}



void CUnitDrawer::Draw(bool drawReflection, bool drawRefraction)
{
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	ISky::SetupFog();

	if (drawReflection) {
		SetUnitGlobalLODFactor(LODScale * LODScaleReflection);
	} else if (drawRefraction) {
		SetUnitGlobalLODFactor(LODScale * LODScaleRefraction);
	} else {
		SetUnitGlobalLODFactor(LODScale);
	}

	camNorm = camera->forward;
	camNorm.y = -0.1f;
	camNorm.ANormalize();

	const CPlayer* myPlayer = gu->GetMyPlayer();
	const CUnit* excludeUnit = drawReflection? NULL: myPlayer->fpsController.GetControllee();

	SetupForUnitDrawing();

	// lock on the bins
	GML_RECMUTEX_LOCK(unit); // Draw

#ifdef USE_GML
	mt_drawReflection = drawReflection; // these member vars will be accessed by DrawOpaqueUnitMT
	mt_drawRefraction = drawRefraction;
	mt_excludeUnit = excludeUnit;
#endif
	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		opaqueModelRenderers[modelType]->PushRenderState();
		DrawOpaqueUnits(modelType, excludeUnit, drawReflection, drawRefraction);
		opaqueModelRenderers[modelType]->PopRenderState();
	}

	CleanUpUnitDrawing();

	DrawOpaqueShaderUnits();
	farTextureHandler->Draw();
	DrawUnitIcons(drawReflection);

	glDisable(GL_FOG);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
}



void CUnitDrawer::DrawOpaqueUnits(int modelType, const CUnit* excludeUnit, bool drawReflection, bool drawRefraction)
{
	typedef std::set<CUnit*> UnitSet;
	typedef std::map<int, UnitSet> UnitBin;

	const UnitBin& unitBin = opaqueModelRenderers[modelType]->GetUnitBin();

	UnitBin::const_iterator unitBinIt;
	UnitSet::const_iterator unitSetIt;

	for (unitBinIt = unitBin.begin(); unitBinIt != unitBin.end(); ++unitBinIt) {
		if (modelType != MODELTYPE_3DO) {
			texturehandlerS3O->SetS3oTexture(unitBinIt->first);
		}

		const UnitSet& unitSet = unitBinIt->second;
#ifdef USE_GML
		bool mt = GML_PROFILER(multiThreadDrawUnit)
		if (mt && unitSet.size() >= GML::ThreadCount() * 4) { // small unitSets will add a significant overhead
			gmlProcessor->Work( // Profiler results, 4 threads, one single large unitSet: Approximately 20% faster with multiThreadDrawUnit
				NULL, NULL, &CUnitDrawer::DrawOpaqueUnitMT, this, GML::ThreadCount(),
				FALSE, &unitSet, unitSet.size(), 50, 100, TRUE
			);
		}
		else
#endif
		{
			for (unitSetIt = unitSet.begin(); unitSetIt != unitSet.end(); ++unitSetIt) {
				DrawOpaqueUnit(*unitSetIt, excludeUnit, drawReflection, drawRefraction);
			}
		}
	}

	if (modelType == MODELTYPE_3DO) {
		DrawOpaqueAIUnits();
	}
}

void CUnitDrawer::DrawOpaqueAIUnits()
{
	GML_STDMUTEX_LOCK(temp); // DrawOpaqueAIUnits

	// non-cloaked AI unit ghosts (FIXME: s3o's + teamcolor)
	for (std::multimap<int, TempDrawUnit>::iterator ti = tempDrawUnits.begin(); ti != tempDrawUnits.end(); ++ti) {
		if (camera->InView(ti->second.pos, 100)) {
			glPushMatrix();
			glTranslatef3(ti->second.pos);
			glRotatef(ti->second.rotation * 180 / PI, 0, 1, 0);

			const UnitDef* udef = ti->second.unitdef;
			const S3DModel* model = udef->LoadModel();

			model->DrawStatic();
			glPopMatrix();
		}
	}
}

void CUnitDrawer::DrawUnitIcons(bool drawReflection)
{
	if (!drawReflection) {
		// Draw unit icons and radar blips.
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);

		for (std::set<CUnit*>::iterator ui = drawIcon.begin(); ui != drawIcon.end(); ++ui) {
			DrawIcon(*ui, false);
		}
		if (!gu->spectatingFullView) {
			for (std::set<CUnit*>::const_iterator ui = unitRadarIcons[gu->myAllyTeam].begin(); ui != unitRadarIcons[gu->myAllyTeam].end(); ++ui) {
				DrawIcon(*ui, ((*ui)->losStatus[gu->myAllyTeam] & (LOS_PREVLOS | LOS_CONTRADAR)) != (LOS_PREVLOS | LOS_CONTRADAR));
			}
		}

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);

#ifdef USE_GML
		for (std::set<CUnit*>::iterator ui = drawStat.begin(); ui != drawStat.end(); ++ui) {
			DrawUnitStats(*ui);
		}
#endif
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
}





/******************************************************************************/
/******************************************************************************/

static void DrawLuaMatBins(LuaMatType type)
{
	const LuaMatBinSet& bins = luaMatHandler.GetBins(type);
	if (bins.empty()) {
		return;
	}

	LUA_DRAWING = true;

	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_TRANSFORM_BIT);
	if (type == LUAMAT_ALPHA || type == LUAMAT_ALPHA_REFLECT) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.1f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);
	}

	const LuaMaterial* currMat = &LuaMaterial::defMat;

	LuaMatBinSet::const_iterator it;
	for (it = bins.begin(); it != bins.end(); ++it) {
		LuaMatBin* bin = *it;
		bin->Execute(*currMat);
		currMat = bin;

		const GML_VECTOR<CUnit*>& units = bin->GetUnits();
		const int count = (int)units.size();
		for (int i = 0; i < count; i++) {
			CUnit* unit = units[i];

			GML_LODMUTEX_LOCK(unit); // DrawLuaMatBins

			const LuaUnitMaterial& unitMat = unit->luaMats[type];
			const LuaUnitLODMaterial* lodMat = unitMat.GetMaterial(unit->currentLOD);
#ifdef USE_GML
			if (!lodMat || !lodMat->IsActive())
				continue;
#endif
			lodMat->uniforms.Execute(unit);

			unitDrawer->SetTeamColour(unit->team);
			unitDrawer->DrawUnitWithLists(unit, lodMat->preDisplayList, lodMat->postDisplayList);
		}
	}

	LuaMaterial::defMat.Execute(*currMat);
	luaMatHandler.ClearBins(type);

	glPopAttrib();

	LUA_DRAWING = false;
}


/******************************************************************************/

static void SetupShadowDrawing()
{
	//FIXME setup face culling for s3o?

	glColor3f(1.0f, 1.0f, 1.0f);
	glDisable(GL_TEXTURE_2D);

	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);

	Shader::IProgramObject* po =
		shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);
	po->Enable();
}


static void CleanUpShadowDrawing()
{
	Shader::IProgramObject* po =
		shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);

	po->Disable();
	glDisable(GL_POLYGON_OFFSET_FILL);
}


/******************************************************************************/

#define ud unitDrawer
static void SetupOpaque3DO() {
	ud->SetupForUnitDrawing();
	ud->GetOpaqueModelRenderer(MODELTYPE_3DO)->PushRenderState();
}
static void ResetOpaque3DO() {
	ud->GetOpaqueModelRenderer(MODELTYPE_3DO)->PopRenderState();
	ud->CleanUpUnitDrawing();
}
static void SetupOpaqueS3O() {
	ud->SetupForUnitDrawing();
	ud->GetOpaqueModelRenderer(MODELTYPE_S3O)->PushRenderState();
}
static void ResetOpaqueS3O() {
	ud->GetOpaqueModelRenderer(MODELTYPE_S3O)->PopRenderState();
	ud->CleanUpUnitDrawing();
}

static void SetupAlpha3DO() {
	ud->SetupForGhostDrawing();
	ud->GetCloakedModelRenderer(MODELTYPE_3DO)->PushRenderState();
}
static void ResetAlpha3DO() {
	ud->GetCloakedModelRenderer(MODELTYPE_3DO)->PopRenderState();
	ud->CleanUpGhostDrawing();
}
static void SetupAlphaS3O() {
	ud->SetupForGhostDrawing();
	ud->GetCloakedModelRenderer(MODELTYPE_S3O)->PushRenderState();
}
static void ResetAlphaS3O() {
	ud->GetCloakedModelRenderer(MODELTYPE_S3O)->PopRenderState();
	ud->CleanUpGhostDrawing();
}
#undef ud

static void SetupShadow3DO() { SetupShadowDrawing();   }
static void ResetShadow3DO() { CleanUpShadowDrawing(); }
static void SetupShadowS3O() { SetupShadowDrawing();   }
static void ResetShadowS3O() { CleanUpShadowDrawing(); }


/******************************************************************************/

void CUnitDrawer::DrawOpaqueShaderUnits()
{
	luaMatHandler.setup3doShader = SetupOpaque3DO;
	luaMatHandler.reset3doShader = ResetOpaque3DO;
	luaMatHandler.setupS3oShader = SetupOpaqueS3O;
	luaMatHandler.resetS3oShader = ResetOpaqueS3O;

	const LuaMatType matType = (water->IsDrawReflection())?
		LUAMAT_OPAQUE_REFLECT:
		LUAMAT_OPAQUE;

	DrawLuaMatBins(matType);
}


void CUnitDrawer::DrawCloakedShaderUnits()
{
	luaMatHandler.setup3doShader = SetupAlpha3DO;
	luaMatHandler.reset3doShader = ResetAlpha3DO;
	luaMatHandler.setupS3oShader = SetupAlphaS3O;
	luaMatHandler.resetS3oShader = ResetAlphaS3O;

	const LuaMatType matType = (water->IsDrawReflection())?
		LUAMAT_ALPHA_REFLECT:
		LUAMAT_ALPHA;

	DrawLuaMatBins(matType);
}


void CUnitDrawer::DrawShadowShaderUnits()
{
	luaMatHandler.setup3doShader = SetupShadow3DO;
	luaMatHandler.reset3doShader = ResetShadow3DO;
	luaMatHandler.setupS3oShader = SetupShadowS3O;
	luaMatHandler.resetS3oShader = ResetShadowS3O;

	DrawLuaMatBins(LUAMAT_SHADOW);
}



/******************************************************************************/
/******************************************************************************/


inline void CUnitDrawer::DrawOpaqueUnitShadow(CUnit* unit) {
	// do shadow alpha-masking for S3O units only
	// (3DO's need more setup than it is worth)
	#ifdef UNIT_SHADOW_ALPHA_MASKING
		#define S3O_TEX(model) \
			texturehandlerS3O->GetS3oTex(model->textureType)
		#define PUSH_SHADOW_TEXTURE_STATE(model)                      \
			if (model->type != MODELTYPE_3DO) {                       \
				glActiveTexture(GL_TEXTURE0);                         \
				glEnable(GL_TEXTURE_2D);                              \
				glBindTexture(GL_TEXTURE_2D, S3O_TEX(model)->tex2);   \
			}
		#define POP_SHADOW_TEXTURE_STATE(model)   \
			if (model->type != MODELTYPE_3DO) {   \
				glBindTexture(GL_TEXTURE_2D, 0);  \
				glDisable(GL_TEXTURE_2D);         \
				glActiveTexture(GL_TEXTURE0);     \
			}
	#else
		#define PUSH_SHADOW_TEXTURE_STATE(model)
		#define POP_SHADOW_TEXTURE_STATE(model)
	#endif

	const bool unitInLOS = ((unit->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectatingFullView);

	// FIXME: test against the shadow projection intersection
	if (!(unitInLOS && camera->InView(unit->drawMidPos, unit->drawRadius + 700.0f))) {
		return;
	}

	const float sqDist = (unit->pos - camera->pos).SqLength();
	const float farLength = unit->sqRadius * unitDrawDistSqr;

	if (unit->noDraw) { return; }
	if (sqDist >= farLength) { return; }
	if (unit->isCloaked) { return; }
	if (DrawAsIcon(unit, sqDist)) { return; }

	GML_LODMUTEX_LOCK(unit); // DrawOpaqueUnitShadow

	if (unit->lodCount <= 0) {
		PUSH_SHADOW_TEXTURE_STATE(unit->model);
		DrawUnitNow(unit);
		POP_SHADOW_TEXTURE_STATE(unit->model);
	} else {
		LuaUnitMaterial& unitMat = unit->luaMats[LUAMAT_SHADOW];
		const unsigned lod = CalcUnitLOD(unit, unitMat.GetLastLOD());
		unit->currentLOD = lod;
		LuaUnitLODMaterial* lodMat = unitMat.GetMaterial(lod);

		if ((lodMat != NULL) && lodMat->IsActive()) {
			lodMat->AddUnit(unit);
		} else {
			PUSH_SHADOW_TEXTURE_STATE(unit->model);
			DrawUnitNow(unit);
			POP_SHADOW_TEXTURE_STATE(unit->model);
		}
	}

	#undef PUSH_SHADOW_TEXTURE_STATE
	#undef POP_SHADOW_TEXTURE_STATE
}
void CUnitDrawer::DrawOpaqueUnitsShadow(int modelType) {
	typedef std::set<CUnit*> UnitSet;
	typedef std::map<int, UnitSet> UnitBin;

	const UnitBin& unitBin = opaqueModelRenderers[modelType]->GetUnitBin();

	UnitBin::const_iterator unitBinIt;
	UnitSet::const_iterator unitSetIt;

	for (unitBinIt = unitBin.begin(); unitBinIt != unitBin.end(); ++unitBinIt) {
		const UnitSet& unitSet = unitBinIt->second;

#ifdef USE_GML
		bool mt = GML_PROFILER(multiThreadDrawUnitShadow)
		if (mt && unitSet.size() >= GML::ThreadCount() * 4) { // small unitSets will add a significant overhead
			gmlProcessor->Work( // Profiler results, 4 threads, one single large unitSet: Approximately 20% faster with multiThreadDrawUnitShadow
				NULL, NULL, &CUnitDrawer::DrawOpaqueUnitShadowMT, this, GML::ThreadCount(),
				FALSE, &unitSet, unitSet.size(), 50, 100, TRUE
			);
		}
		else
#endif
		{
			for (unitSetIt = unitSet.begin(); unitSetIt != unitSet.end(); ++unitSetIt) {
				DrawOpaqueUnitShadow(*unitSetIt);
			}
		}
	}
}

void CUnitDrawer::DrawShadowPass()
{
	glColor3f(1.0f, 1.0f, 1.0f);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);

	#ifdef UNIT_SHADOW_ALPHA_MASKING
	glAlphaFunc(GL_GREATER, 0.5f);
	glEnable(GL_ALPHA_TEST);
	#endif

	Shader::IProgramObject* po =
		shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);
	po->Enable();

	SetUnitGlobalLODFactor(LODScale * LODScaleShadow);

	GML_RECMUTEX_LOCK(unit); // DrawShadowPass

	{
		// 3DO's have clockwise-wound faces and
		// (usually) holes, so disable backface
		// culling for them
		glDisable(GL_CULL_FACE);
		DrawOpaqueUnitsShadow(MODELTYPE_3DO);
		glEnable(GL_CULL_FACE);

		for (int modelType = MODELTYPE_S3O; modelType < MODELTYPE_OTHER; modelType++) {
			// note: just use DrawOpaqueUnits()? would
			// save texture switches needed anyway for
			// UNIT_SHADOW_ALPHA_MASKING
			DrawOpaqueUnitsShadow(modelType);
		}
	}

	po->Disable();

	#ifdef UNIT_SHADOW_ALPHA_MASKING
	glDisable(GL_ALPHA_TEST);
	#endif

	glDisable(GL_POLYGON_OFFSET_FILL);

	DrawShadowShaderUnits();
}



void CUnitDrawer::DrawIcon(CUnit* unit, bool useDefaultIcon)
{
	// If the icon is to be drawn as a radar blip, we want to get the default icon.
	const icon::CIconData* iconData = NULL;

	if (useDefaultIcon) {
		iconData = icon::iconHandler->GetDefaultIconData();
	} else {
		iconData = unit->unitDef->iconType.GetIconData();
	}

	// Calculate the icon size. It scales with:
	//  * The square root of the camera distance.
	//  * The mod defined 'iconSize' (which acts a multiplier).
	//  * The unit radius, depending on whether the mod defined 'radiusadjust' is true or false.
	float3 pos;
	if (gu->spectatingFullView) {
		pos = unit->drawMidPos;
	} else {
		pos = CGameHelper::GetUnitErrorPos(unit, gu->myAllyTeam);
	}

	float dist = fastmath::sqrt2(fastmath::sqrt2((pos - camera->pos).SqLength()));
	float scale = 0.4f * iconData->GetSize() * dist;

	if (iconData->GetRadiusAdjust() && !useDefaultIcon) {
		// I take the standard unit radius to be 30
		// ... call it an educated guess. (Teake Nutma)
		scale *= (unit->radius / 30);
	}

	unit->iconRadius = scale; // store the icon size so that we don't have to calculate it again

	// Is the unit selected? Then draw it white.
	if (unit->isSelected) {
		glColor3ub(255, 255, 255);
	} else {
		glColor3ubv(teamHandler->Team(unit->team)->color);
	}

	// If the icon is partly under the ground, move it up.
	const float h = ground->GetHeightAboveWater(pos.x, pos.z, false);
	if (pos.y < (h + scale)) {
		pos.y = (h + scale);
	}

	// calculate the vertices
	const float3 dy = camera->up    * scale;
	const float3 dx = camera->right * scale;
	const float3 vn = pos - dx;
	const float3 vp = pos + dx;
	const float3 vnn = vn - dy;
	const float3 vpn = vp - dy;
	const float3 vnp = vn + dy;
	const float3 vpp = vp + dy;

	// Draw the icon.
	iconData->Draw(vnn, vpn, vnp, vpp);
}






void CUnitDrawer::SetupForGhostDrawing() const
{
	glEnable(GL_LIGHTING); // Give faded objects same appearance as regular
	glLightfv(GL_LIGHT1, GL_POSITION, sky->GetLight()->GetLightDir());
	glEnable(GL_LIGHT1);

	SetupBasicS3OTexture0();
	SetupBasicS3OTexture1(); // This also sets up the transparency

	static const float cols[] = {1.0f, 1.0f, 1.0f, 1.0f};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cols);
	glColor3f(1.0f, 1.0f, 1.0f);

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);
	glDepthMask(GL_FALSE);
}

void CUnitDrawer::CleanUpGhostDrawing() const
{
	glPopAttrib();
	glDisable(GL_TEXTURE_2D);

	// clean up s3o drawing stuff
	// reset texture1 state
	CleanupBasicS3OTexture1();

	// reset texture0 state
	CleanupBasicS3OTexture0();

	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT1);
}



void CUnitDrawer::DrawCloakedUnits(bool disableAdvShading)
{
	const bool oldAdvShading = advShading;
	// don't use shaders if shadows are enabled
	advShading = advShading && !disableAdvShading;

	if (advShading) {
		SetupForUnitDrawing();
		glDisable(GL_ALPHA_TEST);
	} else {
		SetupForGhostDrawing();
	}

	glColor4f(1.0f, 1.0f, 1.0f, cloakAlpha);

	{
		GML_RECMUTEX_LOCK(unit); // DrawCloakedUnits

		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
			cloakedModelRenderers[modelType]->PushRenderState();
			DrawCloakedUnitsHelper(modelType);
			cloakedModelRenderers[modelType]->PopRenderState();
		}

		if (advShading) {
			CleanUpUnitDrawing();
			glEnable(GL_ALPHA_TEST);
		} else {
			CleanUpGhostDrawing();
		}

		advShading = oldAdvShading;

		// shader rendering
		DrawCloakedShaderUnits();
	}

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void CUnitDrawer::DrawCloakedUnitsHelper(int modelType)
{
	if (modelType == MODELTYPE_3DO) {
		DrawCloakedAIUnits();
	}

	{
		typedef std::set<CUnit*> UnitSet;
		typedef std::map<int, UnitSet> UnitRenderBin;
		typedef std::map<int, UnitSet>::const_iterator UnitRenderBinIt;

		const UnitRenderBin& unitBin = cloakedModelRenderers[modelType]->GetUnitBin();

		// cloaked units
		for (UnitRenderBinIt it = unitBin.begin(); it != unitBin.end(); ++it) {
			if (modelType != MODELTYPE_3DO) {
				texturehandlerS3O->SetS3oTexture(it->first);
			}

			const UnitSet& unitSet = it->second;

			for (UnitSet::const_iterator ui = unitSet.begin(); ui != unitSet.end(); ++ui) {
				DrawCloakedUnit(*ui, modelType, false);
			}
		}
	}

	// living and dead ghosted buildings
	DrawGhostedBuildings(modelType);
}

inline void CUnitDrawer::DrawCloakedUnit(CUnit* unit, int modelType, bool drawGhostBuildingsPass) {
	if (!camera->InView(unit->drawMidPos, unit->drawRadius)) {
		return;
	}

	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];

	if (!drawGhostBuildingsPass) {
		if ((losStatus & LOS_INLOS) || gu->spectatingFullView) {
			if (!unit->isIcon) {
				SetTeamColour(unit->team, cloakAlpha);
				DrawUnitNow(unit);
			}
		}
	} else {
		// check for decoy models
		const UnitDef* decoyDef = unit->unitDef->decoyDef;
		const S3DModel* model = NULL;

		if (decoyDef == NULL) {
			model = unit->model;
		} else {
			model = decoyDef->LoadModel();
		}

		// FIXME: needs a second pass
		if (model->type != modelType) {
			return;
		}

		// ghosted enemy units
		if (losStatus & LOS_CONTRADAR) {
			glColor4f(0.9f, 0.9f, 0.9f, cloakAlpha2);
		} else {
			glColor4f(0.6f, 0.6f, 0.6f, cloakAlpha1);
		}

		glPushMatrix();
		glTranslatef3(unit->pos);
		glRotatef(unit->buildFacing * 90.0f, 0, 1, 0);

		if (modelType != MODELTYPE_3DO) {
			// the units in liveGhostedBuildings[modelType] are not
			// sorted by textureType, but we cannot merge them with
			// cloakedModelRenderers[modelType] since they are not
			// actually cloaked
			texturehandlerS3O->SetS3oTexture(model->textureType);
		}

		SetTeamColour(unit->team, (losStatus & LOS_CONTRADAR) ? cloakAlpha2 : cloakAlpha1);
		model->DrawStatic();
		glPopMatrix();

		glColor4f(1.0f, 1.0f, 1.0f, cloakAlpha);
	}
}

void CUnitDrawer::DrawCloakedAIUnits()
{
	GML_STDMUTEX_LOCK(temp); // DrawCloakedAIUnits

	// cloaked AI unit ghosts (FIXME: S3O's need different state)
	for (std::multimap<int, TempDrawUnit>::iterator ti = tempTransparentDrawUnits.begin(); ti != tempTransparentDrawUnits.end(); ++ti) {
		if (camera->InView(ti->second.pos, 100)) {
			glPushMatrix();
			glTranslatef3(ti->second.pos);
			glRotatef(ti->second.rotation * 180 / PI, 0, 1, 0);

			const UnitDef* udef = ti->second.unitdef;
			const S3DModel* model = udef->LoadModel();

			SetTeamColour(ti->second.team, cloakAlpha);

			model->DrawStatic();
			glPopMatrix();
		}
		if (ti->second.drawBorder) {
			float3 pos = ti->second.pos;
			const UnitDef* unitdef = ti->second.unitdef;

			SetTeamColour(ti->second.team, cloakAlpha3);

			BuildInfo bi(unitdef, pos, ti->second.facing);
			pos = CGameHelper::Pos2BuildPos(bi, false);

			const float xsize = bi.GetXSize() * 4;
			const float zsize = bi.GetZSize() * 4;

			glColor4f(0.2f, 1, 0.2f, cloakAlpha3);
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_LINE_STRIP);
				glVertexf3(pos + float3( xsize, 1.0f,  zsize));
				glVertexf3(pos + float3(-xsize, 1.0f,  zsize));
				glVertexf3(pos + float3(-xsize, 1.0f, -zsize));
				glVertexf3(pos + float3( xsize, 1.0f, -zsize));
				glVertexf3(pos + float3( xsize, 1.0f,  zsize));
			glEnd();
			glColor4f(1.0f, 1.0f, 1.0f, cloakAlpha);
			glEnable(GL_TEXTURE_2D);
		}
	}
}

void CUnitDrawer::DrawGhostedBuildings(int modelType)
{
	std::set<GhostSolidObject*>& deadGhostedBuildings = deadGhostBuildings[modelType];
	std::set<CUnit*>& liveGhostedBuildings = liveGhostBuildings[modelType];

	glColor4f(0.6f, 0.6f, 0.6f, cloakAlpha1);

	// buildings that died while ghosted
	for (std::set<GhostSolidObject*>::iterator it = deadGhostedBuildings.begin(); it != deadGhostedBuildings.end(); ) {
		if (loshandler->InLos((*it)->pos, gu->myAllyTeam) || gu->spectatingFullView) {
			// obtained LOS on the ghost of a dead building
			groundDecals->GhostDestroyed(*it);

			delete *it;
			it = set_erase(deadGhostedBuildings, it);
		} else {
			if (camera->InView((*it)->pos, (*it)->model->drawRadius)) {
				glPushMatrix();
				glTranslatef3((*it)->pos);
				glRotatef((*it)->facing * 90.0f, 0, 1, 0);

				if (modelType != MODELTYPE_3DO)
					texturehandlerS3O->SetS3oTexture((*it)->model->textureType);

				SetTeamColour((*it)->team, cloakAlpha1);
				(*it)->model->DrawStatic();
				glPopMatrix();
			}

			++it;
		}
	}

	if (!gu->spectatingFullView) {
		for (std::set<CUnit*>::const_iterator ui = liveGhostedBuildings.begin(); ui != liveGhostedBuildings.end(); ++ui) {
			if (!((*ui)->losStatus[gu->myAllyTeam] & LOS_INLOS)) // because of team switching via cheat, ghost buildings can exist for units in LOS
				DrawCloakedUnit(*ui, modelType, true);
		}
	}
}





void CUnitDrawer::SetupForUnitDrawing()
{
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	glAlphaFunc(GL_GREATER, 0.5f);
	glEnable(GL_ALPHA_TEST);

	if (advShading && !water->IsDrawReflection()) {
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glMultMatrixf(camera->GetViewMatrix());
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		// ARB standard does not seem to support
		// vertex program + clipplanes (used for
		// reflective pass) at once ==> not true,
		// but needs option ARB_position_invariant
		modelShaders[MODEL_SHADER_S3O_ACTIVE] = (shadowHandler->shadowsLoaded)?
			modelShaders[MODEL_SHADER_S3O_SHADOW]:
			modelShaders[MODEL_SHADER_S3O_BASIC];
		modelShaders[MODEL_SHADER_S3O_ACTIVE]->Enable();

		if (globalRendering->haveGLSL && shadowHandler->shadowsLoaded) {
			modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniform3fv(6, &camera->pos[0]);
			modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniformMatrix4fv(7, false, camera->GetViewMatrix());
			modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniformMatrix4fv(8, false, camera->GetViewMatrixInverse());
			modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniformMatrix4fv(13, false, shadowHandler->shadowMatrix);
			modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniform4fv(14, &(shadowHandler->GetShadowParams().x));

			lightHandler.Update(modelShaders[MODEL_SHADER_S3O_ACTIVE]);
		} else {
			modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
			modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniform4fv(10, &sky->GetLight()->GetLightDir().x);
			modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniform4f(11, unitSunColor.x, unitSunColor.y, unitSunColor.z, 0.0f);
			modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniform4f(12, unitAmbientColor.x, unitAmbientColor.y, unitAmbientColor.z, 1.0f); //!
			modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniform4f(13, camera->pos.x, camera->pos.y, camera->pos.z, 0.0f);
			modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
			modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniform4f(10, 0.0f, 0.0f, 0.0f, sky->GetLight()->GetUnitShadowDensity());
			modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniform4f(11, unitAmbientColor.x, unitAmbientColor.y, unitAmbientColor.z, 1.0f);

			glMatrixMode(GL_MATRIX0_ARB);
			glLoadMatrixf(shadowHandler->shadowMatrix);
			glMatrixMode(GL_MODELVIEW);
		}

		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);

		if (shadowHandler->shadowsLoaded) {
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
			glEnable(GL_TEXTURE_2D);
		}

		glActiveTexture(GL_TEXTURE3);
		glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cubeMapHandler->GetEnvReflectionTextureID());

		glActiveTexture(GL_TEXTURE4);
		glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cubeMapHandler->GetSpecularTextureID());

		glActiveTexture(GL_TEXTURE0);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		glEnable(GL_LIGHTING);
		glLightfv(GL_LIGHT1, GL_POSITION, sky->GetLight()->GetLightDir());
		glEnable(GL_LIGHT1);

		SetupBasicS3OTexture1();
		SetupBasicS3OTexture0();

		// Set material color
		static const float cols[] = {1.0f, 1.0f, 1.0, 1.0f};
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cols);
		glColor4fv(cols);
	}
}

void CUnitDrawer::CleanUpUnitDrawing() const
{
	glDisable(GL_CULL_FACE);
	glDisable(GL_ALPHA_TEST);

	if (advShading && !water->IsDrawReflection()) {
		modelShaders[MODEL_SHADER_S3O_ACTIVE]->Disable();

		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE2);
		glDisable(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);

		glActiveTexture(GL_TEXTURE3);
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);

		glActiveTexture(GL_TEXTURE4);
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);

		glActiveTexture(GL_TEXTURE0);

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	} else {
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT1);

		CleanupBasicS3OTexture1();
		CleanupBasicS3OTexture0();
	}
}



void CUnitDrawer::SetTeamColour(int team, float alpha) const
{
	if (advShading && !water->IsDrawReflection()) {
		const CTeam* t = teamHandler->Team(team);
		const float4 c = float4(t->color[0] / 255.0f, t->color[1] / 255.0f, t->color[2] / 255.0f, alpha);

		if (modelShaders[MODEL_SHADER_S3O_ACTIVE]->IsBound()) {
			if (globalRendering->haveGLSL && shadowHandler->shadowsLoaded) {
				modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniform4fv(9, &c[0]);
			} else {
				modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
				modelShaders[MODEL_SHADER_S3O_ACTIVE]->SetUniform4fv(14, &c[0]);
			}
		}

		if (LUA_DRAWING) {// FIXME?
			SetBasicTeamColour(team, alpha);
		}
	} else {
		// non-shader case via texture combiners
		SetBasicTeamColour(team, alpha);
	}
}

void CUnitDrawer::SetBasicTeamColour(int team, float alpha)
{
	const unsigned char* col = teamHandler->Team(team)->color;
	const float texConstant[] = {col[0] / 255.0f, col[1] / 255.0f, col[2] / 255.0f, alpha};
	const float matConstant[] = {1.0f, 1.0f, 1.0f, alpha};

	glActiveTexture(GL_TEXTURE0);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, texConstant);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, matConstant);
}



/**
 * Set up the texture environment in texture unit 0
 * to give an S3O texture its team-colour.
 *
 * Also:
 * - call SetBasicTeamColour to set the team colour to transform to.
 * - Replace the output alpha channel. If not, only the team-coloured bits will show, if that. Or something.
 */
void CUnitDrawer::SetupBasicS3OTexture0()
{
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	// RGB = Texture * (1 - Alpha) + Teamcolor * Alpha
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

	// ALPHA = Ignore
}

/**
 * This sets the first texture unit to GL_MODULATE the colours from the
 * first texture unit with the current glColor.
 *
 * Normal S3O drawing sets the color to full white; translucencies
 * use this setup to 'tint' the drawn model.
 *
 * - Leaves glActivateTextureARB at the first unit.
 * - This doesn't tinker with the output alpha, either.
 */
void CUnitDrawer::SetupBasicS3OTexture1()
{
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);

	// RGB = Primary Color * Previous
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);

	// ALPHA = Current alpha * Alpha mask
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);
}


void CUnitDrawer::CleanupBasicS3OTexture1()
{
	// reset texture1 state
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
}

void CUnitDrawer::CleanupBasicS3OTexture0()
{
	// reset texture0 state
	glActiveTexture(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_RGB_ARB, GL_CONSTANT_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_RGB_ARB, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
}




/**
 * The companion to UnitDrawingTexturesOff(), re-enables the texture units
 * needed for drawing a model.
 *
 * Does *not* restore the texture bindings.
 */
void CUnitDrawer::UnitDrawingTexturesOn()
{
	// XXX FIXME GL_VERTEX_PROGRAM_ARB is very slow on ATIs here for some reason
	// if clip planes are enabled
	// check later after driver updates
	if (advShading && !water->IsDrawReflection()) {
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE3);
		glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		glActiveTexture(GL_TEXTURE4);
		glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		glActiveTexture(GL_TEXTURE0);
	} else {
		glEnable(GL_LIGHTING);
		glColor3f(1.0f, 1.0f, 1.0f);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
	}
}

/**
 * Between a pair of SetupFor/CleanUpUnitDrawing,
 * temporarily turns off textures and shaders.
 *
 * Used by CUnit::Draw() for drawing a unit under construction.
 *
 * Unfortunately, it doesn't work! With advanced shading on, the green
 * is darker than usual; with shadows as well, it's almost black. -- krudat
 */
void CUnitDrawer::UnitDrawingTexturesOff()
{
	/* If SetupForUnitDrawing is changed, this may need tweaking too. */
	if (advShading && !water->IsDrawReflection()) {
		glActiveTexture(GL_TEXTURE1); //! 'Shiny' texture.
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE2); //! Shadows.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE3); //! reflectionTex
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);
		glActiveTexture(GL_TEXTURE4); //! specularTex
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D); //! albedo + teamcolor
	} else {
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE1); //! GL lighting, I think.
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D); //! albedo + teamcolor
	}
}



/**
 * Draw one unit.
 *
 * Used for drawing the view of the controlled unit.
 *
 * Note: does all the GL state setting for that one unit, so you might want
 * something else for drawing many units.
 */
void CUnitDrawer::DrawIndividual(CUnit* unit)
{
	const bool origDebug = globalRendering->drawdebug;
	globalRendering->drawdebug = false;

	LuaUnitLODMaterial* lodMat = NULL;

	GML_LODMUTEX_LOCK(unit); // DrawIndividual

	if (unit->lodCount > 0) {
		const LuaMatType matType = (water->IsDrawReflection())?
			LUAMAT_OPAQUE_REFLECT : LUAMAT_OPAQUE;
		LuaUnitMaterial& unitMat = unit->luaMats[matType];
		lodMat = unitMat.GetMaterial(unit->currentLOD);
	}

	if (lodMat && lodMat->IsActive()) {
		SetUnitGlobalLODFactor(LODScale);

		luaMatHandler.setup3doShader = SetupOpaque3DO;
		luaMatHandler.reset3doShader = ResetOpaque3DO;
		luaMatHandler.setupS3oShader = SetupOpaqueS3O;
		luaMatHandler.resetS3oShader = ResetOpaqueS3O;

		const LuaMaterial& mat = (LuaMaterial&) *lodMat->matref.GetBin();

		mat.Execute(LuaMaterial::defMat);

		lodMat->uniforms.Execute(unit);

		SetTeamColour(unit->team);
		DrawUnitRawWithLists(unit, lodMat->preDisplayList, lodMat->postDisplayList);

		LuaMaterial::defMat.Execute(mat);
	} else {
		SetupForUnitDrawing();
		opaqueModelRenderers[MDL_TYPE(unit)]->PushRenderState();

		if (MDL_TYPE(unit) != MODELTYPE_3DO) {
			texturehandlerS3O->SetS3oTexture(TEX_TYPE(unit));
		}

		SetTeamColour(unit->team);
		DrawUnitRaw(unit);

		opaqueModelRenderers[MDL_TYPE(unit)]->PopRenderState();
		CleanUpUnitDrawing();
	}

	globalRendering->drawdebug = origDebug;
}


/**
 * Draw one unit,
 * - with depth-buffering(!) and lighting DISABLED,
 * - 'tinted' by the current glColor, *including* alpha.
 *
 * Used for drawing building orders.
 *
 * Note: does all the GL state setting for that one unit, so you might want
 * something else for drawing many translucent units.
 */
void CUnitDrawer::DrawBuildingSample(const UnitDef* unitdef, int side, float3 pos, int facing)
{
	const S3DModel* model = unitdef->LoadModel();

	/* From SetupForGhostDrawing. */
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT);

	SetBasicTeamColour(side);
	SetupBasicS3OTexture0();

	switch (model->type) {
		case MODELTYPE_3DO: {
			texturehandler3DO->Set3doAtlases();
		} break;
		case MODELTYPE_S3O:
		case MODELTYPE_OBJ:
		case MODELTYPE_ASS: {
			texturehandlerS3O->SetS3oTexture(model->textureType);
		} break;
		default: {
		} break;
	}

	SetupBasicS3OTexture1();

	/* Use the alpha given by glColor for the outgoing alpha.
	   (Might need to change this if we ever have transparent bits on units?)
	 */
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PRIMARY_COLOR_ARB);

	glActiveTexture(GL_TEXTURE0);

	/* From SetupForGhostDrawing. */
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE); /* Leave out face culling, as 3DO and 3DO translucents does. */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* Push out the polygons. */
	glPushMatrix();
	glTranslatef3(pos);
	glRotatef(facing * 90.0f, 0, 1, 0);

	model->DrawStatic();
	glPopMatrix();

	// reset texture1 state
	CleanupBasicS3OTexture1();

	/* Also reset the alpha generation. */
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);

	// reset texture0 state
	CleanupBasicS3OTexture0();

	/* From CleanUpGhostDrawing. */
	glPopAttrib();
	glDisable(GL_TEXTURE_2D);
	glDepthMask(GL_TRUE);
}

//! used by LuaOpenGL::DrawUnitShape only
void CUnitDrawer::DrawUnitDef(const UnitDef* unitDef, int team)
{
	const S3DModel* model = unitDef->LoadModel();

	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);

	// get the team-coloured texture constructed by texunit 0
	SetBasicTeamColour(team);
	SetupBasicS3OTexture0();

	switch (model->type) {
		case MODELTYPE_3DO: {
			texturehandler3DO->Set3doAtlases();
		} break;
		case MODELTYPE_S3O: {
			texturehandlerS3O->SetS3oTexture(model->textureType);
		} break;
		default: {
		} break;
	}

	// tint it with the current glColor in texunit 1
	SetupBasicS3OTexture1();

	// use the alpha given by glColor for the outgoing alpha.
	// (might need to change this if we ever have transparent bits on units?)
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PRIMARY_COLOR_ARB);

	glActiveTexture(GL_TEXTURE0);
	model->DrawStatic();

	// reset texture1 state
	CleanupBasicS3OTexture1();

	// also reset the alpha generation
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);

	// reset texture0 state
	CleanupBasicS3OTexture0();

	glPopAttrib();
}



void CUnitDrawer::DrawUnitBeingBuilt(CUnit* unit)
{
	if (shadowHandler->inShadowPass) {
		if (unit->buildProgress > (2.0f / 3.0f)) {
			DrawUnitModel(unit);
		}
		return;
	}

	const float start  = std::max(unit->model->mins.y, -unit->model->height);
	const float height = unit->model->height;

	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);

	const float col = math::fabs(128.0f - ((gs->frameNum * 4) & 255)) / 255.0f + 0.5f;
	const unsigned char* tcol = teamHandler->Team(unit->team)->color;
	// frame line-color
	const float3 fc = (!globalRendering->teamNanospray)?
		unit->unitDef->nanoColor:
		float3(tcol[0] / 255.0f, tcol[1] / 255.0f, tcol[2] / 255.0f);

	glColorf3(fc * col);

	//! render wireframe with FFP
	if (!LUA_DRAWING && advShading && !water->IsDrawReflection()) {
		modelShaders[MODEL_SHADER_S3O_ACTIVE]->Disable();
	}

	unitDrawer->UnitDrawingTexturesOff();

	// first stage: wireframe model
	//
	// Both clip planes move up. Clip plane 0 is the upper bound of the model,
	// clip plane 1 is the lower bound. In other words, clip plane 0 makes the
	// wireframe/flat color/texture appear, and clip plane 1 then erases the
	// wireframe/flat color laten on.

	const double plane0[4] = {0, -1, 0,  start + height * (unit->buildProgress *      3)};
	const double plane1[4] = {0,  1, 0, -start - height * (unit->buildProgress * 10 - 9)};
	glClipPlane(GL_CLIP_PLANE0, plane0);
	glClipPlane(GL_CLIP_PLANE1, plane1);

	if (!globalRendering->atiHacks) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		DrawUnitModel(unit);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	} else {
		//! some ATi mobility cards/drivers dont like clipping wireframes...
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		DrawUnitModel(unit);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glEnable(GL_CLIP_PLANE0);
		glEnable(GL_CLIP_PLANE1);
	}

	// Flat-colored model
	if (unit->buildProgress > (1.0f / 3.0f)) {
		glColorf3(fc * (1.5f - col));
		const double plane0[4] = {0, -1, 0,  start + height * (unit->buildProgress * 3 - 1)};
		const double plane1[4] = {0,  1, 0, -start - height * (unit->buildProgress * 3 - 2)};
		glClipPlane(GL_CLIP_PLANE0, plane0);
		glClipPlane(GL_CLIP_PLANE1, plane1);

		DrawUnitModel(unit);
	}

	glDisable(GL_CLIP_PLANE1);
	unitDrawer->UnitDrawingTexturesOn();

	if (!LUA_DRAWING && advShading && !water->IsDrawReflection()) {
		modelShaders[MODEL_SHADER_S3O_ACTIVE]->Enable();
	}

	// second stage: texture-mapped model
	//
	// XXX FIXME
	// ATI has issues with textures, clip planes and shader programs at once - very low performance
	// FIXME: This may work now I added OPTION ARB_position_invariant to the ARB programs.
	if (unit->buildProgress > (2.0f / 3.0f)) {
		if (globalRendering->atiHacks) {
			glDisable(GL_CLIP_PLANE0);

			glPolygonOffset(1.0f, 1.0f);
			glEnable(GL_POLYGON_OFFSET_FILL);
				DrawUnitModel(unit);
			glDisable(GL_POLYGON_OFFSET_FILL);
		} else {
			const double plane0[4] = {0, -1, 0 , start + height * (unit->buildProgress * 3 - 2)};
			glClipPlane(GL_CLIP_PLANE0, plane0);

			glPolygonOffset(1.0f, 1.0f);
			glEnable(GL_POLYGON_OFFSET_FILL);
				DrawUnitModel(unit);
			glDisable(GL_POLYGON_OFFSET_FILL);
		}
	}

	glDisable(GL_CLIP_PLANE0);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}



inline void CUnitDrawer::DrawUnitModel(CUnit* unit) {
	if (unit->luaDraw && luaRules && luaRules->DrawUnit(unit)) {
		return;
	}

	if (unit->lodCount <= 0) {
		unit->localModel->Draw();
	} else {

		GML_LODMUTEX_LOCK(unit); // DrawUnitModel
#ifdef USE_GML
		if (unit->lodCount <= 0) // re-read the value inside the mutex
			unit->localModel->Draw();
		else
#endif
		unit->localModel->DrawLOD(unit->currentLOD);
	}
}


void CUnitDrawer::DrawUnitNow(CUnit* unit)
{
	/*
	// this interferes with Lua material management
	if (unit->alphaThreshold != 0.1f) {
		glPushAttrib(GL_COLOR_BUFFER_BIT);
		glAlphaFunc(GL_GREATER, unit->alphaThreshold);
	}
	*/

	glPushMatrix();
	glMultMatrixf(unit->GetTransformMatrix());

	if (!unit->beingBuilt || !unit->unitDef->showNanoFrame) {
		DrawUnitModel(unit);
	} else {
		DrawUnitBeingBuilt(unit);
	}
	glPopMatrix();

	/*
	if (unit->alphaThreshold != 0.1f) {
		glPopAttrib();
	}
	*/
}


void CUnitDrawer::DrawUnitWithLists(CUnit* unit, unsigned int preList, unsigned int postList)
{
	glPushMatrix();
	glMultMatrixf(unit->GetTransformMatrix());

	if (preList != 0) {
		glCallList(preList);
	}

	if (!unit->beingBuilt || !unit->unitDef->showNanoFrame) {
		DrawUnitModel(unit);
	} else {
		DrawUnitBeingBuilt(unit);
	}

	if (postList != 0) {
		glCallList(postList);
	}
	glPopMatrix();
}


void CUnitDrawer::DrawUnitRaw(CUnit* unit)
{
	glPushMatrix();
	glMultMatrixf(unit->GetTransformMatrix());
	DrawUnitModel(unit);
	glPopMatrix();
}


void CUnitDrawer::DrawUnitRawModel(CUnit* unit)
{
	if (unit->lodCount <= 0) {
		unit->localModel->Draw();
	} else {

		GML_LODMUTEX_LOCK(unit); // DrawUnitRawModel
#ifdef USE_GML
		if (unit->lodCount <= 0)
			unit->localModel->Draw();
		else
#endif
		unit->localModel->DrawLOD(unit->currentLOD);
	}
}


void CUnitDrawer::DrawUnitRawWithLists(CUnit* unit, unsigned int preList, unsigned int postList)
{
	glPushMatrix();
	glMultMatrixf(unit->GetTransformMatrix());

	if (preList != 0) {
		glCallList(preList);
	}

	DrawUnitModel(unit);

	if (postList != 0) {
		glCallList(postList);
	}

	glPopMatrix();
}

#ifdef USE_GML
void CUnitDrawer::DrawUnitStats(CUnit* unit)
{
	if ((gu->myAllyTeam != unit->allyteam) &&
	    !gu->spectatingFullView && unit->unitDef->hideDamage) {
		return;
	}

	float3 interPos = unit->drawPos;
	interPos.y += unit->model->height + 5.0f;

	// setup the billboard transformation
	glPushMatrix();
	glTranslatef(interPos.x, interPos.y, interPos.z);
	glMultMatrixf(camera->GetBillBoardMatrix());

	if (unit->health < unit->maxHealth || unit->paralyzeDamage > 0.0f) {
		// black background for healthbar
		glColor3f(0.0f, 0.0f, 0.0f);
		glRectf(-5.0f, 4.0f, +5.0f, 6.0f);

		// health & stun level
		const float health = std::max(0.0f, unit->health / unit->maxHealth);
		const float stun = std::min(1.0f, unit->paralyzeDamage / unit->maxHealth);
		float hsmin = std::min(health, stun);
		const float colR = std::max(0.0f, 2.0f - 2.0f * health);
		const float colG = std::min(2.0f * health, 1.0f);
		if (hsmin > 0.0f) {
			const float hscol = 0.8f - 0.5f * hsmin;
			hsmin *= 10.0f;
			glColor3f(colR * hscol, colG * hscol, 1.0f);
			glRectf(-5.0f, 4.0f, hsmin - 5.0f, 6.0f);
		}
		if (health > stun) {
			glColor3f(colR, colG, 0.0f);
			glRectf(hsmin - 5.0f, 4.0f, health * 10.0f - 5.0f, 6.0f);
		}
		if (health < stun) {
			glColor3f(0.0f, 0.0f, 1.0f);
			glRectf(hsmin - 5.0f, 4.0f, stun * 10.0f - 5.0f, 6.0f);
		}
	}

	// skip the rest of the indicators if it isn't a local unit
	if ((gu->myTeam == unit->team) || gu->spectatingFullView) {
		// experience bar
		if (unit->limExperience > 0.0f) {
			const float eEnd = (unit->limExperience * 0.8f) * 10.0f;
			glColor3f(1.0f, 1.0f, 1.0f);
			glRectf(6.0f, -2.0f, 8.0f, eEnd - 2.0f);
		}
		if (unit->beingBuilt) {
			const float bEnd = (unit->buildProgress * 0.8f) * 10.0f;
			glColor3f(1.0f, 0.0f, 0.0f);
			glRectf(-8.0f, -2.0f, -6.0f, bEnd - 2.0f);
		}
		else if (unit->stockpileWeapon) {
			const float sEnd = (unit->stockpileWeapon->buildPercent * 0.8f) * 10.0f;
			glColor3f(1.0f, 0.0f, 0.0f);
			glRectf(-8.0f, -2.0f, -6.0f, sEnd - 2.0f);
		}
		if (unit->group) {
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			font->glFormat(8.0f, 0.0f, 10.0f, FONT_BASELINE, "%i", unit->group->id);
		}
	}

	glPopMatrix();
}
#endif



inline void CUnitDrawer::UpdateUnitIconState(CUnit* unit) {
	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];

	// reset
	unit->isIcon = false;

	if ((losStatus & LOS_INLOS) || gu->spectatingFullView) {
		unit->isIcon = DrawAsIcon(unit, (unit->pos - camera->pos).SqLength());
#ifdef USE_GML
		if (showHealthBars && !unit->noDraw &&
			(unit->health < unit->maxHealth || unit->paralyzeDamage > 0.0f || unit->limExperience > 0.0f ||
			unit->beingBuilt || unit->stockpileWeapon || unit->group) &&
			((unit->pos - camera->pos).SqLength() < (unitDrawDistSqr * 500.0f)))
			drawStat.insert(unit);
#endif
	} else if ((losStatus & LOS_PREVLOS) && (losStatus & LOS_CONTRADAR)) {
		if (gameSetup->ghostedBuildings && unit->unitDef->IsImmobileUnit()) {
			unit->isIcon = DrawAsIcon(unit, (unit->pos - camera->pos).SqLength());
		}
	}

	if (unit->isIcon && !unit->noDraw)
		drawIcon.insert(unit);
}

inline void CUnitDrawer::UpdateUnitDrawPos(CUnit* u) {
	const CTransportUnit* trans = u->GetTransporter();

	if (!GML::SimEnabled()) {
		if (trans != NULL) {
			u->drawPos = u->pos + (trans->speed * globalRendering->timeOffset);
		} else {
			u->drawPos = u->pos + (u->speed * globalRendering->timeOffset);
		}
	} else {
		const float timeOffset = (1.0f * spring_tomsecs(globalRendering->lastFrameStart)) - (1.0f * u->lastUnitUpdate);
		const float timeInterp = timeOffset * globalRendering->weightedSpeedFactor;

		if (trans != NULL) {
			u->drawPos = u->pos + (trans->speed * timeInterp);
		} else {
			u->drawPos = u->pos + (u->speed * timeInterp);
		}
	}

	u->drawMidPos = u->drawPos + (u->midPos - u->pos);
}



bool CUnitDrawer::DrawAsIcon(const CUnit* unit, const float sqUnitCamDist) const {

	const float sqIconDistMult = unit->unitDef->iconType->GetDistanceSqr();
	const float realIconLength = iconLength * sqIconDistMult;
	bool asIcon = false;

	if (useDistToGroundForIcons) {
		asIcon = (sqCamDistToGroundForIcons > realIconLength);
	} else {
		asIcon = (sqUnitCamDist > realIconLength);
	}

	return asIcon;
}




//! visualize if a unit can be built at specified position
bool CUnitDrawer::ShowUnitBuildSquare(const BuildInfo& buildInfo)
{
	return ShowUnitBuildSquare(buildInfo, std::vector<Command>());
}

bool CUnitDrawer::ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command>& commands)
{
	//TODO: make this a lua callin!
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	CFeature* feature = NULL;
	CVertexArray* va = GetVertexArray();

	std::vector<float3> buildableSquares; // buildable squares
	std::vector<float3> featureSquares; // occupied squares
	std::vector<float3> illegalSquares; // non-buildable squares

	const float3& pos = buildInfo.pos;
	const int x1 = pos.x - (buildInfo.GetXSize() * 0.5f * SQUARE_SIZE);
	const int x2 =    x1 + (buildInfo.GetXSize() *        SQUARE_SIZE);
	const int z1 = pos.z - (buildInfo.GetZSize() * 0.5f * SQUARE_SIZE);
	const int z2 =    z1 + (buildInfo.GetZSize() *        SQUARE_SIZE);
	const float h = CGameHelper::GetBuildHeight(pos, buildInfo.def, false);

	const bool canBuild = !!CGameHelper::TestUnitBuildSquare(
		buildInfo,
		feature,
		-1,
		false,
		&buildableSquares,
		&featureSquares,
		&illegalSquares,
		&commands
	);

	if (canBuild) {
		glColor4f(0.0f, 0.9f, 0.0f, 0.7f);
	} else {
		glColor4f(0.9f, 0.8f, 0.0f, 0.7f);
	}

	va->Initialize();
	va->EnlargeArrays(buildableSquares.size() * 4, 0, VA_SIZE_0);

	for (unsigned int i = 0; i < buildableSquares.size(); i++) {
		va->AddVertexQ0(buildableSquares[i]                                      );
		va->AddVertexQ0(buildableSquares[i] + float3(SQUARE_SIZE, 0,           0));
		va->AddVertexQ0(buildableSquares[i] + float3(SQUARE_SIZE, 0, SQUARE_SIZE));
		va->AddVertexQ0(buildableSquares[i] + float3(          0, 0, SQUARE_SIZE));
	}
	va->DrawArray0(GL_QUADS);


	glColor4f(0.9f, 0.8f, 0.0f, 0.7f);
	va->Initialize();
	va->EnlargeArrays(featureSquares.size() * 4, 0, VA_SIZE_0);

	for (unsigned int i = 0; i < featureSquares.size(); i++) {
		va->AddVertexQ0(featureSquares[i]                                      );
		va->AddVertexQ0(featureSquares[i] + float3(SQUARE_SIZE, 0,           0));
		va->AddVertexQ0(featureSquares[i] + float3(SQUARE_SIZE, 0, SQUARE_SIZE));
		va->AddVertexQ0(featureSquares[i] + float3(          0, 0, SQUARE_SIZE));
	}
	va->DrawArray0(GL_QUADS);


	glColor4f(0.9f, 0.0f, 0.0f, 0.7f);
	va->Initialize();
	va->EnlargeArrays(illegalSquares.size() * 4, 0, VA_SIZE_0);

	for (unsigned int i = 0; i < illegalSquares.size(); i++) {
		va->AddVertexQ0(illegalSquares[i]);
		va->AddVertexQ0(illegalSquares[i] + float3(SQUARE_SIZE, 0,           0));
		va->AddVertexQ0(illegalSquares[i] + float3(SQUARE_SIZE, 0, SQUARE_SIZE));
		va->AddVertexQ0(illegalSquares[i] + float3(          0, 0, SQUARE_SIZE));
	}
	va->DrawArray0(GL_QUADS);


	if (h < 0.0f) {
		const unsigned char s[4] = { 0,   0, 255, 128 }; // start color
		const unsigned char e[4] = { 0, 128, 255, 255 }; // end color

		va->Initialize();
		va->EnlargeArrays(8, 0, VA_SIZE_C);
		va->AddVertexQC(float3(x1, h, z1), s); va->AddVertexQC(float3(x1, 0.f, z1), e);
		va->AddVertexQC(float3(x1, h, z2), s); va->AddVertexQC(float3(x1, 0.f, z2), e);
		va->AddVertexQC(float3(x2, h, z2), s); va->AddVertexQC(float3(x2, 0.f, z2), e);
		va->AddVertexQC(float3(x2, h, z1), s); va->AddVertexQC(float3(x2, 0.f, z1), e);
		va->DrawArrayC(GL_LINES);

		va->Initialize();
		va->AddVertexQC(float3(x1, 0.0f, z1), e);
		va->AddVertexQC(float3(x1, 0.0f, z2), e);
		va->AddVertexQC(float3(x2, 0.0f, z2), e);
		va->AddVertexQC(float3(x2, 0.0f, z1), e);
		va->DrawArrayC(GL_LINE_LOOP);
	}

	glEnable(GL_DEPTH_TEST );
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	// glDisable(GL_BLEND);

	return canBuild;
}



inline const icon::CIconData* GetUnitIcon(const CUnit* unit) {
	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];
	const unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);

	const UnitDef* unitDef = unit->unitDef;
	const icon::CIconData* iconData = NULL;

	// use the unit's custom icon if we can currently see it,
	// or have seen it before and did not lose contact since
	const bool unitVisible = ((losStatus & LOS_INLOS) || ((losStatus & LOS_INRADAR) && ((losStatus & prevMask) == prevMask)));

	if (minimap->UseUnitIcons() && (unitVisible || gu->spectatingFullView)) {
		iconData = unitDef->iconType.GetIconData();
	} else {
		if (losStatus & LOS_INRADAR) {
			iconData = icon::iconHandler->GetDefaultIconData();
		}
	}

	return iconData;
}

inline float GetUnitIconScale(const CUnit* unit) {
	float scale = unit->myIcon->GetSize();

	if (!minimap->UseUnitIcons())
		return scale;
	if (!unit->myIcon->GetRadiusAdjust())
		return scale;

	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];
	const unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);
	const bool unitVisible = ((losStatus & LOS_INLOS) || ((losStatus & LOS_INRADAR) && ((losStatus & prevMask) == prevMask)));

	if ((unitVisible || gu->spectatingFullView)) {
		scale *= (unit->radius / 30.0f);
	}

	return scale;
}


void CUnitDrawer::DrawUnitMiniMapIcon(const CUnit* unit, CVertexArray* va) const {
	if (unit->noMinimap)
		return;
	if (unit->myIcon == NULL)
		return;

	const unsigned char defaultColor[4] = {255, 255, 255, 255};
	const unsigned char* color = &defaultColor[0];

	if (!unit->isSelected) {
		if (minimap->UseSimpleColors()) {
			if (unit->team == gu->myTeam) {
				color = minimap->GetMyTeamIconColor();
			} else if (teamHandler->Ally(gu->myAllyTeam, unit->allyteam)) {
				color = minimap->GetAllyTeamIconColor();
			} else {
				color = minimap->GetEnemyTeamIconColor();
			}
		} else {
			color = teamHandler->Team(unit->team)->color;
		}
	}

	const float iconScale = GetUnitIconScale(unit);
	const float3& iconPos = (!gu->spectatingFullView)?
		CGameHelper::GetUnitErrorPos(unit, gu->myAllyTeam):
		static_cast<float3>(unit->midPos);

	const float iconSizeX = (iconScale * minimap->GetUnitSizeX());
	const float iconSizeY = (iconScale * minimap->GetUnitSizeY());

	const float x0 = iconPos.x - iconSizeX;
	const float x1 = iconPos.x + iconSizeX;
	const float y0 = iconPos.z - iconSizeY;
	const float y1 = iconPos.z + iconSizeY;

	unit->myIcon->DrawArray(va, x0, y0, x1, y1, color);
}

// TODO:
//   UnitDrawer::DrawIcon was half-duplicate of MiniMap::DrawUnit&co
//   the latter has been replaced by this, do the same for the former
//   (mini-map icons and real-map radar icons are the same anyway)
void CUnitDrawer::DrawUnitMiniMapIcons() const {
	std::map<icon::CIconData*, std::set<const CUnit*> >::const_iterator iconIt;
	std::set<const CUnit*>::const_iterator unitIt;

	CVertexArray* va = GetVertexArray();

	for (iconIt = unitsByIcon.begin(); iconIt != unitsByIcon.end(); ++iconIt) {
		const icon::CIconData* icon = iconIt->first;
		const std::set<const CUnit*>& units = iconIt->second;

		if (icon == NULL)
			continue;
		if (units.empty())
			continue;

		va->Initialize();
		va->EnlargeArrays(units.size() * 4, 0, VA_SIZE_2DTC);
		icon->BindTexture();

		for (unitIt = units.begin(); unitIt != units.end(); ++unitIt) {
			assert((*unitIt)->myIcon == icon);
			DrawUnitMiniMapIcon(*unitIt, va);
		}

		va->DrawArray2dTC(GL_QUADS);
	}
}

void CUnitDrawer::UpdateUnitMiniMapIcon(const CUnit* unit, bool forced, bool killed) {
	icon::CIconData* oldIcon = unit->myIcon;
	icon::CIconData* newIcon = const_cast<icon::CIconData*>(GetUnitIcon(unit));

	CUnit* u = const_cast<CUnit*>(unit);

	if (!killed) {
		if ((oldIcon != newIcon) || forced) {
			unitsByIcon[oldIcon].erase(u);
			unitsByIcon[newIcon].insert(u);
		}
	} else {
		unitsByIcon[oldIcon].erase(u);
	}

	u->myIcon = killed? NULL: newIcon;
}



void CUnitDrawer::RenderUnitCreated(const CUnit* u, int cloaked) {
	CUnit* unit = const_cast<CUnit*>(u);
	texturehandlerS3O->UpdateDraw();

	if (GML::SimEnabled() && !GML::ShareLists()) {
		if (u->model && TEX_TYPE(u) < 0)
			TEX_TYPE(u) = texturehandlerS3O->LoadS3OTextureNow(u->model);
		if ((unsortedUnits.size() % 10) == 0)
			Watchdog::ClearPrimaryTimers(); // batching can create an avalance of events during /give xxx, triggering hang detection
	}

	if (u->model != NULL) {
		if (cloaked) {
			cloakedModelRenderers[MDL_TYPE(u)]->AddUnit(u);
		} else {
			opaqueModelRenderers[MDL_TYPE(u)]->AddUnit(u);
		}
	}

	UpdateUnitMiniMapIcon(u, false, false);
	unsortedUnits.insert(unit);
}


void CUnitDrawer::RenderUnitDestroyed(const CUnit* unit) {
	CUnit* u = const_cast<CUnit*>(unit);
	GhostSolidObject* gb = NULL;

	if ((dynamic_cast<CBuilding*>(u) != NULL) && gameSetup->ghostedBuildings &&
		!(u->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_CONTRADAR)) &&
		(u->losStatus[gu->myAllyTeam] & (LOS_PREVLOS)) && !gu->spectatingFullView
	) {
		// FIXME -- adjust decals for decoys? gets weird?
		const UnitDef* decoyDef = u->unitDef->decoyDef;
		S3DModel* gbModel = (decoyDef == NULL) ? u->model : decoyDef->LoadModel();

		gb = new GhostSolidObject();
		gb->pos    = u->pos;
		gb->model  = gbModel;
		gb->decal  = NULL;
		gb->facing = u->buildFacing;
		gb->dir    = u->frontdir;
		gb->team   = u->team;

		deadGhostBuildings[gbModel->type].insert(gb);

		groundDecals->GhostCreated(u, gb);
	}

	if (u->model) {
		// renderer unit cloak state may not match sim (because of MT) - erase from both renderers to be sure
		cloakedModelRenderers[MDL_TYPE(u)]->DelUnit(u);
		opaqueModelRenderers[MDL_TYPE(u)]->DelUnit(u);
	}

	unsortedUnits.erase(u);
	liveGhostBuildings[MDL_TYPE(u)].erase(u);

	// remove the icon for all ally-teams
	for (std::vector<std::set<CUnit*> >::iterator it = unitRadarIcons.begin(); it != unitRadarIcons.end(); ++it) {
		(*it).erase(u);
	}
#ifdef USE_GML
	drawIcon.erase(u);
	drawStat.erase(u);
#endif

	UpdateUnitMiniMapIcon(unit, false, true);
	SetUnitLODCount(u, 0);
}


void CUnitDrawer::RenderUnitCloakChanged(const CUnit* unit, int cloaked) {
	CUnit* u = const_cast<CUnit*>(unit);

	if (u->model) {
		if (cloaked) {
			cloakedModelRenderers[MDL_TYPE(u)]->AddUnit(u);
			opaqueModelRenderers[MDL_TYPE(u)]->DelUnit(u);
		} else {
			opaqueModelRenderers[MDL_TYPE(u)]->AddUnit(u);
			cloakedModelRenderers[MDL_TYPE(u)]->DelUnit(u);
		}
	}
}


void CUnitDrawer::RenderUnitLOSChanged(const CUnit* unit, int allyTeam, int newStatus) {
	CUnit* u = const_cast<CUnit*>(unit);

	if (newStatus & LOS_INLOS) {
		if (allyTeam == gu->myAllyTeam) {
			if (gameSetup->ghostedBuildings && unit->unitDef->IsImmobileUnit()) {
				liveGhostBuildings[MDL_TYPE(unit)].erase(u);
			}
		}
		unitRadarIcons[allyTeam].erase(u);
	} else {
		if (newStatus & LOS_PREVLOS) {
			if (allyTeam == gu->myAllyTeam) {
				if (gameSetup->ghostedBuildings && unit->unitDef->IsImmobileUnit()) {
					liveGhostBuildings[MDL_TYPE(unit)].insert(u);
				}
			}
		}

		if (newStatus & LOS_INRADAR) {
			unitRadarIcons[allyTeam].insert(u);
		} else {
			unitRadarIcons[allyTeam].erase(u);
		}
	}

	UpdateUnitMiniMapIcon(unit, false, false);
}






unsigned int CUnitDrawer::CalcUnitLOD(const CUnit* unit, unsigned int lastLOD)
{
	if (lastLOD == 0) { return 0; }

	const float3 diff = (unit->pos - camera->pos);
	const float dist = diff.dot(camera->forward);
	const float lpp = std::max(0.0f, dist * UNIT_GLOBAL_LOD_FACTOR);

	for (/* no-op */; lastLOD != 0; lastLOD--) {
		if (lpp > unit->lodLengths[lastLOD]) {
			break;
		}
	}

	return lastLOD;
}

// unused
unsigned int CUnitDrawer::CalcUnitShadowLOD(const CUnit* unit, unsigned int lastLOD)
{
	return CalcUnitLOD(unit, lastLOD); // FIXME

	// FIXME -- the more 'correct' method
	if (lastLOD == 0) { return 0; }

	// FIXME: fix it, cap it for shallow shadows?
	const float3& sun = sky->GetLight()->GetLightDir();
	const float3 diff = (camera->pos - unit->pos);
	const float  dot  = diff.dot(sun);
	const float3 gap  = diff - (sun * dot);
	const float  lpp  = std::max(0.0f, gap.Length() * UNIT_GLOBAL_LOD_FACTOR);

	for (/* no-op */; lastLOD != 0; lastLOD--) {
		if (lpp > unit->lodLengths[lastLOD]) {
			break;
		}
	}

	return lastLOD;
}

void CUnitDrawer::SetUnitLODCount(CUnit* unit, unsigned int count)
{
	GML_LODMUTEX_LOCK(unit); // SetUnitLODCount

	const unsigned int oldCount = unit->lodCount;

	unit->lodCount = count;
	unit->lodLengths.resize(count);
	unit->localModel->SetLODCount(count);

	for (unsigned int i = oldCount; i < count; i++) {
		unit->lodLengths[i] = -1.0f;
	}

	for (int m = 0; m < LUAMAT_TYPE_COUNT; m++) {
		unit->luaMats[m].SetLODCount(count);
	}
#ifdef USE_GML
	if (unit->currentLOD >= count)
		unit->currentLOD = (count == 0) ? 0 : count - 1;
#endif
}









void CUnitDrawer::PlayerChanged(int playerNum) {
	if (playerNum != gu->myPlayerNum)
		return;

	std::map<icon::CIconData*, std::set<const CUnit*> >::iterator iconIt;
	std::set<CUnit*>::const_iterator unitIt;

	for (iconIt = unitsByIcon.begin(); iconIt != unitsByIcon.end(); ++iconIt) {
		(iconIt->second).clear();
	}

	for (unitIt = unsortedUnits.begin(); unitIt != unsortedUnits.end(); ++unitIt) {
		// force an erase (no-op) followed by an insert
		UpdateUnitMiniMapIcon(*unitIt, true, false);
	}
}

void CUnitDrawer::SunChanged(const float3& sunDir) {
	if (advShading && shadowHandler->shadowsSupported && globalRendering->haveGLSL) {
		const float3 factoredUnitSunColor = unitSunColor * sky->GetLight()->GetLightIntensity();

		modelShaders[MODEL_SHADER_S3O_SHADOW]->Enable();
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniform3fv(5, &sky->GetLight()->GetLightDir().x);
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniform1f(12, sky->GetLight()->GetUnitShadowDensity());
		modelShaders[MODEL_SHADER_S3O_SHADOW]->SetUniform3fv(11, &factoredUnitSunColor[0]);
		modelShaders[MODEL_SHADER_S3O_SHADOW]->Disable();
	}
}

