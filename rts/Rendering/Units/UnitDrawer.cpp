/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnitDrawer.h"

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/UI/MiniMap.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/FarTextureHandler.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Colors.h"
#include "Rendering/IconHandler.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Common/ModelDrawerHelpers.h"
#include "Rendering/Models/3DModelVAO.h"
#include "Rendering/Models/MatricesMemStorage.h"

#include "Sim/Features/Feature.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"

#include "System/EventHandler.h"
#include "System/Config/ConfigHandler.h"
//#include "System/FileSystem/FileHandler.h"

#include "System/StringUtil.h"
#include "System/MemPoolTypes.h"
#include "System/SpringMath.h"

#include "System/Threading/ThreadPool.h"

CONFIG(int, UnitIconDist).defaultValue(200).headlessValue(0);
CONFIG(float, UnitIconScaleUI).defaultValue(1.0f).minimumValue(0.5f).maximumValue(2.0f);
CONFIG(float, UnitIconFadeStart).defaultValue(3000.0f).minimumValue(1.0f).maximumValue(10000.0f);
CONFIG(float, UnitIconFadeVanish).defaultValue(1000.0f).minimumValue(1.0f).maximumValue(10000.0f);
CONFIG(float, UnitTransparency).defaultValue(0.7f);
CONFIG(bool, UnitIconsAsUI).defaultValue(false).description("Draw unit icons like it is an UI element and not like unit's LOD.");
CONFIG(bool, UnitIconsHideWithUI).defaultValue(false).description("Hide unit icons when UI is hidden.");

CONFIG(int, MaxDynamicModelLights)
	.defaultValue(1)
	.minimumValue(0);

CONFIG(bool, AdvUnitShading).defaultValue(true).headlessValue(false).safemodeValue(false).description("Determines whether specular highlights and other lighting effects are rendered for units.");

/***********************************************************************/

//don't inherit and leave only static Unit specific helpers
class CUnitDrawerHelper
{
public:
	static void LoadUnitExplosionGenerators() {
		using F = decltype(&UnitDef::AddModelExpGenID);
		using T = decltype(UnitDef::modelCEGTags);

		const auto LoadGenerators = [](UnitDef* ud, const F addExplGenID, const T& explGenTags, const char* explGenPrefix) {
			for (const auto& explGenTag : explGenTags) {
				if (explGenTag[0] == 0)
					break;

				// build a contiguous range of valid ID's
				(ud->*addExplGenID)(explGenHandler.LoadGeneratorID(explGenTag, explGenPrefix));
			}
		};

		for (uint32_t i = 0, n = unitDefHandler->NumUnitDefs(); i < n; i++) {
			UnitDef* ud = const_cast<UnitDef*>(unitDefHandler->GetUnitDefByID(i + 1));

			// piece- and crash-generators can only be custom so the prefix is not required to be given game-side
			LoadGenerators(ud, &UnitDef::AddModelExpGenID, ud->modelCEGTags, "");
			LoadGenerators(ud, &UnitDef::AddPieceExpGenID, ud->pieceCEGTags, CEG_PREFIX_STRING);
			LoadGenerators(ud, &UnitDef::AddCrashExpGenID, ud->crashCEGTags, CEG_PREFIX_STRING);
		}
	}

	static inline float GetUnitIconScale(const CUnit* unit) {
		float scale = unit->myIcon->GetSize();

		if (!minimap->UseUnitIcons())
			return scale;
		if (!unit->myIcon->GetRadiusAdjust())
			return scale;

		const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];
		const unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);
		const bool unitVisible = ((losStatus & LOS_INLOS) || ((losStatus & LOS_INRADAR) && ((losStatus & prevMask) == prevMask)));

		if ((unitVisible || gu->spectatingFullView)) {
			scale *= (unit->radius / unit->myIcon->GetRadiusScale());
		}

		return scale;
	}
};


/***********************************************************************/


void CUnitDrawer::InitStatic()
{
	CModelDrawerBase<CUnitDrawerData, CUnitDrawer>::InitStatic();

	LuaObjectDrawer::ReadLODScales(LUAOBJ_UNIT);

	CUnitDrawerHelper::LoadUnitExplosionGenerators();

	CUnitDrawer::InitInstance<CUnitDrawerFFP >(MODEL_DRAWER_FFP );
	CUnitDrawer::InitInstance<CUnitDrawerARB >(MODEL_DRAWER_ARB );
	CUnitDrawer::InitInstance<CUnitDrawerGLSL>(MODEL_DRAWER_GLSL);
	CUnitDrawer::InitInstance<CUnitDrawerGL4 >(MODEL_DRAWER_GL4 );

	SelectImplementation();
}

bool CUnitDrawer::ShouldDrawOpaqueUnit(CUnit* u, bool drawReflection, bool drawRefraction)
{
	if (u == (drawReflection ? nullptr : (gu->GetMyPlayer())->fpsController.GetControllee()))
		return false;

	assert(u);
	assert(u->model);

	if (u->drawFlag == 0)
		return false;

	if (u->HasDrawFlag(DrawFlags::SO_DRICON_FLAG))
		return false;

	if (u->isIcon) //TODO remove isIcon
		return false;

	if (u->HasDrawFlag(DrawFlags::SO_ALPHAF_FLAG))
		return false;

	if (u->HasDrawFlag(DrawFlags::SO_FARTEX_FLAG)) {
		farTextureHandler->Queue(u);
		return false;
	}

	if (LuaObjectDrawer::AddOpaqueMaterialObject(u, LUAOBJ_UNIT))
		return false;

	return true;
}

bool CUnitDrawer::ShouldDrawAlphaUnit(CUnit* u)
{
	assert(u);
	assert(u->model);

	if (u->drawFlag == 0)
		return false;

	if (u->HasDrawFlag(DrawFlags::SO_DRICON_FLAG))
		return false;

	if (u->isIcon) //TODO remove isIcon
		return false;

	if (u->HasDrawFlag(DrawFlags::SO_OPAQUE_FLAG))
		return false;

	if (LuaObjectDrawer::AddAlphaMaterialObject(u, LUAOBJ_UNIT))
		return false;

	return true;
}

bool CUnitDrawer::ShouldDrawUnitShadow(CUnit* u)
{
	assert(u);
	assert(u->model);

	if (!u->HasDrawFlag(DrawFlags::SO_SHADOW_FLAG))
		return false;

	if (LuaObjectDrawer::AddShadowMaterialObject(u, LUAOBJ_UNIT))
		return false;

	return true;
}

/***********************************************************************/


void CUnitDrawerBase::Update() const
{
	SCOPED_TIMER("CUnitDrawerBase::Update");
	modelDrawerData->Update();
}

/***********************************************************************/

void CUnitDrawerLegacy::DrawUnitModel(const CUnit* unit, bool noLuaCall) const
{
	if (!noLuaCall && unit->luaDraw && eventHandler.DrawUnit(unit))
		return;

	unit->localModel.Draw();
}

void CUnitDrawerLegacy::DrawUnitNoTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const
{
	const bool noNanoDraw = lodCall || !unit->beingBuilt || !unit->unitDef->showNanoFrame;
	const bool shadowPass = shadowHandler.InShadowPass();

	if (preList != 0) {
		glCallList(preList);
	}

	// if called from LuaObjectDrawer, unit has a custom material
	//
	// we want Lua-material shaders to have full control over build
	// visualisation, so keep it simple and make LOD-calls draw the
	// full model
	//
	// NOTE: "raw" calls will no longer skip DrawUnitBeingBuilt
	//

	//drawModelFuncs[std::max(noNanoDraw * 2, shadowPass)](unit, noLuaCall);
	if (noNanoDraw)
		DrawUnitModel(unit, noLuaCall);
	else {
		if (shadowPass)
			DrawUnitModelBeingBuiltShadow(unit, noLuaCall);
		else
			DrawUnitModelBeingBuiltOpaque(unit, noLuaCall);
	}


	if (postList != 0) {
		glCallList(postList);
	}
}

void CUnitDrawerLegacy::DrawUnitTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const
{
	glPushMatrix();
	glMultMatrixf(unit->GetTransformMatrix());

	DrawUnitNoTrans(unit, preList, postList, lodCall, noLuaCall);

	glPopMatrix();
}

void CUnitDrawerLegacy::DrawUnitMiniMapIcons() const
{
	CVertexArray* va = GetVertexArray();

	for (const auto& [icon, units] : modelDrawerData->GetUnitsByIcon()) {

		if (icon == nullptr)
			continue;
		if (units.empty())
			continue;

		va->Initialize();
		va->EnlargeArrays(units.size() * 4, 0, VA_SIZE_2DTC);
		icon->BindTexture();

		for (const CUnit* unit : units) {
			assert(unit->myIcon == icon);
			DrawUnitMiniMapIcon(unit, va);
		}

		va->DrawArray2dTC(GL_QUADS);
	}
}

void CUnitDrawerLegacy::DrawUnitIcons() const
{
	// draw unit icons and radar blips
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.05f);

	// A2C effectiveness is limited below four samples
	if (globalRendering->msaaLevel >= 4)
		glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);

	for (CUnit* u : modelDrawerData->GetUnsortedObjects()) {
		if (!u->isIcon)
			continue;

		const unsigned short closBits = (u->losStatus[gu->myAllyTeam] & (LOS_INLOS));
		const unsigned short plosBits = (u->losStatus[gu->myAllyTeam] & (LOS_PREVLOS | LOS_CONTRADAR));

		DrawIcon(u, !gu->spectatingFullView && closBits == 0 && plosBits != (LOS_PREVLOS | LOS_CONTRADAR));
	}

	glPopAttrib();
}

void CUnitDrawerLegacy::DrawUnitIconsScreen() const
{
	if (game->hideInterface && modelDrawerData->iconHideWithUI)
		return;

	// draw unit icons and radar blips
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.05f);

	CVertexArray* va = GetVertexArray();

	for (const auto& [icon, units] : modelDrawerData->GetUnitsByIcon())
	{

		if (icon == nullptr)
			continue;
		if (units.empty())
			continue;

		va->Initialize();
		va->EnlargeArrays(units.size() * 4, 0, VA_SIZE_2DTC);
		icon->BindTexture();

		for (const CUnit* unit : units)
		{
			if (unit->noDraw)
				continue;
			if (unit->IsInVoid())
				continue;
			if (unit->health <= 0 || unit->beingBuilt)
				continue;

			const unsigned short closBits = (unit->losStatus[gu->myAllyTeam] & (LOS_INLOS));
			const unsigned short plosBits = (unit->losStatus[gu->myAllyTeam] & (LOS_PREVLOS | LOS_CONTRADAR));

			assert(unit->myIcon == icon);
			DrawIconScreenArray(unit, icon, !gu->spectatingFullView && closBits == 0 && plosBits != (LOS_PREVLOS | LOS_CONTRADAR), modelDrawerData->iconZoomDist, va);
		}

		va->DrawArray2dTC(GL_QUADS);
	}
	glPopAttrib();
}

void CUnitDrawerLegacy::DrawObjectsShadow(int modelType) const
{
	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		// only need to bind the atlas once for 3DO's, but KISS
		assert((modelType != MODELTYPE_3DO) || (mdlRenderer.GetObjectBinKey(i) == 0));

		//shadowTexBindFuncs[modelType](textureHandlerS3O.GetTexture(mdlRenderer.GetObjectBinKey(i)));
		const auto* texMat = textureHandlerS3O.GetTexture(mdlRenderer.GetObjectBinKey(i));
		CModelDrawerHelper::modelDrawerHelpers[modelType]->BindShadowTex(texMat);

		for (auto* o : mdlRenderer.GetObjectBin(i)) {
			DrawUnitShadow(o);
		}

		CModelDrawerHelper::modelDrawerHelpers[modelType]->UnbindShadowTex();
	}
}

void CUnitDrawerLegacy::DrawOpaqueObjects(int modelType, bool drawReflection, bool drawRefraction) const
{
	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		for (auto* o : mdlRenderer.GetObjectBin(i)) {
			DrawOpaqueUnit(o, drawReflection, drawRefraction);
		}
	}
}

void CUnitDrawerLegacy::DrawAlphaObjects(int modelType) const
{
	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		for (auto* o : mdlRenderer.GetObjectBin(i)) {
			DrawAlphaUnit(o, modelType, false);
		}
	}

	// living and dead ghosted buildings
	if (!gu->spectatingFullView)
		DrawGhostedBuildings(modelType);
}

void CUnitDrawerLegacy::DrawOpaqueObjectsAux(int modelType) const
{
	const std::vector<CUnitDrawerData::TempDrawUnit>& tmpOpaqueUnits = modelDrawerData->GetTempOpaqueDrawUnits(modelType);

	// NOTE: not type-sorted
	for (const auto& unit : tmpOpaqueUnits) {
		if (!camera->InView(unit.pos, 100.0f))
			continue;

		DrawOpaqueAIUnit(unit);
	}
}

void CUnitDrawerLegacy::DrawAlphaObjectsAux(int modelType) const
{
	const std::vector<CUnitDrawerData::TempDrawUnit>& tmpAlphaUnits = modelDrawerData->GetTempAlphaDrawUnits(modelType);

	// NOTE: not type-sorted
	for (const auto& unit : tmpAlphaUnits) {
		if (!camera->InView(unit.pos, 100.0f))
			continue;

		DrawAlphaAIUnit(unit);
		DrawAlphaAIUnitBorder(unit);
	}
}

void CUnitDrawerLegacy::DrawGhostedBuildings(int modelType) const
{
	const auto& deadGhostedBuildings = modelDrawerData->GetDeadGhostBuildings(gu->myAllyTeam, modelType);
	const auto& liveGhostedBuildings = modelDrawerData->GetLiveGhostBuildings(gu->myAllyTeam, modelType);

	glColor4f(0.6f, 0.6f, 0.6f, IModelDrawerState::alphaValues.y);

	// buildings that died while ghosted
	for (GhostSolidObject* dgb : deadGhostedBuildings) {
		if (camera->InView(dgb->pos, dgb->model->GetDrawRadius())) {
			glPushMatrix();
			glTranslatef3(dgb->pos);
			glRotatef(dgb->facing * 90.0f, 0, 1, 0);

			CModelDrawerHelper::BindModelTypeTexture(modelType, dgb->model->textureType);
			SetTeamColor(dgb->team, float2(IModelDrawerState::alphaValues.y, 1.0f));

			dgb->model->DrawStatic();
			glPopMatrix();
			dgb->lastDrawFrame = globalRendering->drawFrame;
		}
	}

	for (CUnit* lgb : liveGhostedBuildings) {
		DrawAlphaUnit(lgb, modelType, true);
	}
}

void CUnitDrawerLegacy::DrawOpaqueUnit(CUnit* unit, bool drawReflection, bool drawRefraction) const
{
	if (!ShouldDrawOpaqueUnit(unit, drawReflection, drawRefraction))
		return;

	// draw the unit with the default (non-Lua) material
	SetTeamColor(unit->team);
	DrawUnitTrans(unit, 0, 0, false, false);
}

void CUnitDrawerLegacy::DrawUnitShadow(CUnit* unit) const
{
	if (ShouldDrawUnitShadow(unit))
		DrawUnitTrans(unit, 0, 0, false, false);
}

void CUnitDrawerLegacy::DrawAlphaUnit(CUnit* unit, int modelType, bool drawGhostBuildingsPass) const
{
	if (unit->drawFlag == 0)
		return;

	if (unit->HasDrawFlag(DrawFlags::SO_OPAQUE_FLAG))
		return;

	if (LuaObjectDrawer::AddAlphaMaterialObject(unit, LUAOBJ_UNIT))
		return;

	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];

	if (drawGhostBuildingsPass) {
		// check for decoy models
		const UnitDef* decoyDef = unit->unitDef->decoyDef;
		const S3DModel* model = nullptr;

		if (decoyDef == nullptr) {
			model = unit->model;
		}
		else {
			model = decoyDef->LoadModel();
		}

		// FIXME: needs a second pass
		if (model->type != modelType)
			return;

		// ghosted enemy units
		if (losStatus & LOS_CONTRADAR) {
			glColor4f(0.9f, 0.9f, 0.9f, IModelDrawerState::alphaValues.z);
		}
		else {
			glColor4f(0.6f, 0.6f, 0.6f, IModelDrawerState::alphaValues.y);
		}

		glPushMatrix();
		glTranslatef3(unit->drawPos);
		glRotatef(unit->buildFacing * 90.0f, 0, 1, 0);

		// the units in liveGhostedBuildings[modelType] are not
		// sorted by textureType, but we cannot merge them with
		// alphaModelRenderers[modelType] either since they are
		// not actually cloaked
		CModelDrawerHelper::BindModelTypeTexture(modelType, model->textureType);

		SetTeamColor(unit->team, float2((losStatus & LOS_CONTRADAR) ? IModelDrawerState::alphaValues.z : IModelDrawerState::alphaValues.y, 1.0f));
		model->DrawStatic();
		glPopMatrix();

		glColor4f(1.0f, 1.0f, 1.0f, IModelDrawerState::alphaValues.x);
		return;
	}

	if (unit->HasDrawFlag(DrawFlags::SO_DRICON_FLAG))
		return;

	if (unit->isIcon)
		return;

	if ((losStatus & LOS_INLOS) || gu->spectatingFullView) {
		SetTeamColor(unit->team, float2(IModelDrawerState::alphaValues.x, 1.0f));
		DrawUnitTrans(unit, 0, 0, false, false);
	}
}

void CUnitDrawerLegacy::DrawOpaqueAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const
{
	glPushMatrix();
	glTranslatef3(unit.pos);
	glRotatef(unit.rotation * math::RAD_TO_DEG, 0.0f, 1.0f, 0.0f);

	const UnitDef* def = unit.unitDef;
	const S3DModel* mdl = def->model;

	assert(mdl != nullptr);

	CModelDrawerHelper::BindModelTypeTexture(mdl->type, mdl->textureType);
	SetTeamColor(unit.team);
	mdl->DrawStatic();

	glPopMatrix();
}

void CUnitDrawerLegacy::DrawAlphaAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const
{
	glPushMatrix();
	glTranslatef3(unit.pos);
	glRotatef(unit.rotation * math::RAD_TO_DEG, 0.0f, 1.0f, 0.0f);

	const UnitDef* def = unit.unitDef;
	const S3DModel* mdl = def->model;

	assert(mdl != nullptr);

	CModelDrawerHelper::BindModelTypeTexture(mdl->type, mdl->textureType);
	SetTeamColor(unit.team, float2(IModelDrawerState::alphaValues.x, 1.0f));
	mdl->DrawStatic();

	glPopMatrix();
}

void CUnitDrawerLegacy::DrawAlphaAIUnitBorder(const CUnitDrawerData::TempDrawUnit& unit) const
{
	if (!unit.drawBorder)
		return;

	SetTeamColor(unit.team, float2(IModelDrawerState::alphaValues.w, 1.0f));

	const BuildInfo buildInfo(unit.unitDef, unit.pos, unit.facing);
	const float3 buildPos = CGameHelper::Pos2BuildPos(buildInfo, false);

	const float xsize = buildInfo.GetXSize() * (SQUARE_SIZE >> 1);
	const float zsize = buildInfo.GetZSize() * (SQUARE_SIZE >> 1);

	glColor4f(0.2f, 1, 0.2f, IModelDrawerState::alphaValues.w);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINE_STRIP);
	glVertexf3(buildPos + float3(xsize, 1.0f, zsize));
	glVertexf3(buildPos + float3(-xsize, 1.0f, zsize));
	glVertexf3(buildPos + float3(-xsize, 1.0f, -zsize));
	glVertexf3(buildPos + float3(xsize, 1.0f, -zsize));
	glVertexf3(buildPos + float3(xsize, 1.0f, zsize));
	glEnd();
	glColor4f(1.0f, 1.0f, 1.0f, IModelDrawerState::alphaValues.x);
	glEnable(GL_TEXTURE_2D);
}

void CUnitDrawerLegacy::DrawUnitMiniMapIcon(const CUnit* unit, CVertexArray* va) const
{
	if (unit->noMinimap)
		return;
	if (unit->myIcon == nullptr)
		return;
	if (unit->IsInVoid())
		return;

	const unsigned char defaultColor[4] = { 255, 255, 255, 255 };
	const unsigned char* color = &defaultColor[0];

	if (!unit->isSelected) {
		if (minimap->UseSimpleColors()) {
			if (unit->team == gu->myTeam) {
				color = minimap->GetMyTeamIconColor();
			}
			else if (teamHandler.Ally(gu->myAllyTeam, unit->allyteam)) {
				color = minimap->GetAllyTeamIconColor();
			}
			else {
				color = minimap->GetEnemyTeamIconColor();
			}
		}
		else {
			color = teamHandler.Team(unit->team)->color;
		}
	}

	const float iconScale = CUnitDrawerHelper::GetUnitIconScale(unit);
	const float3& iconPos = (!gu->spectatingFullView) ?
		unit->GetObjDrawErrorPos(gu->myAllyTeam) :
		unit->GetObjDrawMidPos();

	const float iconSizeX = (iconScale * minimap->GetUnitSizeX());
	const float iconSizeY = (iconScale * minimap->GetUnitSizeY());

	const float x0 = iconPos.x - iconSizeX;
	const float x1 = iconPos.x + iconSizeX;
	const float y0 = iconPos.z - iconSizeY;
	const float y1 = iconPos.z + iconSizeY;

	unit->myIcon->DrawArray(va, x0, y0, x1, y1, color);
}

void CUnitDrawerLegacy::DrawIcon(CUnit* unit, bool useDefaultIcon)
{
	// iconUnits should not never contain void-space units, see UpdateUnitIconState
	assert(!unit->IsInVoid());

	// If the icon is to be drawn as a radar blip, we want to get the default icon.
	const icon::CIconData* iconData = nullptr;

	if (useDefaultIcon) {
		iconData = icon::iconHandler.GetDefaultIconData();
	}
	else {
		iconData = unit->unitDef->iconType.GetIconData();
	}

	// drawMidPos is auto-calculated now; can wobble on its own as pieces move
	float3 pos = (!gu->spectatingFullView) ?
		unit->GetObjDrawErrorPos(gu->myAllyTeam) :
		unit->GetObjDrawMidPos();

	// make sure icon is above ground (needed before we calculate scale below)
	const float h = CGround::GetHeightReal(pos.x, pos.z, false);

	pos.y = std::max(pos.y, h);

	// Calculate the icon size. It scales with:
	//  * The square root of the camera distance.
	//  * The mod defined 'iconSize' (which acts a multiplier).
	//  * The unit radius, depending on whether the mod defined 'radiusadjust' is true or false.
	const float dist = std::min(8000.0f, fastmath::sqrt_builtin(camera->GetPos().SqDistance(pos)));
	const float iconScaleDist = 0.4f * fastmath::sqrt_builtin(dist); // makes far icons bigger
	float scale = iconData->GetSize() * iconScaleDist;

	if (iconData->GetRadiusAdjust() && !useDefaultIcon)
		scale *= (unit->radius / iconData->GetRadiusScale());

	// make sure icon is not partly under ground
	pos.y = std::max(pos.y, h + (unit->iconRadius = scale));

	// use white for selected units
	const uint8_t* colors[] = { teamHandler.Team(unit->team)->color, color4::white };
	const uint8_t* color = colors[unit->isSelected];

	glColor3ubv(color);

	// calculate the vertices
	const float3 dy = camera->GetUp() * scale;
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

void CUnitDrawerLegacy::DrawIconScreenArray(const CUnit* unit, const icon::CIconData* icon, bool useDefaultIcon, const float dist, CVertexArray* va) const
{
	// iconUnits should not never contain void-space units, see UpdateUnitIconState
	assert(!unit->IsInVoid());

	// drawMidPos is auto-calculated now; can wobble on its own as pieces move
	float3 pos = (!gu->spectatingFullView) ?
		unit->GetObjDrawErrorPos(gu->myAllyTeam) :
		unit->GetObjDrawMidPos();

	pos = camera->CalcWindowCoordinates(pos);
	if (pos.z > 1.0f || pos.z < 0.0f)
		return;

	// use white for selected units
	const uint8_t* srcColor = unit->isSelected ? color4::white : teamHandler.Team(unit->team)->color;
	uint8_t color[4] = { srcColor[0], srcColor[1], srcColor[2], 255 };

	float unitRadiusMult = icon->GetSize();
	if (icon->GetRadiusAdjust() && !useDefaultIcon)
		unitRadiusMult *= (unit->radius / icon->GetRadiusScale());
	unitRadiusMult = (unitRadiusMult - 1) * 0.75 + 1;

	// fade icons away in high zoom in levels
	if (!unit->isIcon)
		if (dist / unitRadiusMult < modelDrawerData->iconFadeVanish)
			return;
		else if (modelDrawerData->iconFadeVanish < modelDrawerData->iconFadeStart && dist / unitRadiusMult < modelDrawerData->iconFadeStart)
			// alpha range [64, 255], since icons is unrecognisable with alpha < 64
			color[3] = 64 + 191.0f * (dist / unitRadiusMult - modelDrawerData->iconFadeVanish) / (modelDrawerData->iconFadeStart - modelDrawerData->iconFadeVanish);

	// calculate the vertices
	const float offset = modelDrawerData->iconSizeBase / 2.0f * unitRadiusMult;

	const float x0 = (pos.x - offset) / globalRendering->viewSizeX;
	const float y0 = (pos.y + offset) / globalRendering->viewSizeY;
	const float x1 = (pos.x + offset) / globalRendering->viewSizeX;
	const float y1 = (pos.y - offset) / globalRendering->viewSizeY;

	if (x1 < 0 || x0 > 1 || y0 < 0 || y1 > 1)
		return; // don't try to draw outside the screen

	// Draw the icon.
	icon->DrawArray(va, x0, y0, x1, y1, color);
}


void CUnitDrawerLegacy::DrawUnitModelBeingBuiltShadow(const CUnit* unit, bool noLuaCall) const
{
	const float3 stageBounds = { 0.0f, unit->model->CalcDrawHeight(), unit->buildProgress };

	// draw-height defaults to maxs.y - mins.y, but can be overridden for non-3DO models
	// the default value derives from the model vertices and makes more sense to use here
	//
	// Both clip planes move up. Clip plane 0 is the upper bound of the model,
	// clip plane 1 is the lower bound. In other words, clip plane 0 makes the
	// wireframe/flat color/texture appear, and clip plane 1 then erases the
	// wireframe/flat color later on.
	const double upperPlanes[BuildStages::BUILDSTAGE_CNT][4] = {
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 1.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 2.0f)},
		{0.0f,  0.0f, 0.0f,                                                          0.0f },
	};
	const double lowerPlanes[BuildStages::BUILDSTAGE_CNT][4] = {
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 10.0f - 9.0f)},
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 3.0f  - 2.0f)},
		{0.0f,  1.0f, 0.0f,                                  (0.0f)},
		{0.0f,  0.0f, 0.0f,                                                           0.0f },
	};

	glPushAttrib(GL_CURRENT_BIT);

	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);

	if (stageBounds.z > 0.0f / 3.0f) {
		// wireframe, unconditional
		DrawModelWireBuildStageShadow(unit, upperPlanes[BUILDSTAGE_WIRE], lowerPlanes[BUILDSTAGE_WIRE], noLuaCall);
	}

	if (stageBounds.z > 1.0f / 3.0f) {
		// flat-colored, conditional
		DrawModelFlatBuildStageShadow(unit, upperPlanes[BUILDSTAGE_FLAT], lowerPlanes[BUILDSTAGE_FLAT], noLuaCall);
	}

	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE0);

	if (stageBounds.z > 2.0f / 3.0f) {
		// fully-shaded, conditional
		DrawModelFillBuildStageShadow(unit, upperPlanes[BUILDSTAGE_FILL], lowerPlanes[BUILDSTAGE_FILL], noLuaCall);
	}

	glPopAttrib();
}

void CUnitDrawerLegacy::DrawModelWireBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const
{
	if (globalRendering->amdHacks) {
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);
	} else {
		glPushMatrix();
		glLoadIdentity();
		glClipPlane(GL_CLIP_PLANE0, upperPlane);
		glClipPlane(GL_CLIP_PLANE1, lowerPlane);
		glPopMatrix();
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	DrawUnitModel(unit, noLuaCall);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if (globalRendering->amdHacks) {
		glEnable(GL_CLIP_PLANE0);
		glEnable(GL_CLIP_PLANE1);
	}
}

void CUnitDrawerLegacy::DrawModelFlatBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const
{
	glPushMatrix();
	glLoadIdentity();
	glClipPlane(GL_CLIP_PLANE0, upperPlane);
	glClipPlane(GL_CLIP_PLANE1, lowerPlane);
	glPopMatrix();

	DrawUnitModel(unit, noLuaCall);
}

void CUnitDrawerLegacy::DrawModelFillBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const
{
	DrawUnitModel(unit, noLuaCall);
}

void CUnitDrawerLegacy::DrawUnitModelBeingBuiltOpaque(const CUnit* unit, bool noLuaCall) const
{
	const S3DModel* model = unit->model;
	const    CTeam* team = teamHandler.Team(unit->team);
	const   SColor  color = team->color;

	const float wireColorMult = std::fabs(128.0f - ((gs->frameNum * 4) & 255)) / 255.0f + 0.5f;
	const float flatColorMult = 1.5f - wireColorMult;

	const float3 frameColors[2] = { unit->unitDef->nanoColor, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f} };
	const float3 stageColors[2] = { frameColors[globalRendering->teamNanospray], frameColors[globalRendering->teamNanospray] };
	const float3 stageBounds = { 0.0f, model->CalcDrawHeight(), unit->buildProgress };

	// draw-height defaults to maxs.y - mins.y, but can be overridden for non-3DO models
	// the default value derives from the model vertices and makes more sense to use here
	//
	// Both clip planes move up. Clip plane 0 is the upper bound of the model,
	// clip plane 1 is the lower bound. In other words, clip plane 0 makes the
	// wireframe/flat color/texture appear, and clip plane 1 then erases the
	// wireframe/flat color later on.
	const double upperPlanes[4][4] = {
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 1.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 2.0f)},
		{0.0f,  0.0f, 0.0f,                                                          0.0f },
	};
	const double lowerPlanes[4][4] = {
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 10.0f - 9.0f)},
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 3.0f - 2.0f)},
		{0.0f,  1.0f, 0.0f,                                  (0.0f)},
		{0.0f,  0.0f, 0.0f,                                                           0.0f },
	};

	glPushAttrib(GL_CURRENT_BIT);
	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);

	if (stageBounds.z > 0.0f / 3.0f) {
		// wireframe, unconditional
		SetNanoColor(float4(stageColors[0] * wireColorMult, 1.0f));
		DrawModelWireBuildStageOpaque(unit, upperPlanes[BUILDSTAGE_WIRE], lowerPlanes[BUILDSTAGE_WIRE], noLuaCall);
	}

	if (stageBounds.z > 1.0f / 3.0f) {
		// flat-colored, conditional
		SetNanoColor(float4(stageColors[1] * flatColorMult, 1.0f));
		DrawModelFlatBuildStageOpaque(unit, upperPlanes[BUILDSTAGE_WIRE], lowerPlanes[BUILDSTAGE_WIRE], noLuaCall);
	}

	glDisable(GL_CLIP_PLANE1);

	if (stageBounds.z > 2.0f / 3.0f) {
		// fully-shaded, conditional
		SetNanoColor(float4(1.0f, 1.0f, 1.0f, 0.0f)); // turn off
		DrawModelFillBuildStageOpaque(unit, upperPlanes[BUILDSTAGE_FILL], lowerPlanes[BUILDSTAGE_FILL], noLuaCall);
	}

	glDisable(GL_CLIP_PLANE0);
	glPopAttrib();
}

void CUnitDrawerLegacy::DrawModelWireBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const
{
	if (globalRendering->amdHacks) {
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);
	} else {
		glClipPlane(GL_CLIP_PLANE0, upperPlane);
		glClipPlane(GL_CLIP_PLANE1, lowerPlane);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	DrawUnitModel(unit, noLuaCall);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if (globalRendering->amdHacks) {
		glEnable(GL_CLIP_PLANE0);
		glEnable(GL_CLIP_PLANE1);
	}
}

void CUnitDrawerLegacy::DrawModelFlatBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const
{
	glClipPlane(GL_CLIP_PLANE0, upperPlane);
	glClipPlane(GL_CLIP_PLANE1, lowerPlane);

	DrawUnitModel(unit, noLuaCall);
}

void CUnitDrawerLegacy::DrawModelFillBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const
{
	if (globalRendering->amdHacks)
		glDisable(GL_CLIP_PLANE0);
	else
		glClipPlane(GL_CLIP_PLANE0, upperPlane);

	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);
	DrawUnitModel(unit, noLuaCall);
	glDisable(GL_POLYGON_OFFSET_FILL);
}

void CUnitDrawerLegacy::PushIndividualOpaqueState(const CUnit* unit, bool deferredPass) const { PushIndividualOpaqueState(unit->model, unit->team, deferredPass); }
void CUnitDrawerLegacy::PushIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass) const
{
	// these are not handled by Setup*Drawing but CGame
	// easier to assume they no longer have the correct
	// values at this point
	glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	SetupOpaqueDrawing(deferredPass);
	CModelDrawerHelper::PushModelRenderState(model);
	SetTeamColor(teamID);
}

void CUnitDrawerLegacy::PushIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass) const
{
	SetupAlphaDrawing(deferredPass);
	CModelDrawerHelper::PushModelRenderState(model);
	SetTeamColor(teamID, float2(IModelDrawerState::alphaValues.x, 1.0f));
}

void CUnitDrawerLegacy::PopIndividualOpaqueState(const CUnit* unit, bool deferredPass) const { PopIndividualOpaqueState(unit->model, unit->team, deferredPass); }
void CUnitDrawerLegacy::PopIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass) const
{
	CModelDrawerHelper::PopModelRenderState(model);
	ResetOpaqueDrawing(deferredPass);

	glPopAttrib();
}

void CUnitDrawerLegacy::PopIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass) const
{
	CModelDrawerHelper::PopModelRenderState(model);
	ResetAlphaDrawing(deferredPass);
}

void CUnitDrawerLegacy::DrawIndividual(const CUnit* unit, bool noLuaCall) const
{
	if (LuaObjectDrawer::DrawSingleObject(unit, LUAOBJ_UNIT /*, noLuaCall*/))
		return;

	// set the full default state
	PushIndividualOpaqueState(unit, false);
	DrawUnitTrans(unit, 0, 0, false, noLuaCall);
	PopIndividualOpaqueState(unit, false);
}

void CUnitDrawerLegacy::DrawIndividualNoTrans(const CUnit* unit, bool noLuaCall) const
{
	if (LuaObjectDrawer::DrawSingleObjectNoTrans(unit, LUAOBJ_UNIT /*, noLuaCall*/))
		return;

	PushIndividualOpaqueState(unit, false);
	DrawUnitNoTrans(unit, 0, 0, false, noLuaCall);
	PopIndividualOpaqueState(unit, false);
}

void CUnitDrawerLegacy::DrawIndividualDefOpaque(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen) const
{
	const S3DModel* model = objectDef->LoadModel();

	if (model == nullptr)
		return;

	if (!rawState) {
		if (!CModelDrawerHelper::DIDCheckMatrixMode(GL_MODELVIEW))
			return;

		// teamID validity is checked by SetTeamColor
		PushIndividualOpaqueState(model, teamID, false);

		// NOTE:
		//   unlike DrawIndividual(...) the model transform is
		//   always provided by Lua, not taken from the object
		//   (which does not exist here) so we must restore it
		//   (by undoing the UnitDrawerState MVP setup)
		//
		//   assumes the Lua transform includes a LoadIdentity!
		CModelDrawerHelper::DIDResetPrevProjection(toScreen);
		CModelDrawerHelper::DIDResetPrevModelView();
	}

	model->DrawStatic();

	if (!rawState) {
		PopIndividualOpaqueState(model, teamID, false);
	}
}

void CUnitDrawerLegacy::DrawIndividualDefAlpha(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen) const
{
	const S3DModel* model = objectDef->LoadModel();

	if (model == nullptr)
		return;

	if (!rawState) {
		if (!CModelDrawerHelper::DIDCheckMatrixMode(GL_MODELVIEW))
			return;

		PushIndividualAlphaState(model, teamID, false);

		CModelDrawerHelper::DIDResetPrevProjection(toScreen);
		CModelDrawerHelper::DIDResetPrevModelView();
	}

	model->DrawStatic();

	if (!rawState) {
		PopIndividualAlphaState(model, teamID, false);
	}
}


bool CUnitDrawerLegacy::ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command>& commands) const
{
	//TODO: make this a lua callin!
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	CFeature* feature = nullptr;

	std::vector<float3> buildableSquares; // buildable squares
	std::vector<float3> featureSquares; // occupied squares
	std::vector<float3> illegalSquares; // non-buildable squares

	const float3& pos = buildInfo.pos;
	const int x1 = pos.x - (buildInfo.GetXSize() * 0.5f * SQUARE_SIZE);
	const int x2 = x1 + (buildInfo.GetXSize() * SQUARE_SIZE);
	const int z1 = pos.z - (buildInfo.GetZSize() * 0.5f * SQUARE_SIZE);
	const int z2 = z1 + (buildInfo.GetZSize() * SQUARE_SIZE);
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
	}
	else {
		glColor4f(0.9f, 0.8f, 0.0f, 0.7f);
	}

	CVertexArray* va = GetVertexArray();
	va->Initialize();
	va->EnlargeArrays(buildableSquares.size() * 4, 0, VA_SIZE_0);

	for (uint32_t i = 0; i < buildableSquares.size(); i++) {
		va->AddVertexQ0(buildableSquares[i]);
		va->AddVertexQ0(buildableSquares[i] + float3(SQUARE_SIZE, 0, 0));
		va->AddVertexQ0(buildableSquares[i] + float3(SQUARE_SIZE, 0, SQUARE_SIZE));
		va->AddVertexQ0(buildableSquares[i] + float3(0, 0, SQUARE_SIZE));
	}
	va->DrawArray0(GL_QUADS);


	glColor4f(0.9f, 0.8f, 0.0f, 0.7f);
	va = GetVertexArray();
	va->Initialize();
	va->EnlargeArrays(featureSquares.size() * 4, 0, VA_SIZE_0);

	for (uint32_t i = 0; i < featureSquares.size(); i++) {
		va->AddVertexQ0(featureSquares[i]);
		va->AddVertexQ0(featureSquares[i] + float3(SQUARE_SIZE, 0, 0));
		va->AddVertexQ0(featureSquares[i] + float3(SQUARE_SIZE, 0, SQUARE_SIZE));
		va->AddVertexQ0(featureSquares[i] + float3(0, 0, SQUARE_SIZE));
	}
	va->DrawArray0(GL_QUADS);


	glColor4f(0.9f, 0.0f, 0.0f, 0.7f);
	va = GetVertexArray();
	va->Initialize();
	va->EnlargeArrays(illegalSquares.size() * 4, 0, VA_SIZE_0);

	for (uint32_t i = 0; i < illegalSquares.size(); i++) {
		va->AddVertexQ0(illegalSquares[i]);
		va->AddVertexQ0(illegalSquares[i] + float3(SQUARE_SIZE, 0, 0));
		va->AddVertexQ0(illegalSquares[i] + float3(SQUARE_SIZE, 0, SQUARE_SIZE));
		va->AddVertexQ0(illegalSquares[i] + float3(0, 0, SQUARE_SIZE));
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

	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	// glDisable(GL_BLEND);

	return canBuild;
}

/***********************************************************************/

void CUnitDrawerGL4::DrawObjectsShadow(int modelType) const
{
	DrawObjectsShadowComplete(modelType);
	//DrawObjectsShadowUnderCon(modelType);
}

void CUnitDrawerGL4::DrawObjectsShadowComplete(int modelType) const
{
	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		const auto* texMat = textureHandlerS3O.GetTexture(mdlRenderer.GetObjectBinKey(i));
		CModelDrawerHelper::modelDrawerHelpers[modelType]->BindShadowTex(texMat);

		const auto& bin = mdlRenderer.GetObjectBin(i);

		if (!mtModelDrawer || true) {
			for (auto* o : bin) {
				if (!ShouldDrawUnitShadow(o))
					continue;

				smv.AddToSubmission(o);
			}
		}
		else {
			static std::vector<const ObjType*> renderList;
			renderList.resize(bin.size());

			for_mt(0, bin.size(), [&bin, this](const int i) {
				renderList[i] = ShouldDrawUnitShadow(bin[i]) ? bin[i] : nullptr;
			});

			for (auto* o : renderList) {
				if (!o)
					continue;

				smv.AddToSubmission(o);
			}
		}
		smv.Submit(GL_TRIANGLES, false);

		CModelDrawerHelper::modelDrawerHelpers[modelType]->UnbindShadowTex();
	}

	smv.Unbind();
}

void CUnitDrawerGL4::DrawObjectsShadowUnderCon(int modelType) const
{

}


void CUnitDrawerGL4::DrawOpaqueObjects(int modelType, bool drawReflection, bool drawRefraction) const
{
	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	modelDrawerState->SetColorMultiplier();

	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	for (unsigned int i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		if (!mtModelDrawer || true) {
			for (auto* o : mdlRenderer.GetObjectBin(i)) {
				if (!ShouldDrawOpaqueUnit(o, drawReflection, drawRefraction))
					continue;

				smv.AddToSubmission(o);
			}
		}
		else {
			const auto& bin = mdlRenderer.GetObjectBin(i);
			static std::vector<const ObjType*> renderList;
			renderList.resize(bin.size());
			for_mt(0, renderList.size(), [this, &bin, drawReflection, drawRefraction](int k) {
				auto* o = bin[k];
				renderList[k] = ShouldDrawOpaqueUnit(o, drawReflection, drawRefraction) ? o : nullptr;
				});

			for (const auto* o : renderList)
				if (o)
					smv.AddToSubmission(o);

		}
		smv.Submit(GL_TRIANGLES, false);
	}

	smv.Unbind();
}

void CUnitDrawerGL4::DrawAlphaObjects(int modelType) const
{
	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	modelDrawerState->SetColorMultiplier(IModelDrawerState::alphaValues.x);
	//main cloaked alpha pass
	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		const auto& bin = mdlRenderer.GetObjectBin(i);

		if (!mtModelDrawer || true) {
			for (auto* o : bin) {
				if (!ShouldDrawAlphaUnit(o))
					continue;

				smv.AddToSubmission(o);
			}
		}
		else {
			static vector<const ObjType*> renderList;

			renderList.resize(bin.size());

			for_mt(0, bin.size(), [&bin, this](const int i) {
				renderList[i] = ShouldDrawAlphaUnit(bin[i]) ? bin[i] : nullptr;
				});

			for (auto* o : renderList) {
				if (!o)
					continue;

				smv.AddToSubmission(o);
			}
		}

		smv.Submit(GL_TRIANGLES, false);
	}

	// void CGLUnitDrawer::DrawGhostedBuildings(int modelType)
	if (gu->spectatingFullView)
		return;

	const auto& deadGhostBuildings = modelDrawerData->GetDeadGhostBuildings(gu->myAllyTeam, modelType);

	modelDrawerState->SetDrawingMode(ShaderDrawingModes::STATIC_MODEL);
	// deadGhostedBuildings
	{
		modelDrawerState->SetColorMultiplier(0.6f, 0.6f, 0.6f, IModelDrawerState::alphaValues.y);

		int prevModelType = -1;
		int prevTexType = -1;
		for (const auto* dgb : deadGhostBuildings) {
			if (!camera->InView(dgb->pos, dgb->model->GetDrawRadius()))
				continue;

			static CMatrix44f staticWorldMat;

			staticWorldMat.LoadIdentity();
			staticWorldMat.Translate(dgb->pos);

			staticWorldMat.RotateY(math::DEG_TO_RAD * 90.0f);

			if (prevModelType != modelType || prevTexType != dgb->model->textureType) {
				prevModelType = modelType; prevTexType = dgb->model->textureType;
				CModelDrawerHelper::BindModelTypeTexture(modelType, dgb->model->textureType); //ineficient rendering, but w/e
			}

			modelDrawerState->SetStaticModelMatrix(staticWorldMat);
			smv.SubmitImmediately(dgb->model, dgb->team); //need to submit immediately every model because of static per-model matrix
		}
	}

	// liveGhostedBuildings
	{
		const auto& liveGhostedBuildings = modelDrawerData->GetLiveGhostBuildings(gu->myAllyTeam, modelType);

		int prevModelType = -1;
		int prevTexType = -1;
		for (const auto* lgb : liveGhostedBuildings) {
			if (!camera->InView(lgb->pos, lgb->model->GetDrawRadius()))
				continue;

			// check for decoy models
			const UnitDef* decoyDef = lgb->unitDef->decoyDef;
			const S3DModel* model = nullptr;

			if (decoyDef == nullptr) {
				model = lgb->model;
			}
			else {
				model = decoyDef->LoadModel();
			}

			// FIXME: needs a second pass
			if (model->type != modelType)
				continue;

			static CMatrix44f staticWorldMat;

			staticWorldMat.LoadIdentity();
			staticWorldMat.Translate(lgb->pos);

			staticWorldMat.RotateY(math::DEG_TO_RAD * 90.0f);

			const unsigned short losStatus = lgb->losStatus[gu->myAllyTeam];

			// ghosted enemy units
			if (losStatus & LOS_CONTRADAR)
				modelDrawerState->SetColorMultiplier(0.9f, 0.9f, 0.9f, IModelDrawerState::alphaValues.z);
			else
				modelDrawerState->SetColorMultiplier(0.6f, 0.6f, 0.6f, IModelDrawerState::alphaValues.y);

			if (prevModelType != modelType || prevTexType != model->textureType) {
				prevModelType = modelType; prevTexType = model->textureType;
				CModelDrawerHelper::BindModelTypeTexture(modelType, model->textureType); //ineficient rendering, but w/e
			}

			modelDrawerState->SetStaticModelMatrix(staticWorldMat);
			smv.SubmitImmediately(model, lgb->team); //need to submit immediately every model because of static per-model matrix
		}
	}

	modelDrawerState->SetDrawingMode(); //reset is needed because other modelType's might be rendered afterwards
	smv.Unbind();
}

void CUnitDrawerGL4::DrawAlphaObjectsAux(int modelType) const
{
	const std::vector<CUnitDrawerData::TempDrawUnit>& tmpAlphaUnits = modelDrawerData->GetTempAlphaDrawUnits(modelType);
	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	modelDrawerState->SetDrawingMode(ShaderDrawingModes::STATIC_MODEL);

	// NOTE: not type-sorted
	for (const auto& unit : tmpAlphaUnits) {
		if (!camera->InView(unit.pos, 100.0f))
			continue;

		DrawAlphaAIUnit(unit);
		DrawAlphaAIUnitBorder(unit);
	}

	modelDrawerState->SetDrawingMode(); //reset is needed because other modelType's might be rendered afterwards
	smv.Unbind();
}

void CUnitDrawerGL4::DrawAlphaAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const
{
	static CMatrix44f staticWorldMat;

	staticWorldMat.LoadIdentity();
	staticWorldMat.Translate(unit.pos);

	staticWorldMat.RotateY(unit.rotation);

	auto& smv = S3DModelVAO::GetInstance(); //bound already

	const UnitDef* def = unit.unitDef;
	const S3DModel* mdl = def->model;

	assert(mdl != nullptr);

	CModelDrawerHelper::BindModelTypeTexture(mdl->type, mdl->textureType);

	SetTeamColor(unit.team, float2(IModelDrawerState::alphaValues.x, 1.0f));
	modelDrawerState->SetStaticModelMatrix(staticWorldMat);

	smv.SubmitImmediately(mdl, unit.team);
}

void CUnitDrawerGL4::DrawOpaqueObjectsAux(int modelType) const
{
	const std::vector<CUnitDrawerData::TempDrawUnit>& tmpOpaqueUnits = modelDrawerData->GetTempOpaqueDrawUnits(modelType);
	auto& smv = S3DModelVAO::GetInstance();

	modelDrawerState->SetDrawingMode(ShaderDrawingModes::STATIC_MODEL);

	// NOTE: not type-sorted
	for (const auto& unit : tmpOpaqueUnits) {
		if (!camera->InView(unit.pos, 100.0f))
			continue;

		DrawOpaqueAIUnit(unit);
	}

	modelDrawerState->SetDrawingMode(); //reset is needed because other modelType's might be rendered afterwards
}

void CUnitDrawerGL4::DrawOpaqueAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const
{
	static CMatrix44f staticWorldMat;

	staticWorldMat.LoadIdentity();
	staticWorldMat.Translate(unit.pos);

	staticWorldMat.RotateY(unit.rotation);

	auto& smv = S3DModelVAO::GetInstance(); //bound already

	const UnitDef* def = unit.unitDef;
	const S3DModel* mdl = def->model;

	assert(mdl != nullptr);

	CModelDrawerHelper::BindModelTypeTexture(mdl->type, mdl->textureType);

	SetTeamColor(unit.team);
	modelDrawerState->SetStaticModelMatrix(staticWorldMat);

	smv.SubmitImmediately(mdl, unit.team);
}

