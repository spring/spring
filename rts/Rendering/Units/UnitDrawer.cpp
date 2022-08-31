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
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/RenderBuffers.h"
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
#include "Rendering/Models/ModelsMemStorage.h"

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
	CUnitDrawer::InitInstance<CUnitDrawerGLSL>(MODEL_DRAWER_GLSL);
	CUnitDrawer::InitInstance<CUnitDrawerGL4 >(MODEL_DRAWER_GL4 );

	SelectImplementation();
}

bool CUnitDrawer::ShouldDrawOpaqueUnit(CUnit* u, uint8_t thisPassMask)
{
	if (u == ((thisPassMask == DrawFlags::SO_REFLEC_FLAG) ? nullptr : (gu->GetMyPlayer())->fpsController.GetControllee()))
		return false;

	assert(u);
	assert(u->model);

	if (u->drawFlag == 0)
		return false;

	if (u->GetIsIcon())
		return false;

	if (u->HasDrawFlag(DrawFlags::SO_ALPHAF_FLAG))
		return false;

	if (thisPassMask == DrawFlags::SO_REFLEC_FLAG && !u->HasDrawFlag(DrawFlags::SO_REFLEC_FLAG))
		return false;

	if (thisPassMask == DrawFlags::SO_REFRAC_FLAG && !u->HasDrawFlag(DrawFlags::SO_REFRAC_FLAG))
		return false;

	if (LuaObjectDrawer::AddOpaqueMaterialObject(u, LUAOBJ_UNIT))
		return false;

	if ((u->engineDrawMask & thisPassMask) != thisPassMask)
		return false;

	return true;
}

bool CUnitDrawer::ShouldDrawAlphaUnit(CUnit* u, uint8_t thisPassMask)
{
	assert(u);
	assert(u->model);

	if (u->drawFlag == 0)
		return false;

	if (u->GetIsIcon())
		return false;

	if (u->HasDrawFlag(DrawFlags::SO_OPAQUE_FLAG))
		return false;

	if (thisPassMask == DrawFlags::SO_REFLEC_FLAG && !u->HasDrawFlag(DrawFlags::SO_REFLEC_FLAG))
		return false;

	if (thisPassMask == DrawFlags::SO_REFRAC_FLAG && !u->HasDrawFlag(DrawFlags::SO_REFRAC_FLAG))
		return false;

	if (LuaObjectDrawer::AddAlphaMaterialObject(u, LUAOBJ_UNIT))
		return false;

	if ((u->engineDrawMask & thisPassMask) != thisPassMask)
		return false;

	return true;
}

bool CUnitDrawer::ShouldDrawUnitShadow(CUnit* u)
{
	assert(u);
	assert(u->model);

	static constexpr uint8_t thisPassMask = DrawFlags::SO_SHOPAQ_FLAG;

	if (!u->HasDrawFlag(DrawFlags::SO_SHOPAQ_FLAG))
		return false;

	if (LuaObjectDrawer::AddShadowMaterialObject(u, LUAOBJ_UNIT))
		return false;

	if ((u->engineDrawMask & thisPassMask) != thisPassMask)
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
	static auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DTC>();
	assert(rb.AssertSubmission());

	auto& sh = rb.GetShader();

	sh.Enable();
	sh.SetUniform("alphaCtrl", 0.0f, 1.0f, 0.0f, 0.0f); // GL_GREATER > 0.0

	static constexpr uint8_t defaultColor[4] = { 255, 255, 255, 255 };

	if (!minimap->UseUnitIcons())
		icon::iconHandler.GetDefaultIconData()->BindTexture();

	for (const auto& [icon, units] : modelDrawerData->GetUnitsByIcon()) {

		if (icon == nullptr)
			continue;
		if (units.empty())
			continue;

		if (minimap->UseUnitIcons())
			icon->BindTexture();

		for (const CUnit* unit : units) {
			assert(unit->myIcon == icon);
			if (unit->noMinimap)
				continue;
			if (unit->myIcon == nullptr)
				continue;
			if (!unit->drawIcon)
				continue;
			if (unit->IsInVoid())
				continue;

			const uint8_t* color = &defaultColor[0];

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

			rb.AddQuadTriangles(
				{ x0, y0, 0.0f, 0.0f, color },
				{ x1, y0, 1.0f, 0.0f, color },
				{ x1, y1, 1.0f, 1.0f, color },
				{ x0, y1, 0.0f, 1.0f, color }
			);
		}

		rb.Submit(GL_TRIANGLES);
	}

	sh.SetUniform("alphaCtrl", 0.0f, 0.0f, 0.0f, 1.0f);
	sh.Disable();
	glBindTexture(GL_TEXTURE_2D, 0);
}

void CUnitDrawerLegacy::DrawUnitIcons() const
{
#if 0
	if (game->hideInterface && modelDrawerData->iconHideWithUI)
		return;
#endif

	// draw unit icons and radar blips
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	// A2C effectiveness is limited below four samples
	if (globalRendering->msaaLevel >= 4)
		glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);

	static auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_TC>();
	assert(rb.AssertSubmission());

	auto& sh = rb.GetShader();
	sh.Enable();
	sh.SetUniform("alphaCtrl", 0.05f, 1.0f, 0.0f, 0.0f); // GL_GREATER > 0.05

	for (const auto& [icon, units] : modelDrawerData->GetUnitsByIcon())
	{
		if (icon == nullptr)
			continue;
		if (units.empty())
			continue;

		icon->BindTexture();

		for (const CUnit* cu : units)
		{
			CUnit* unit = const_cast<CUnit*>(cu);

			if (!unit->GetIsIcon())
				continue;
			if (!unit->drawIcon)
				continue;

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
			float scale = icon->GetSize() * iconScaleDist;

			if (icon->GetRadiusAdjust() && icon != icon::iconHandler.GetDefaultIconData())
				scale *= (unit->radius / icon->GetRadiusScale());

			// make sure icon is not partly under ground
			pos.y = std::max(pos.y, h + (unit->iconRadius = scale));

			// use white for selected units
			const uint8_t* colors[] = { teamHandler.Team(unit->team)->color, color4::white };
			const uint8_t* color = colors[unit->isSelected];

			const float3 dy = camera->GetUp() * unit->iconRadius;
			const float3 dx = camera->GetRight() * unit->iconRadius;
			const float3 vn = pos - dx;
			const float3 vp = pos + dx;
			const float3 bl = vn - dy; // bottom-left
			const float3 br = vp - dy; // bottom-right
			const float3 tl = vn + dy; // top-left
			const float3 tr = vp + dy; // top-right

			rb.AddQuadTriangles(
				{ tl, 0.0f, 0.0f, color },
				{ tr, 1.0f, 0.0f, color },
				{ br, 1.0f, 1.0f, color },
				{ bl, 0.0f, 1.0f, color }
			);
		}
		rb.Submit(GL_TRIANGLES);
	}
	sh.SetUniform("alphaCtrl", 0.0f, 0.0f, 0.0f, 1.0f);
	sh.Disable();
	glBindTexture(GL_TEXTURE_2D, 0);

	glPopAttrib();
}

void CUnitDrawerLegacy::DrawUnitIconsScreen() const
{
	if (game->hideInterface && modelDrawerData->iconHideWithUI)
		return;

	// draw unit icons and radar blips
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	static auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DTC>();
	assert(rb.AssertSubmission());

	auto& sh = rb.GetShader();

	sh.Enable();
	sh.SetUniform("alphaCtrl", 0.05f, 1.0f, 0.0f, 0.0f); // GL_GREATER > 0.05

	for (const auto& [icon, units] : modelDrawerData->GetUnitsByIcon())
	{
		if (icon == nullptr)
			continue;
		if (units.empty())
			continue;

		icon->BindTexture();

		for (const CUnit* unit : units)
		{
			if (!unit->drawIcon)
				continue;

			assert(unit->myIcon == icon);
			// iconUnits should not never contain void-space units, see UpdateUnitIconState
			assert(!unit->IsInVoid());

			// drawMidPos is auto-calculated now; can wobble on its own as pieces move
			float3 pos = (!gu->spectatingFullView) ?
				unit->GetObjDrawErrorPos(gu->myAllyTeam) :
				unit->GetObjDrawMidPos();

			pos = camera->CalcWindowCoordinates(pos);
			if (pos.z > 1.0f || pos.z < 0.0f)
				continue;

			// use white for selected units
			SColor color = unit->isSelected ? color4::white : SColor{ teamHandler.Team(unit->team)->color };

			float unitRadiusMult = icon->GetSize();
			if (icon->GetRadiusAdjust() && icon != icon::iconHandler.GetDefaultIconData())
				unitRadiusMult *= (unit->radius / icon->GetRadiusScale());
			unitRadiusMult = (unitRadiusMult - 1) * 0.75 + 1;

			// fade icons away in high zoom in levels
			if (!unit->GetIsIcon()) {
				if (modelDrawerData->iconZoomDist / unitRadiusMult < modelDrawerData->iconFadeVanish)
					continue;
				else if (modelDrawerData->iconFadeVanish < modelDrawerData->iconFadeStart && modelDrawerData->iconZoomDist / unitRadiusMult < modelDrawerData->iconFadeStart)
					// alpha range [64, 255], since icons is unrecognisable with alpha < 64
					color.a = 64 + 191.0f * (modelDrawerData->iconZoomDist / unitRadiusMult - modelDrawerData->iconFadeVanish) / (modelDrawerData->iconFadeStart - modelDrawerData->iconFadeVanish);
			}

			// calculate the vertices
			const float offset = modelDrawerData->iconSizeBase / 2.0f * unitRadiusMult;

			const float x0 = (pos.x - offset) / globalRendering->viewSizeX;
			const float y0 = (pos.y + offset) / globalRendering->viewSizeY;
			const float x1 = (pos.x + offset) / globalRendering->viewSizeX;
			const float y1 = (pos.y - offset) / globalRendering->viewSizeY;

			if (x1 < 0 && x0 > 1 && y0 < 0 && y1 > 1)
				continue; // don't try to draw when totally outside the screen

			rb.AddQuadTriangles(
				{ x0, y0, 0.0f, 0.0f, color },
				{ x1, y0, 1.0f, 0.0f, color },
				{ x1, y1, 1.0f, 1.0f, color },
				{ x0, y1, 0.0f, 1.0f, color }
			);
		}

		rb.Submit(GL_TRIANGLES);
	}

	sh.SetUniform("alphaCtrl", 0.0f, 0.0f, 0.0f, 1.0f);
	sh.Disable();
	glBindTexture(GL_TEXTURE_2D, 0);
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
	const uint8_t thisPassMask =
		(1 - (drawReflection || drawRefraction)) * DrawFlags::SO_OPAQUE_FLAG +
		(drawReflection * DrawFlags::SO_REFLEC_FLAG) +
		(drawRefraction * DrawFlags::SO_REFRAC_FLAG);

	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		for (auto* o : mdlRenderer.GetObjectBin(i)) {
			DrawOpaqueUnit(o, thisPassMask);
		}
	}
}

void CUnitDrawerLegacy::DrawAlphaObjects(int modelType, bool drawReflection, bool drawRefraction) const
{
	const uint8_t thisPassMask =
		(1 - (drawReflection || drawRefraction)) * DrawFlags::SO_ALPHAF_FLAG +
		(drawReflection * DrawFlags::SO_REFLEC_FLAG) +
		(drawRefraction * DrawFlags::SO_REFRAC_FLAG);

	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		for (auto* o : mdlRenderer.GetObjectBin(i)) {
			DrawAlphaUnit(o, modelType, thisPassMask, false);
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
			SetTeamColor(dgb->team, IModelDrawerState::alphaValues.y);

			dgb->model->DrawStatic();
			glPopMatrix();
			dgb->lastDrawFrame = globalRendering->drawFrame;
		}
	}

	for (CUnit* lgb : liveGhostedBuildings) {
		DrawAlphaUnit(lgb, modelType, DrawFlags::SO_ALPHAF_FLAG, true);
	}
}

void CUnitDrawerLegacy::DrawOpaqueUnit(CUnit* unit, uint8_t thisPassMask) const
{
	if (!ShouldDrawOpaqueUnit(unit, thisPassMask))
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

void CUnitDrawerLegacy::DrawAlphaUnit(CUnit* unit, int modelType, uint8_t thisPassMask, bool drawGhostBuildingsPass) const
{
	if (!ShouldDrawAlphaUnit(unit, thisPassMask))
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

		SetTeamColor(unit->team, (losStatus & LOS_CONTRADAR) ? IModelDrawerState::alphaValues.z : IModelDrawerState::alphaValues.y);
		model->DrawStatic();
		glPopMatrix();

		glColor4f(1.0f, 1.0f, 1.0f, IModelDrawerState::alphaValues.x);
		return;
	}

	if (unit->GetIsIcon())
		return;

	if ((losStatus & LOS_INLOS) || gu->spectatingFullView) {
		SetTeamColor(unit->team, IModelDrawerState::alphaValues.x);
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
	SetTeamColor(unit.team, IModelDrawerState::alphaValues.x);
	mdl->DrawStatic();

	glPopMatrix();
}

void CUnitDrawerLegacy::DrawAlphaAIUnitBorder(const CUnitDrawerData::TempDrawUnit& unit) const
{
	if (!unit.drawBorder)
		return;

	SetTeamColor(unit.team, IModelDrawerState::alphaValues.w);

	const BuildInfo buildInfo(unit.unitDef, unit.pos, unit.facing);
	const float3 buildPos = CGameHelper::Pos2BuildPos(buildInfo, false);

	const float xsize = buildInfo.GetXSize() * (SQUARE_SIZE >> 1);
	const float zsize = buildInfo.GetZSize() * (SQUARE_SIZE >> 1);

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_C>();

	const SColor col = SColor{ 0.2f, 1.0f, 0.2f, IModelDrawerState::alphaValues.w };

	rb.AddVertices({
		{buildPos + float3( xsize, 1.0f,  zsize), col},
		{buildPos + float3(-xsize, 1.0f,  zsize), col},
		{buildPos + float3(-xsize, 1.0f, -zsize), col},
		{buildPos + float3( xsize, 1.0f, -zsize), col},
		{buildPos + float3( xsize, 1.0f,  zsize), col}
	});

	GLint progID = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &progID);

	auto& sh = rb.GetShader();
	sh.Enable();
	rb.DrawArrays(GL_LINE_STRIP);
	sh.Disable();

	if (progID > 0)
		glUseProgram(progID);

	glColor4f(1.0f, 1.0f, 1.0f, IModelDrawerState::alphaValues.x);
	glEnable(GL_TEXTURE_2D);
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
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f       )},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 1.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 2.0f)},
		{0.0f,  0.0f, 0.0f,                                                          0.0f },
	};
	const double lowerPlanes[BuildStages::BUILDSTAGE_CNT][4] = {
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 10.0f - 9.0f)},
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 3.0f  - 2.0f)},
		{0.0f,  1.0f, 0.0f,                                                           0.0f },
		{0.0f,  0.0f, 0.0f,                                                           0.0f },
	};

	glPushAttrib(GL_CURRENT_BIT);

	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);

	{
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
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f       )},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 1.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 2.0f)},
		{0.0f,  0.0f, 0.0f,                                                          0.0f },
	};
	const double lowerPlanes[4][4] = {
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 10.0f - 9.0f)},
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z *  3.0f - 2.0f)},
		{0.0f,  1.0f, 0.0f,                                                           0.0f },
		{0.0f,  0.0f, 0.0f,                                                           0.0f },
	};

	glPushAttrib(GL_CURRENT_BIT);
	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);

	{
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
		SetNanoColor(float4(1.0f, 1.0f, 1.0f, 0.0f));
		DrawModelFillBuildStageOpaque(unit, upperPlanes[BUILDSTAGE_FILL], lowerPlanes[BUILDSTAGE_FILL], noLuaCall);
	}

	SetNanoColor(float4(1.0f, 1.0f, 1.0f, 0.0f)); // turn off in any case
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
	SetTeamColor(teamID, IModelDrawerState::alphaValues.x);
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
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

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

	static constexpr float buildColorT[4]  = { 0.0f, 0.9f, 0.0f, 0.7f };
	static constexpr float buildColorF[4]  = { 0.9f, 0.8f, 0.0f, 0.7f };
	static constexpr float featureColor[4] = { 0.9f, 0.8f, 0.0f, 0.7f };
	static constexpr float illegalColor[4] = { 0.9f, 0.0f, 0.0f, 0.7f };

	static auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_C>();
	assert(rb.AssertSubmission());

	auto& sh = rb.GetShader();

	sh.Enable();

	const float* color = canBuild ? &buildColorT[0] : &buildColorF[0];
	for (const auto& buildableSquare : buildableSquares) {
		rb.AddQuadLines(
			{ buildableSquare                                      , color },
			{ buildableSquare + float3(SQUARE_SIZE, 0, 0          ), color },
			{ buildableSquare + float3(SQUARE_SIZE, 0, SQUARE_SIZE), color },
			{ buildableSquare + float3(0          , 0, SQUARE_SIZE), color }
		);
	}

	color = &featureColor[0];
	for (const auto& featureSquare : featureSquares) {
		rb.AddQuadLines(
			{ featureSquare                                      , color },
			{ featureSquare + float3(SQUARE_SIZE, 0, 0          ), color },
			{ featureSquare + float3(SQUARE_SIZE, 0, SQUARE_SIZE), color },
			{ featureSquare + float3(0          , 0, SQUARE_SIZE), color }
		);
	}

	color = &illegalColor[0];
	for (const auto& illegalSquare : illegalSquares) {
		rb.AddQuadLines(
			{ illegalSquare                                      , color },
			{ illegalSquare + float3(SQUARE_SIZE, 0, 0          ), color },
			{ illegalSquare + float3(SQUARE_SIZE, 0, SQUARE_SIZE), color },
			{ illegalSquare + float3(0          , 0, SQUARE_SIZE), color }
		);
	}
	rb.Submit(GL_LINES);

	if (h < 0.0f) {
		constexpr uint8_t s[4] = { 0,   0, 255, 128 }; // start color
		constexpr uint8_t e[4] = { 0, 128, 255, 255 }; // end color

		rb.AddVertex({ float3(x1, h, z1), s }); rb.AddVertex({ float3(x1, 0.f, z1), e });
		rb.AddVertex({ float3(x1, h, z2), s }); rb.AddVertex({ float3(x1, 0.f, z2), e });
		rb.AddVertex({ float3(x2, h, z2), s }); rb.AddVertex({ float3(x2, 0.f, z2), e });
		rb.AddVertex({ float3(x2, h, z1), s }); rb.AddVertex({ float3(x2, 0.f, z1), e });
		rb.Submit(GL_LINES);

		rb.AddVertex({ float3(x1, 0.0f, z1), e });
		rb.AddVertex({ float3(x1, 0.0f, z2), e });
		rb.AddVertex({ float3(x2, 0.0f, z2), e });
		rb.AddVertex({ float3(x2, 0.0f, z1), e });
		rb.Submit(GL_LINE_LOOP);
	}

	sh.Disable();

	glEnable(GL_DEPTH_TEST);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	// glDisable(GL_BLEND);

	return canBuild;
}

void CUnitDrawerLegacy::DrawBuildIcons(const std::vector<CCursorIcons::BuildIcon>& buildIcons) const
{
	if (buildIcons.empty())
		return;

	glEnable(GL_DEPTH_TEST);
	glColor4f(1.0f, 1.0f, 1.0f, 0.3f);

	for (const auto& buildIcon : buildIcons) {
		const auto* unitDef = unitDefHandler->GetUnitDefByID(-(buildIcon.cmd));
		assert(unitDef);

		const auto* model = unitDef->LoadModel();
		assert(model);

		if (!camera->InView(buildIcon.pos, model->GetDrawRadius()))
			continue;

		glPushMatrix();
		glLoadIdentity();
		glTranslatef3(buildIcon.pos);
		glRotatef(buildIcon.facing * 90.0f, 0.0f, 1.0f, 0.0f);

		unitDrawer->DrawIndividualDefAlpha(unitDef, buildIcon.team, false);

		glPopMatrix();
	}

	glDisable(GL_DEPTH_TEST);
}

/***********************************************************************/

// CUnitDrawerLegacy::DrawBuildIcons is seemingly unbeatable in terms of FPS ?
void CUnitDrawerGL4::DrawBuildIcons(const std::vector<CCursorIcons::BuildIcon>& buildIcons) const
{
	if (buildIcons.empty())
		return;

	glEnable(GL_DEPTH_TEST);
	SetupAlphaDrawing(false);

	const auto oldMM = modelDrawerState->SetMatrixMode(ShaderMatrixModes::STATIC_MATMODE);
	const auto oldSM = modelDrawerState->SetShadingMode(ShaderShadingModes::SKIP_SHADING);
	modelDrawerState->SetTeamColor(0);
	modelDrawerState->SetColorMultiplier(0.9f, 0.9f, 0.9f, 0.3f);

	int prevModelType = -1;
	int prevTexType = -1;

	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	for (const auto& buildIcon : buildIcons) {
		const auto* unitDef = unitDefHandler->GetUnitDefByID(-(buildIcon.cmd));
		assert(unitDef);

		const auto* model = unitDef->LoadModel();
		assert(model);

		if (!camera->InView(buildIcon.pos, model->GetDrawRadius()))
			continue;

		CMatrix44f staticWorldMat;

		staticWorldMat.Translate(buildIcon.pos);
		staticWorldMat.RotateY(-buildIcon.facing * math::DEG_TO_RAD * 90.0f);

		modelDrawerState->SetStaticModelMatrix(staticWorldMat);

		if (prevModelType != model->type || prevTexType != model->textureType) {
			if (prevModelType != -1)
				CModelDrawerHelper::PopModelRenderState(prevModelType);

			prevModelType = model->type; prevTexType = model->textureType;
			CModelDrawerHelper::PushModelRenderState(model->type);
			CModelDrawerHelper::BindModelTypeTexture(model->type, model->textureType); //ineficient rendering, but w/e
		}

		smv.SubmitImmediately(model, buildIcon.team, DrawFlags::SO_ALPHAF_FLAG);
	}

	if (prevModelType != -1)
		CModelDrawerHelper::PopModelRenderState(prevModelType);

	modelDrawerState->SetColorMultiplier();
	modelDrawerState->SetMatrixMode(oldMM);
	modelDrawerState->SetShadingMode(oldSM);

	smv.Unbind();

	ResetAlphaDrawing(false);
	glDisable(GL_DEPTH_TEST);
}


void CUnitDrawerGL4::DrawObjectsShadow(int modelType) const
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

		static vector<const ObjType*> beingBuilt;
		beingBuilt.clear();

		for (auto* o : bin) {
			if (!ShouldDrawUnitShadow(o))
				continue;

			if (o->beingBuilt && o->unitDef->showNanoFrame) {
				beingBuilt.emplace_back(o);
				continue;
			}

			smv.AddToSubmission(o);
		}

		smv.Submit(GL_TRIANGLES, false);

		for (auto* o : beingBuilt) {
			DrawUnitModelBeingBuiltShadow(o, false);
		}

		CModelDrawerHelper::modelDrawerHelpers[modelType]->UnbindShadowTex();
	}

	smv.Unbind();
}

void CUnitDrawerGL4::DrawOpaqueObjects(int modelType, bool drawReflection, bool drawRefraction) const
{
	const uint8_t thisPassMask =
		(1 - (drawReflection || drawRefraction)) * DrawFlags::SO_OPAQUE_FLAG +
		(drawReflection * DrawFlags::SO_REFLEC_FLAG) +
		(drawRefraction * DrawFlags::SO_REFRAC_FLAG);

	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	SetTeamColor(0, 1.0f);
	modelDrawerState->SetColorMultiplier();

	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	for (unsigned int i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		static vector<const ObjType*> beingBuilt;
		beingBuilt.clear();

		for (auto* o : mdlRenderer.GetObjectBin(i)) {
			if (!ShouldDrawOpaqueUnit(o, thisPassMask))
				continue;

			if (o->beingBuilt && o->unitDef->showNanoFrame) {
				beingBuilt.emplace_back(o);
				continue;
			}

			smv.AddToSubmission(o);
		}

		smv.Submit(GL_TRIANGLES, false);

		for (auto* o : beingBuilt) {
			DrawUnitModelBeingBuiltOpaque(o, false);
		}
	}

	smv.Unbind();
}

void CUnitDrawerGL4::DrawAlphaObjects(int modelType, bool drawReflection, bool drawRefraction) const
{
	const uint8_t thisPassMask =
		(1 - (drawReflection || drawRefraction)) * DrawFlags::SO_ALPHAF_FLAG +
		(drawReflection * DrawFlags::SO_REFLEC_FLAG) +
		(drawRefraction * DrawFlags::SO_REFRAC_FLAG);

	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	//some magical constant that equalizes alpha with GLSL drawer, the origin of this difference is unknown
	modelDrawerState->SetColorMultiplier(0.6f);
	modelDrawerState->SetTeamColor(0, IModelDrawerState::alphaValues.x); //teamID doesn't matter here
	//main cloaked alpha pass
	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		const auto& bin = mdlRenderer.GetObjectBin(i);

		for (auto* o : bin) {
			if (!ShouldDrawAlphaUnit(o, thisPassMask))
				continue;

			smv.AddToSubmission(o);
		}

		smv.Submit(GL_TRIANGLES, false);
	}

	// void CGLUnitDrawer::DrawGhostedBuildings(int modelType)
	if (gu->spectatingFullView)
		return;

	const auto& deadGhostBuildings = modelDrawerData->GetDeadGhostBuildings(gu->myAllyTeam, modelType);

	const auto oldMM = modelDrawerState->SetMatrixMode(ShaderMatrixModes::STATIC_MATMODE);
	// deadGhostedBuildings
	{
		modelDrawerState->SetColorMultiplier(0.6f, 0.6f, 0.6f, IModelDrawerState::alphaValues.y);
		modelDrawerState->SetTeamColor(0, IModelDrawerState::alphaValues.y); //teamID doesn't matter here

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
			smv.SubmitImmediately(dgb->model, dgb->team, DrawFlags::SO_ALPHAF_FLAG); //need to submit immediately every model because of static per-model matrix
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

			staticWorldMat.RotateY(-lgb->buildFacing * math::DEG_TO_RAD * 90.0f);

			const unsigned short losStatus = lgb->losStatus[gu->myAllyTeam];

			// ghosted enemy units
			if (losStatus & LOS_CONTRADAR) {
				modelDrawerState->SetColorMultiplier(0.9f, 0.9f, 0.9f, IModelDrawerState::alphaValues.z);
				modelDrawerState->SetTeamColor(lgb->team, IModelDrawerState::alphaValues.z);
			}
			else {
				modelDrawerState->SetColorMultiplier(0.6f, 0.6f, 0.6f, IModelDrawerState::alphaValues.y);
				modelDrawerState->SetTeamColor(lgb->team, IModelDrawerState::alphaValues.y);
			}

			if (prevModelType != modelType || prevTexType != model->textureType) {
				prevModelType = modelType; prevTexType = model->textureType;
				CModelDrawerHelper::BindModelTypeTexture(modelType, model->textureType); //ineficient rendering, but w/e
			}

			modelDrawerState->SetStaticModelMatrix(staticWorldMat);
			smv.SubmitImmediately(model, lgb->team, DrawFlags::SO_ALPHAF_FLAG); //need to submit immediately every model because of static per-model matrix
		}
	}

	modelDrawerState->SetColorMultiplier(IModelDrawerState::alphaValues.x);
	modelDrawerState->SetMatrixMode(oldMM); //reset is needed because other modelType's might be rendered afterwards
	smv.Unbind();
}

void CUnitDrawerGL4::DrawAlphaObjectsAux(int modelType) const
{
	const std::vector<CUnitDrawerData::TempDrawUnit>& tmpAlphaUnits = modelDrawerData->GetTempAlphaDrawUnits(modelType);
	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	const auto oldMM = modelDrawerState->SetMatrixMode(ShaderMatrixModes::STATIC_MATMODE);

	// NOTE: not type-sorted
	for (const auto& unit : tmpAlphaUnits) {
		if (!camera->InView(unit.pos, 100.0f))
			continue;

		DrawAlphaAIUnit(unit);
		DrawAlphaAIUnitBorder(unit);
	}

	modelDrawerState->SetMatrixMode(oldMM); //reset is needed because other modelType's might be rendered afterwards
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

	SetTeamColor(unit.team, IModelDrawerState::alphaValues.x);
	modelDrawerState->SetStaticModelMatrix(staticWorldMat);

	smv.SubmitImmediately(mdl, unit.team, DrawFlags::SO_ALPHAF_FLAG);
}

void CUnitDrawerGL4::DrawOpaqueObjectsAux(int modelType) const
{
	const std::vector<CUnitDrawerData::TempDrawUnit>& tmpOpaqueUnits = modelDrawerData->GetTempOpaqueDrawUnits(modelType);
	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	const auto oldMM = modelDrawerState->SetMatrixMode(ShaderMatrixModes::STATIC_MATMODE);

	// NOTE: not type-sorted
	for (const auto& unit : tmpOpaqueUnits) {
		if (!camera->InView(unit.pos, 100.0f))
			continue;

		DrawOpaqueAIUnit(unit);
	}

	modelDrawerState->SetMatrixMode(oldMM); //reset is needed because other modelType's might be rendered afterwards

	smv.Unbind();
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

	smv.SubmitImmediately(mdl, unit.team, DrawFlags::SO_OPAQUE_FLAG);
}

void CUnitDrawerGL4::DrawUnitModelBeingBuiltShadow(const CUnit* unit, bool noLuaCall) const
{
	auto& smv = S3DModelVAO::GetInstance();

	const float3 stageBounds = { 0.0f, unit->model->CalcDrawHeight(), unit->buildProgress };

	const float4 upperPlanes[] = {
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f       )},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 1.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 2.0f)},
		{0.0f,  0.0f, 0.0f,                                                          0.0f },
	};
	const float4 lowerPlanes[] = {
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 10.0f - 9.0f)},
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 3.0f  - 2.0f)},
		{0.0f,  1.0f, 0.0f,                                                           0.0f },
		{0.0f,  0.0f, 0.0f,                                                           0.0f },
	};

	Shader::IProgramObject* po = shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL_GL4);
	assert(po);
	assert(po->IsBound());

	glPushAttrib(GL_POLYGON_BIT);

	glEnable(GL_CLIP_DISTANCE0);
	glEnable(GL_CLIP_DISTANCE1);

	const auto SetClipPlane = [po](uint8_t idx, const float4& cp = float4{ 0.0f, 0.0f, 0.0f, 1.0f }) {
		switch (idx)
		{
		case 0: //upper construction clip plane
			po->SetUniform("clipPlane0", cp.x, cp.y, cp.z, cp.w);
			break;
		case 1: //lower construction clip plane
			po->SetUniform("clipPlane1", cp.x, cp.y, cp.z, cp.w);
			break;
		default:
			assert(false);
			break;
		}
	};

	{
		// wireframe, unconditional
		SetClipPlane(0, upperPlanes[BUILDSTAGE_WIRE]);
		SetClipPlane(1, lowerPlanes[BUILDSTAGE_WIRE]);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		smv.SubmitImmediately(unit, GL_TRIANGLES);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (stageBounds.z > 1.0f / 3.0f) {
		// flat-colored, conditional
		SetClipPlane(0, upperPlanes[BUILDSTAGE_FLAT]);
		SetClipPlane(1, lowerPlanes[BUILDSTAGE_FLAT]);

		smv.SubmitImmediately(unit, GL_TRIANGLES);
	}

	SetClipPlane(0); //default
	SetClipPlane(1); //default;

	glDisable(GL_CLIP_DISTANCE1);
	glDisable(GL_CLIP_DISTANCE0);

	if (stageBounds.z > 2.0f / 3.0f) {
		// fully-shaded, conditional
		smv.SubmitImmediately(unit, GL_TRIANGLES);
	}

	glPopAttrib();
}

void CUnitDrawerGL4::DrawUnitModelBeingBuiltOpaque(const CUnit* unit, bool noLuaCall) const
{
	auto& smv = S3DModelVAO::GetInstance();

	const    CTeam* team = teamHandler.Team(unit->team);
	const   SColor  color = team->color;

	const float wireColorMult = std::fabs(128.0f - ((gs->frameNum * 4) & 255)) / 255.0f + 0.5f;
	const float flatColorMult = 1.5f - wireColorMult;

	const float3 frameColors[2] = { unit->unitDef->nanoColor, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f} };
	const float3 stageColors[2] = { frameColors[globalRendering->teamNanospray], frameColors[globalRendering->teamNanospray] };


	const float3 stageBounds = { 0.0f, unit->model->CalcDrawHeight(), unit->buildProgress };

	// draw-height defaults to maxs.y - mins.y, but can be overridden for non-3DO models
	// the default value derives from the model vertices and makes more sense to use here
	//
	// Both clip planes move up. Clip plane 0 is the upper bound of the model,
	// clip plane 1 is the lower bound. In other words, clip plane 0 makes the
	// wireframe/flat color/texture appear, and clip plane 1 then erases the
	// wireframe/flat color later on.
	const float4 upperPlanes[] = {
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f       )},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 1.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 2.0f)},
		{0.0f,  0.0f, 0.0f,                                                          0.0f },
	};
	const float4 lowerPlanes[] = {
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 10.0f - 9.0f)},
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z *  3.0f - 2.0f)},
		{0.0f,  1.0f, 0.0f,                                  (                        0.0f)},
		{0.0f,  0.0f, 0.0f,                                                           0.0f },
	};

	glPushAttrib(GL_POLYGON_BIT);

	glEnable(GL_CLIP_DISTANCE0);
	glEnable(GL_CLIP_DISTANCE1);

	{
		// wireframe, unconditional
		SetNanoColor(float4(stageColors[0] * wireColorMult, 1.0f));
		modelDrawerState->SetClipPlane(0, upperPlanes[BUILDSTAGE_WIRE]);
		modelDrawerState->SetClipPlane(1, lowerPlanes[BUILDSTAGE_WIRE]);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		smv.SubmitImmediately(unit, GL_TRIANGLES);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (stageBounds.z > 1.0f / 3.0f) {
		// flat-colored, conditional
		SetNanoColor(float4(stageColors[1] * flatColorMult, 1.0f));
		modelDrawerState->SetClipPlane(0, upperPlanes[BUILDSTAGE_FLAT]);
		modelDrawerState->SetClipPlane(1, lowerPlanes[BUILDSTAGE_FLAT]);

		smv.SubmitImmediately(unit, GL_TRIANGLES);
	}

	modelDrawerState->SetClipPlane(1); //default;
	glDisable(GL_CLIP_DISTANCE1);

	if (stageBounds.z > 2.0f / 3.0f) {
		// fully-shaded, conditional
		glPolygonOffset(1.0f, 1.0f);
		glEnable(GL_POLYGON_OFFSET_FILL);
		SetNanoColor(float4(1.0f, 1.0f, 1.0f, 0.0f));
		modelDrawerState->SetClipPlane(0, upperPlanes[BUILDSTAGE_FLAT]);

		smv.SubmitImmediately(unit, GL_TRIANGLES);

		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	SetNanoColor(float4(1.0f, 1.0f, 1.0f, 0.0f)); // turn off in any case
	modelDrawerState->SetClipPlane(0); //default
	glDisable(GL_CLIP_DISTANCE0);

	glPopAttrib();
}

