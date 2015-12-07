/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "UnitDrawer.h"
#include "UnitDrawerState.hpp"

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/UI/MiniMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"

#include "Rendering/Env/ISky.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/FarTextureHandler.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/IconHandler.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Models/WorldObjectModelRenderer.h"

#include "Sim/Features/Feature.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Unit.h"

#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/Util.h"

#define UNIT_SHADOW_ALPHA_MASKING

CUnitDrawer* unitDrawer;

CONFIG(int, UnitLodDist).defaultValue(1000).headlessValue(0);
CONFIG(int, UnitIconDist).defaultValue(200).headlessValue(0);
CONFIG(float, UnitTransparency).defaultValue(0.7f);

CONFIG(int, MaxDynamicModelLights)
	.defaultValue(1)
	.minimumValue(0);

CONFIG(bool, AdvUnitShading).defaultValue(true).headlessValue(false).safemodeValue(false).description("Determines whether specular highlights and other lighting effects are rendered for units.");



CUnitDrawer::CUnitDrawer(): CEventClient("[CUnitDrawer]", 271828, false)
{
	eventHandler.AddClient(this);

	LuaObjectDrawer::ReadLODScales(LUAOBJ_UNIT);
	SetUnitDrawDist((float)configHandler->GetInt("UnitLodDist"));
	SetUnitIconDist((float)configHandler->GetInt("UnitIconDist"));

	unitAmbientColor = mapInfo->light.unitAmbientColor;
	unitSunColor = mapInfo->light.unitSunColor;

	cloakAlpha  = std::max(0.11f, std::min(1.0f, 1.0f - configHandler->GetFloat("UnitTransparency")));
	cloakAlpha1 = std::min(1.0f, cloakAlpha + 0.1f);
	cloakAlpha2 = std::min(1.0f, cloakAlpha + 0.2f);
	cloakAlpha3 = std::min(1.0f, cloakAlpha + 0.4f);

	// load unit explosion generators and decals
	for (size_t unitDefID = 1; unitDefID < unitDefHandler->unitDefs.size(); unitDefID++) {
		UnitDef& ud = unitDefHandler->unitDefs[unitDefID];

		for (unsigned int n = 0; n < ud.modelCEGTags.size(); n++) {
			ud.SetModelExplosionGeneratorID(n, explGenHandler->LoadGeneratorID(ud.modelCEGTags[n]));
		}
		for (unsigned int n = 0; n < ud.pieceCEGTags.size(); n++) {
			// these can only be custom EG's so prefix is not required game-side
			ud.SetPieceExplosionGeneratorID(n, explGenHandler->LoadGeneratorID(CEG_PREFIX_STRING + ud.pieceCEGTags[n]));
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

	// LH must be initialized before drawer-state is initialized
	lightHandler.Init(2U, configHandler->GetInt("MaxDynamicModelLights"));

	unitDrawerStateSSP = IUnitDrawerState::GetInstance(globalRendering->haveARB, globalRendering->haveGLSL);
	unitDrawerStateFFP = IUnitDrawerState::GetInstance(                   false,                     false);

	// also set in ::SetupForUnitDrawing, but SunChanged can be
	// called first if DynamicSun is enabled --> must be non-NULL
	SelectRenderState(false);

	// shared with FeatureDrawer!
	geomBuffer = LuaObjectDrawer::GetGeometryBuffer();

	drawForward = true;
	drawDeferred = (geomBuffer->Valid());

	// NOTE:
	//     advShading can NOT change at runtime if initially false***
	//     (see AdvModelShadingActionExecutor), so we will always use
	//     unitDrawerStateFFP (in ::Draw) in that special case and it
	//     does not matter if unitDrawerStateSSP is initialized
	//     *** except for DrawCloakedUnits
	advFading = GLEW_NV_vertex_program2;
	advShading = (unitDrawerStateSSP->Init(this) && cubeMapHandler->Init());
}

CUnitDrawer::~CUnitDrawer()
{
	eventHandler.RemoveClient(this);

	unitDrawerStateSSP->Kill(); IUnitDrawerState::FreeInstance(unitDrawerStateSSP);
	unitDrawerStateFFP->Kill(); IUnitDrawerState::FreeInstance(unitDrawerStateFFP);

	cubeMapHandler->Free();

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		for (GhostSolidObject* ghost: deadGhostBuildings[modelType]) {
			delete ghost;
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

	unitRadarIcons.clear();
	unsortedUnits.clear();
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
		while (!tempDrawUnits.empty() && tempDrawUnits.begin()->first < gs->frameNum - 1) {
			tempDrawUnits.erase(tempDrawUnits.begin());
		}
		while (!tempTransparentDrawUnits.empty() && tempTransparentDrawUnits.begin()->first <= gs->frameNum) {
			tempTransparentDrawUnits.erase(tempTransparentDrawUnits.begin());
		}
	}

	{
		drawIcon.clear();
		for (CUnit* unit: unsortedUnits) {
			UpdateUnitIconState(unit);
			UpdateUnitDrawPos(unit);
		}
	}

	useDistToGroundForIcons = (camHandler->GetCurrentController()).GetUseDistToGroundForIcons();

	if (useDistToGroundForIcons) {
		const float3& camPos = camera->GetPos();
		// use the height at the current camera position
		//const float groundHeight = CGround::GetHeightAboveWater(camPos.x, camPos.z, false);
		// use the middle between the highest and lowest position on the map as average
		const float groundHeight = (readMap->GetCurrMinHeight() + readMap->GetCurrMaxHeight()) * 0.5f;
		const float overGround = camPos.y - groundHeight;

		sqCamDistToGroundForIcons = overGround * overGround;
	}
}




void CUnitDrawer::Draw(bool drawReflection, bool drawRefraction)
{
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	ISky::SetupFog();

	camNorm = camera->GetDir();
	camNorm.y = -0.1f;
	camNorm.ANormalize();

	const CPlayer* myPlayer = gu->GetMyPlayer();
	const CUnit* excludeUnit = drawReflection? NULL: myPlayer->fpsController.GetControllee();

	// first do the deferred pass; conditional because
	// most of the water renderers use their own FBO's
	if (drawDeferred && !drawReflection && !drawRefraction) {
		LuaObjectDrawer::DrawDeferredPass(excludeUnit, LUAOBJ_UNIT);
	}

	// now do the regular forward pass
	if (drawForward) {
		DrawOpaquePass(excludeUnit, false, drawReflection, drawRefraction);
	}

	farTextureHandler->Draw();
	DrawUnitIcons(drawReflection);

	glDisable(GL_FOG);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
}

void CUnitDrawer::DrawOpaquePass(const CUnit* excludeUnit, bool deferredPass, bool drawReflection, bool drawRefraction)
{
	SetupForUnitDrawing(deferredPass);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		opaqueModelRenderers[modelType]->PushRenderState();
		DrawOpaqueUnits(modelType, excludeUnit, drawReflection, drawRefraction);
		opaqueModelRenderers[modelType]->PopRenderState();
	}

	// not true during DynWater::DrawRefraction
	// assert(drawReflection == water->DrawReflectionPass());

	CleanUpUnitDrawing(deferredPass);

	// draw all custom'ed units that were bypassed in the loop above
	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_UNIT);
	LuaObjectDrawer::DrawOpaqueMaterialObjects(LUAOBJ_UNIT, deferredPass);
}



void CUnitDrawer::DrawOpaqueUnits(int modelType, const CUnit* excludeUnit, bool drawReflection, bool drawRefraction)
{
	typedef std::vector<CUnit*> UnitSet;
	typedef std::map<int, UnitSet> UnitBin;

	const UnitBin& unitBin = opaqueModelRenderers[modelType]->GetUnitBin();

	UnitBin::const_iterator unitBinIt;

	for (unitBinIt = unitBin.begin(); unitBinIt != unitBin.end(); ++unitBinIt) {
		if (modelType != MODELTYPE_3DO) {
			texturehandlerS3O->SetS3oTexture(unitBinIt->first);
		}

		for (CUnit* unit: unitBinIt->second) {
			DrawOpaqueUnit(unit, excludeUnit, drawReflection, drawRefraction);
		}
	}

	if (modelType == MODELTYPE_3DO) {
		DrawOpaqueAIUnits();
	}
}

inline void CUnitDrawer::DrawOpaqueUnit(CUnit* unit, const CUnit* excludeUnit, bool drawReflection, bool drawRefraction)
{
	if (!CanDrawOpaqueUnit(unit, excludeUnit, drawReflection, drawRefraction))
		return;

	if ((unit->pos).SqDistance(camera->GetPos()) > (unit->sqRadius * unitDrawDistSqr)) {
		farTextureHandler->Queue(unit);
		return;
	}

	if (LuaObjectDrawer::AddOpaqueMaterialObject(unit, LUAOBJ_UNIT))
		return;

	// draw the unit with the default (non-Lua) material
	SetTeamColour(unit->team);
	DrawUnit(unit, 0, 0, false, false);
}


void CUnitDrawer::DrawOpaqueAIUnits()
{
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

		for (CUnit* u: drawIcon) {
			DrawIcon(u, false);
		}
		if (!gu->spectatingFullView) {
			for (CUnit* u: unitRadarIcons[gu->myAllyTeam]) {
				DrawIcon(u, (u->losStatus[gu->myAllyTeam] & (LOS_PREVLOS | LOS_CONTRADAR)) != (LOS_PREVLOS | LOS_CONTRADAR));
			}
		}

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
}



/******************************************************************************/
/******************************************************************************/

bool CUnitDrawer::CanDrawOpaqueUnit(
	const CUnit* unit,
	const CUnit* excludeUnit,
	bool drawReflection,
	bool drawRefraction
) const {
	if (unit == excludeUnit)
		return false;
	if (unit->noDraw)
		return false;
	if (unit->IsInVoid())
		return false;
	if (unit->isIcon)
		return false;

	if (!(unit->losStatus[gu->myAllyTeam] & LOS_INLOS) && !gu->spectatingFullView)
		return false;

	if (drawReflection) {
		float3 zeroPos = unit->drawMidPos;

		if (unit->drawMidPos.y >= 0.0f) {
			const float dif = unit->drawMidPos.y - camera->GetPos().y;

			zeroPos  = (camera->GetPos()  * (unit->drawMidPos.y / dif));
			zeroPos += (unit->drawMidPos * (-camera->GetPos().y / dif));
		}
		if (CGround::GetApproximateHeight(zeroPos.x, zeroPos.z, false) > unit->drawRadius) {
			return false;
		}
	} else if (drawRefraction) {
		if (unit->pos.y > 0.0f) {
			return false;
		}
	}

	return (camera->InView(unit->drawMidPos, unit->drawRadius));
}

bool CUnitDrawer::CanDrawOpaqueUnitShadow(const CUnit* unit) const
{
	const bool unitInLOS = ((unit->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectatingFullView);

	if (unit->noDraw)
		return false;
	if (unit->IsInVoid())
		return false;
	if (unit->isIcon)
		return false;

	// FIXME: test against the shadow projection intersection
	if (!(unitInLOS && camera->InView(unit->drawMidPos, unit->drawRadius + 700.0f)))
		return false;

	const float sqDist = (unit->pos - camera->GetPos()).SqLength();
	const float farLength = unit->sqRadius * unitDrawDistSqr;

	if (sqDist >= farLength)
		return false;
	if (unit->isCloaked)
		return false;

	return true;
}




void CUnitDrawer::DrawOpaqueUnitShadow(CUnit* unit) {
	if (!CanDrawOpaqueUnitShadow(unit))
		return;

	if (LuaObjectDrawer::AddShadowMaterialObject(unit, LUAOBJ_UNIT))
		return;

	DrawUnit(unit, 0, 0, false, false);
}


void CUnitDrawer::DrawOpaqueUnitsShadow(int modelType) {
	const auto& unitBin = opaqueModelRenderers[modelType]->GetUnitBin();

	for (const auto& unitBinP: unitBin) {
		const auto& unitSet = unitBinP.second;
		const int textureType = unitBinP.first;

		if (modelType != MODELTYPE_3DO) {
			glActiveTexture(GL_TEXTURE0);
			glEnable(GL_TEXTURE_2D);
			const auto modelTex = texturehandlerS3O->GetS3oTex(textureType);
			glBindTexture(GL_TEXTURE_2D, modelTex->tex2);
		}

		for (const auto& unitSetP: unitSet) {
			DrawOpaqueUnitShadow(unitSetP);
		}

		if (modelType != MODELTYPE_3DO) {
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);
			glActiveTexture(GL_TEXTURE0);
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

	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_UNIT);
	LuaObjectDrawer::DrawShadowMaterialObjects(LUAOBJ_UNIT, false);
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
		pos = unit->GetDrawErrorPos(gu->myAllyTeam);
	}

	// make sure icon is above ground (needed before we calculate scale below)
	const float h = CGround::GetHeightReal(pos.x, pos.z, false);

	if (pos.y < h) {
		pos.y = h;
	}

	const float dist = std::min(8000.0f, fastmath::sqrt2(camera->GetPos().SqDistance(pos)));
	const float iconScale = 0.4f * fastmath::sqrt2(dist); // makes far icons bigger
	float scale = iconData->GetSize() * iconScale;

	if (iconData->GetRadiusAdjust() && !useDefaultIcon) {
		scale *= (unit->radius / WORLDOBJECT_DEFAULT_DRAWRADIUS);
	}

	// make sure icon is not partly under ground
	if (pos.y < (h + scale)) {
		pos.y = (h + scale);
	}

	unit->iconRadius = scale; // store the icon size so that we don't have to calculate it again

	// Is the unit selected? Then draw it white.
	if (unit->isSelected) {
		glColor3ub(255, 255, 255);
	} else {
		glColor3ubv(teamHandler->Team(unit->team)->color);
	}

	// calculate the vertices
	const float3 dy = camera->GetUp()    * scale;
	const float3 dx = camera->GetRight() * scale;
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
	// don't use shaders if shadows are enabled (WHY?)
	// also not safe: should set state instance to FFP
	advShading = advShading && !disableAdvShading;

	if (advShading) {
		SetupForUnitDrawing(false);
		glDisable(GL_ALPHA_TEST);
	} else {
		SetupForGhostDrawing();
	}

	glColor4f(1.0f, 1.0f, 1.0f, cloakAlpha);

	{
		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
			cloakedModelRenderers[modelType]->PushRenderState();
			DrawCloakedUnitsHelper(modelType);
			cloakedModelRenderers[modelType]->PopRenderState();
		}

		if (advShading) {
			CleanUpUnitDrawing(false);
			glEnable(GL_ALPHA_TEST);
		} else {
			CleanUpGhostDrawing();
		}

		advShading = oldAdvShading;
	}

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_UNIT);
	LuaObjectDrawer::DrawAlphaMaterialObjects(LUAOBJ_UNIT, false);
}

void CUnitDrawer::DrawCloakedUnitsHelper(int modelType)
{
	if (modelType == MODELTYPE_3DO) {
		DrawCloakedAIUnits();
	}

	{
		typedef std::vector<CUnit*> UnitSet;
		typedef std::map<int, UnitSet> UnitRenderBin;
		typedef std::map<int, UnitSet>::const_iterator UnitRenderBinIt;

		const UnitRenderBin& unitBin = cloakedModelRenderers[modelType]->GetUnitBin();

		// cloaked units
		for (UnitRenderBinIt it = unitBin.begin(); it != unitBin.end(); ++it) {
			if (modelType != MODELTYPE_3DO) {
				texturehandlerS3O->SetS3oTexture(it->first);
			}

			const UnitSet& unitSet = it->second;

			for (auto &unit: unitSet) {
				DrawCloakedUnit(unit, modelType, false);
			}
		}
	}

	// living and dead ghosted buildings
	DrawGhostedBuildings(modelType);
}

inline void CUnitDrawer::DrawCloakedUnit(CUnit* unit, int modelType, bool drawGhostBuildingsPass) {
	if (!camera->InView(unit->drawMidPos, unit->drawRadius))
		return;

	if (LuaObjectDrawer::AddAlphaMaterialObject(unit, LUAOBJ_UNIT))
		return;

	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];

	if (!drawGhostBuildingsPass) {
		if ((losStatus & LOS_INLOS) || gu->spectatingFullView) {
			if (!unit->isIcon) {
				SetTeamColour(unit->team, cloakAlpha);
				DrawUnit(unit, 0, 0, false, false);
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
		if (model->type != modelType)
			return;

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
	std::vector<GhostSolidObject*>& deadGhostedBuildings = deadGhostBuildings[modelType];
	std::vector<CUnit*>& liveGhostedBuildings = liveGhostBuildings[modelType];

	glColor4f(0.6f, 0.6f, 0.6f, cloakAlpha1);

	// buildings that died while ghosted
	for (std::vector<GhostSolidObject*>::iterator it = deadGhostedBuildings.begin(); it != deadGhostedBuildings.end(); ) {
		if (losHandler->InLos((*it)->pos, gu->myAllyTeam) || gu->spectatingFullView) {
			// obtained LOS on the ghost of a dead building
			groundDecals->GhostDestroyed(*it);

			delete *it;
			*it = deadGhostedBuildings.back();
			deadGhostedBuildings.pop_back();
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
		for (CUnit* u: liveGhostedBuildings) {
			// because of team switching via cheat, ghost buildings can exist for units in LOS
			if (!(u->losStatus[gu->myAllyTeam] & LOS_INLOS))
				DrawCloakedUnit(u, modelType, true);
		}
	}
}





void CUnitDrawer::SetupForUnitDrawing(bool deferredPass)
{
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	glAlphaFunc(GL_GREATER, 0.5f);
	glEnable(GL_ALPHA_TEST);

	// pick base shaders (ARB/GLSL) or FFP; not used by custom-material models
	SelectRenderState(unitDrawerStateSSP->CanEnable(this));
	unitDrawerState->Enable(this, deferredPass);

	// NOTE:
	//   when deferredPass is true we MUST be able to use unitDrawerStateSSP
	//   all calling code (reached from DrawOpaquePass(deferred=true)) should
	//   ensure this is the case
	assert(!deferredPass || unitDrawerState == unitDrawerStateSSP);
}

void CUnitDrawer::CleanUpUnitDrawing(bool deferredPass) const
{
	glDisable(GL_CULL_FACE);
	glDisable(GL_ALPHA_TEST);

	unitDrawerState->Disable(this, deferredPass);
}


bool CUnitDrawer::DrawDeferredSupported() const {
	if (!unitDrawerStateSSP->CanEnable(this))
		return false;
	if (!unitDrawerStateSSP->CanDrawDeferred())
		return false;

	return true;
}


void CUnitDrawer::SetTeamColour(int team, float alpha) const
{
	// need this because we can be called by no-team projectiles
	if (teamHandler->IsValidTeam(team)) {
		unitDrawerState->SetTeamColor(team, alpha);
	}
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

	if (!LuaObjectDrawer::DrawSingleObject(unit, LUAOBJ_UNIT)) {
		SetupForUnitDrawing(false);
		opaqueModelRenderers[MDL_TYPE(unit)]->PushRenderState();

		if (MDL_TYPE(unit) != MODELTYPE_3DO) {
			texturehandlerS3O->SetS3oTexture(TEX_TYPE(unit));
		}

		SetTeamColour(unit->team);
		DrawUnit(unit, 0, 0, false, true);

		opaqueModelRenderers[MDL_TYPE(unit)]->PopRenderState();
		CleanUpUnitDrawing(false);
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
void CUnitDrawer::DrawBuildingSample(const UnitDef* unitdef, int team, float3 pos, int facing)
{
	const S3DModel* model = unitdef->LoadModel();

	/* From SetupForGhostDrawing. */
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT);

	assert(teamHandler->IsValidTeam(team));
	IUnitDrawerState::SetBasicTeamColor(team, 1.0f);
	SetupBasicS3OTexture0();

	switch (model->type) {
		case MODELTYPE_3DO: {
			texturehandler3DO->Set3doAtlases();
		} break;
		default: texturehandlerS3O->SetS3oTexture(model->textureType);
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

// used by LuaOpenGL::DrawUnitShape only
void CUnitDrawer::DrawUnitDef(const UnitDef* unitDef, int team)
{
	const S3DModel* model = unitDef->LoadModel();

	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);

	// get the team-coloured texture constructed by texunit 0
	assert(teamHandler->IsValidTeam(team));
	IUnitDrawerState::SetBasicTeamColor(team, 1.0f);
	SetupBasicS3OTexture0();

	switch (model->type) {
		case MODELTYPE_3DO: {
			texturehandler3DO->Set3doAtlases();
		} break;
		default: texturehandlerS3O->SetS3oTexture(model->textureType);
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



void CUnitDrawer::DrawUnitBeingBuilt(const CUnit* unit)
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

	// render wireframe with FFP
	unitDrawerState->DisableShaders(this);
	unitDrawerState->DisableTextures(this);


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
		// some ATi mobility cards/drivers dont like clipping wireframes...
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

	unitDrawerState->EnableTextures(this);
	unitDrawerState->EnableShaders(this);

	// second stage: texture-mapped model
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




void CUnitDrawer::DrawUnitModel(const CUnit* unit, bool rawCall) {
	if (!rawCall && unit->luaDraw && eventHandler.DrawUnit(unit))
		return;

	unit->localModel.Draw();
}

void CUnitDrawer::DrawUnit(const CUnit* unit, unsigned int preList, unsigned int postList, bool lodCall, bool rawCall)
{
	glPushMatrix();
	glMultMatrixf(unit->GetTransformMatrix());

	if (preList != 0) {
		glCallList(preList);
	}

	// if called from LuaObjectDrawer, unit has a custom material
	//
	// DrawUnitBeingBuilt disables *any* shader that is currently
	// active for its first stage and only enables a standard one
	// for its second, while we want the Lua shader (if provided)
	// to have full control over the build visualisation: keep it
	// simple and bypass the engine path
	//
	// NOTE: "raw" calls will no longer skip DrawUnitBeingBuilt
	//
	if (lodCall || !unit->beingBuilt || !unit->unitDef->showNanoFrame) {
		DrawUnitModel(unit, rawCall);
	} else {
		DrawUnitBeingBuilt(unit);
	}

	if (postList != 0) {
		glCallList(postList);
	}

	glPopMatrix();
}




inline void CUnitDrawer::UpdateUnitIconState(CUnit* unit) {
	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];

	// reset
	unit->isIcon = false;

	if ((losStatus & LOS_INLOS) || gu->spectatingFullView) {
		unit->isIcon = DrawAsIcon(unit, (unit->pos - camera->GetPos()).SqLength());

	} else if ((losStatus & LOS_PREVLOS) && (losStatus & LOS_CONTRADAR)) {
		if (gameSetup->ghostedBuildings && unit->unitDef->IsImmobileUnit()) {
			unit->isIcon = DrawAsIcon(unit, (unit->pos - camera->GetPos()).SqLength());
		}
	}

	if (unit->isIcon && !unit->noDraw && !unit->IsInVoid())
		drawIcon.push_back(unit);
}

inline void CUnitDrawer::UpdateUnitDrawPos(CUnit* u) {
	const CUnit* trans = u->GetTransporter();

	if (trans != NULL) {
		u->drawPos = u->pos + (trans->speed * globalRendering->timeOffset);
	} else {
		u->drawPos = u->pos + (u->speed * globalRendering->timeOffset);
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




// visualize if a unit can be built at specified position
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

	CVertexArray* va = GetVertexArray();
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
	va = GetVertexArray();
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
	va = GetVertexArray();
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

		va = GetVertexArray();
		va->Initialize();
		va->EnlargeArrays(8, 0, VA_SIZE_C);
		va->AddVertexQC(float3(x1, h, z1), s); va->AddVertexQC(float3(x1, 0.f, z1), e);
		va->AddVertexQC(float3(x1, h, z2), s); va->AddVertexQC(float3(x1, 0.f, z2), e);
		va->AddVertexQC(float3(x2, h, z2), s); va->AddVertexQC(float3(x2, 0.f, z2), e);
		va->AddVertexQC(float3(x2, h, z1), s); va->AddVertexQC(float3(x2, 0.f, z1), e);
		va->DrawArrayC(GL_LINES);

		va = GetVertexArray();
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
	const bool unitVisible = ((losStatus & (LOS_INLOS | LOS_INRADAR)) && ((losStatus & prevMask) == prevMask));
	const bool customIcon = (minimap->UseUnitIcons() && (unitVisible || gu->spectatingFullView));

	if (customIcon) {
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
		scale *= (unit->radius / WORLDOBJECT_DEFAULT_DRAWRADIUS);
	}

	return scale;
}


void CUnitDrawer::DrawUnitMiniMapIcon(const CUnit* unit, CVertexArray* va) const {
	if (unit->noMinimap)
		return;
	if (unit->myIcon == NULL)
		return;
	if (unit->IsInVoid())
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
	const float3& iconPos = (!gu->spectatingFullView) ?
		unit->GetDrawErrorPos(gu->myAllyTeam):
		unit->drawMidPos;

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
	std::map<icon::CIconData*, std::vector<const CUnit*> >::const_iterator iconIt;

	CVertexArray* va = GetVertexArray();

	for (iconIt = unitsByIcon.begin(); iconIt != unitsByIcon.end(); ++iconIt) {
		const icon::CIconData* icon = iconIt->first;
		const std::vector<const CUnit*>& units = iconIt->second;

		if (icon == NULL)
			continue;
		if (units.empty())
			continue;

		va->Initialize();
		va->EnlargeArrays(units.size() * 4, 0, VA_SIZE_2DTC);
		icon->BindTexture();

		for (const CUnit* unit: units) {
			assert(unit->myIcon == icon);
			DrawUnitMiniMapIcon(unit, va);
		}

		va->DrawArray2dTC(GL_QUADS);
	}
}

void CUnitDrawer::UpdateUnitMiniMapIcon(const CUnit* unit, bool forced, bool killed) {
	icon::CIconData* oldIcon = unit->myIcon;
	icon::CIconData* newIcon = const_cast<icon::CIconData*>(GetUnitIcon(unit));

	if (!killed) {
		if ((oldIcon != newIcon) || forced) {
			VectorErase(unitsByIcon[oldIcon], unit);
			unitsByIcon[newIcon].push_back(unit);
		}
	} else {
		VectorErase(unitsByIcon[oldIcon], unit);
	}

	(const_cast<CUnit*>(unit))->myIcon = killed? NULL: newIcon;
}



void CUnitDrawer::RenderUnitCreated(const CUnit* u, int cloaked) {
	CUnit* unit = const_cast<CUnit*>(u);
	texturehandlerS3O->UpdateDraw();

	if (u->model != NULL) {
		if (cloaked) {
			cloakedModelRenderers[MDL_TYPE(u)]->AddUnit(u);
		} else {
			opaqueModelRenderers[MDL_TYPE(u)]->AddUnit(u);
		}
	}

	UpdateUnitMiniMapIcon(u, false, false);
	unsortedUnits.push_back(unit);
}


void CUnitDrawer::RenderUnitDestroyed(const CUnit* unit) {
	CUnit* u = const_cast<CUnit*>(unit);

	if (u->unitDef->IsBuildingUnit() && gameSetup->ghostedBuildings &&
		!(u->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_CONTRADAR)) &&
		(u->losStatus[gu->myAllyTeam] & (LOS_PREVLOS)) && !gu->spectatingFullView
	) {
		// FIXME -- adjust decals for decoys? gets weird?
		const UnitDef* decoyDef = u->unitDef->decoyDef;
		S3DModel* gbModel = (decoyDef == NULL) ? u->model : decoyDef->LoadModel();

		GhostSolidObject* gb = new GhostSolidObject();
		gb->pos    = u->pos;
		gb->model  = gbModel;
		gb->decal  = NULL;
		gb->facing = u->buildFacing;
		gb->dir    = u->frontdir;
		gb->team   = u->team;

		deadGhostBuildings[gbModel->type].push_back(gb);

		groundDecals->GhostCreated(u, gb);
	}

	if (u->model) {
		// delete from both; cloaked state is unreliable at this point
		cloakedModelRenderers[MDL_TYPE(u)]->DelUnit(u);
		opaqueModelRenderers[MDL_TYPE(u)]->DelUnit(u);
	}

	VectorErase(unsortedUnits, u);
	VectorErase(liveGhostBuildings[MDL_TYPE(u)], u);

	// remove the icon for all ally-teams
	for (std::vector<std::vector<CUnit*> >::iterator it = unitRadarIcons.begin(); it != unitRadarIcons.end(); ++it) {
		VectorErase(*it, u);
	}

	UpdateUnitMiniMapIcon(unit, false, true);
	LuaObjectDrawer::SetObjectLOD(u, LUAOBJ_UNIT, 0);
}


void CUnitDrawer::UnitCloaked(const CUnit* unit) {
	CUnit* u = const_cast<CUnit*>(unit);

	if (u->model) {
		cloakedModelRenderers[MDL_TYPE(u)]->AddUnit(u);
		opaqueModelRenderers[MDL_TYPE(u)]->DelUnit(u);
	}
}

void CUnitDrawer::UnitDecloaked(const CUnit* unit) {
	CUnit* u = const_cast<CUnit*>(unit);

	if (u->model) {
		opaqueModelRenderers[MDL_TYPE(u)]->AddUnit(u);
		cloakedModelRenderers[MDL_TYPE(u)]->DelUnit(u);
	}
}

void CUnitDrawer::UnitEnteredLos(const CUnit* unit, int allyTeam) {
	if (allyTeam != gu->myAllyTeam) {
		return;
	}

	CUnit* u = const_cast<CUnit*>(unit); //cleanup

	if (gameSetup->ghostedBuildings && unit->unitDef->IsImmobileUnit()) {
		VectorErase(liveGhostBuildings[MDL_TYPE(unit)], u);
	}

	VectorErase(unitRadarIcons[allyTeam], u);
	UpdateUnitMiniMapIcon(unit, false, false);
}

void CUnitDrawer::UnitLeftLos(const CUnit* unit, int allyTeam) {
	if (allyTeam != gu->myAllyTeam) {
		return;
	}

	CUnit* u = const_cast<CUnit*>(unit); //cleanup
	if (gameSetup->ghostedBuildings && unit->unitDef->IsImmobileUnit()) {
		VectorInsertUnique(liveGhostBuildings[MDL_TYPE(unit)], u, true);
	}

	if (unit->losStatus[allyTeam] & LOS_INRADAR) {
		VectorInsertUnique(unitRadarIcons[allyTeam], u, true);
	}

	UpdateUnitMiniMapIcon(unit, false, false);
}

void CUnitDrawer::UnitEnteredRadar(const CUnit* unit, int allyTeam) {
	if (allyTeam != gu->myAllyTeam) {
		return;
	}

	CUnit* u = const_cast<CUnit*>(unit);

	if (!(unit->losStatus[allyTeam] & LOS_INLOS)) {
		VectorInsertUnique(unitRadarIcons[allyTeam], u, true);
	}

	UpdateUnitMiniMapIcon(unit, false, false);
}

void CUnitDrawer::UnitLeftRadar(const CUnit* unit, int allyTeam) {
	if (allyTeam != gu->myAllyTeam) {
		return;
	}

	CUnit* u = const_cast<CUnit*>(unit);

	VectorErase(unitRadarIcons[allyTeam], u);
	UpdateUnitMiniMapIcon(unit, false, false);
}


void CUnitDrawer::PlayerChanged(int playerNum) {
	if (playerNum != gu->myPlayerNum)
		return;

	// unitsByIcon is a map of (Icon*, std::vector) entries
	for (auto iconIt = unitsByIcon.begin(); iconIt != unitsByIcon.end(); ++iconIt) {
		(iconIt->second).clear();
	}

	for (CUnit* unit: unsortedUnits) {
		// force an erase (no-op) followed by an insert
		UpdateUnitMiniMapIcon(unit, true, false);
	}
}

void CUnitDrawer::SunChanged(const float3& sunDir) {
	unitDrawerState->UpdateCurrentShader(this, sky->GetLight());
}

