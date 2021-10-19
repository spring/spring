/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <vector>
#include <array>

#include "Rendering/Common/ModelDrawer.h"
#include "Rendering/Common/ModelDrawerState.hpp"
#include "Rendering/Units/UnitDrawerData.h"
#include "Rendering/GL/LightHandler.h"
#include "System/type2.h"
#include "Sim/Units/CommandAI/Command.h"

class CSolidObject;
class CUnit;
struct S3DModel;
struct SolidObjectDef;

namespace Shader { struct IProgramObject; }

class CUnitDrawer : public CModelDrawerBase<CUnitDrawerData, CUnitDrawer>
{
public:
	static void InitStatic();
	//static void KillStatic(bool reload); //use base
	//static void UpdateStatic(); //use base
public:
	// Interface with CUnitDrawerData
	static void UpdateGhostedBuildings() { modelDrawerData->UpdateGhostedBuildings(); }

	static uint32_t GetUnitDefImage(const UnitDef* ud) { return modelDrawerData->GetUnitDefImage(ud); }
	static void SetUnitDefImage(const UnitDef* unitDef, const std::string& texName) { return modelDrawerData->SetUnitDefImage(unitDef, texName); }
	static void SetUnitDefImage(const UnitDef* unitDef, uint32_t texID, int xsize, int ysize) { return modelDrawerData->SetUnitDefImage(unitDef, texID, xsize, ysize); }

	static bool& UseScreenIcons() { return modelDrawerData->useScreenIcons; }

	static float GetUnitIconFadeStart() { return modelDrawerData->GetUnitIconFadeStart(); }
	static void SetUnitIconFadeStart(float scale) { modelDrawerData->SetUnitIconFadeStart(scale); }

	static float GetUnitIconScaleUI() { return modelDrawerData->GetUnitIconScaleUI(); }
	static void SetUnitIconScaleUI(float scale) { modelDrawerData->SetUnitIconScaleUI(scale); }

	static float GetUnitDrawDist() { return CModelDrawerDataConcept::modelDrawDist; }
	static void SetModelDrawDist(float dist) { CModelDrawerDataConcept::SetModelDrawDist(dist); }

	static float GetUnitIconDist(float dist) { return modelDrawerData->unitIconDist; }
	static void SetUnitIconDist(float dist) { modelDrawerData->SetUnitIconDist(dist); }

	static float GetUnitIconFadeVanish() { return modelDrawerData->iconFadeVanish; }
	static void SetUnitIconFadeVanish(float dist) { modelDrawerData->SetUnitIconFadeVanish(dist); }

	static bool& IconHideWithUI() { return modelDrawerData->iconHideWithUI; }

	static void AddTempDrawUnit(const CUnitDrawerData::TempDrawUnit& tempDrawUnit) { modelDrawerData->AddTempDrawUnit(tempDrawUnit); }

	static const std::vector<CUnit*>& GetUnsortedUnits() { return modelDrawerData->GetUnsortedObjects(); }
public:
	// DrawUnit*
	virtual void DrawUnitModel(const CUnit* unit, bool noLuaCall) const = 0;
	virtual void DrawUnitModelBeingBuiltShadow(const CUnit* unit, bool noLuaCall) const = 0;
	virtual void DrawUnitModelBeingBuiltOpaque(const CUnit* unit, bool noLuaCall) const = 0;
	virtual void DrawUnitNoTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const = 0;
	virtual void DrawUnitTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const = 0;
	virtual void DrawIndividual(const CUnit* unit, bool noLuaCall) const = 0;
	virtual void DrawIndividualNoTrans(const CUnit* unit, bool noLuaCall) const = 0;

	// DrawIndividualDef*
	virtual void DrawIndividualDefOpaque(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const = 0;
	virtual void DrawIndividualDefAlpha(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const = 0;

	// Icons Minimap
	virtual void DrawUnitMiniMapIcons() const = 0;
	        void UpdateUnitDefMiniMapIcons(const UnitDef* ud) { modelDrawerData->UpdateUnitDefMiniMapIcons(ud); }

	// Icons Map
	virtual void DrawUnitIcons() const = 0;
	virtual void DrawUnitIconsScreen() const = 0;

	// Build Squares
	        bool ShowUnitBuildSquare(const BuildInfo& buildInfo) const { return ShowUnitBuildSquare(buildInfo, std::vector<Command>()); }
	virtual bool ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command>& commands) const = 0;
protected:
	static bool ShouldDrawOpaqueUnit(CUnit* u, bool drawReflection, bool drawRefraction);
	static bool ShouldDrawAlphaUnit(CUnit* u);
	static bool ShouldDrawUnitShadow(CUnit* u);

	virtual void DrawGhostedBuildings(int modelType) const = 0;
private:
	inline static std::array<CUnitDrawer*, ModelDrawerTypes::MODEL_DRAWER_CNT> unitDrawers = {};
public:
	enum BuildStages {
		BUILDSTAGE_WIRE = 0,
		BUILDSTAGE_FLAT = 1,
		BUILDSTAGE_FILL = 2,
		BUILDSTAGE_NONE = 3,
		BUILDSTAGE_CNT = 4,
	};
	enum ModelShaderProgram {
		MODEL_SHADER_NOSHADOW_STANDARD = 0, ///< model shader (V+F) without self-shadowing
		MODEL_SHADER_SHADOWED_STANDARD = 1, ///< model shader (V+F) with    self-shadowing
		MODEL_SHADER_NOSHADOW_DEFERRED = 2, ///< deferred version of MODEL_SHADER_NOSHADOW (GLSL-only)
		MODEL_SHADER_SHADOWED_DEFERRED = 3, ///< deferred version of MODEL_SHADER_SHADOW   (GLSL-only)
		MODEL_SHADER_COUNT = 4,
	};
};

class CUnitDrawerBase : public CUnitDrawer {
public:
	void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const override {
		DrawOpaquePassImpl<LuaObjType::LUAOBJ_UNIT>(deferredPass, drawReflection, drawRefraction);
	};
	void DrawAlphaPass() const override {
		DrawAlphaPassImpl<LuaObjType::LUAOBJ_UNIT>();
	};
protected:
	void Update() const override;
};

class CUnitDrawerLegacy : public CUnitDrawerBase {
public:
	void Draw(bool drawReflection, bool drawRefraction = false) const override {
		DrawImpl<true, LuaObjType::LUAOBJ_UNIT>(drawReflection, drawRefraction);
	}
	void DrawShadowPass() const override {
		DrawShadowPassImpl<true, LuaObjType::LUAOBJ_UNIT>();
	}

	void DrawUnitModel(const CUnit* unit, bool noLuaCall) const override;
	void DrawUnitNoTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const override;
	void DrawUnitTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const override;
	void DrawIndividual(const CUnit* unit, bool noLuaCall) const override;
	void DrawIndividualNoTrans(const CUnit* unit, bool noLuaCall) const override;

	void DrawIndividualDefOpaque(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const override;
	void DrawIndividualDefAlpha(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const override;

	bool ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command>& commands) const override;

	void DrawUnitMiniMapIcons() const override;
	void DrawUnitIcons() const override;
	void DrawUnitIconsScreen() const override;
protected:
	void DrawObjectsShadow(int modelType) const override;
	void DrawOpaqueObjects(int modelType, bool drawReflection, bool drawRefraction) const;
	void DrawOpaqueObjectsAux(int modelType) const override; //AI units

	void DrawAlphaObjects(int modelType) const override;
	void DrawAlphaObjectsAux(int modelType) const override;

	void DrawGhostedBuildings(int modelType) const override;

	void DrawOpaqueUnit(CUnit* unit, bool drawReflection, bool drawRefraction) const;
	void DrawUnitShadow(CUnit* unit) const;
	void DrawAlphaUnit(CUnit* unit, int modelType, bool drawGhostBuildingsPass) const;

	void DrawOpaqueAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const;
	void DrawAlphaAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const;
	void DrawAlphaAIUnitBorder(const CUnitDrawerData::TempDrawUnit& unit) const;

	void DrawUnitModelBeingBuiltShadow(const CUnit* unit, bool noLuaCall) const override;
	void DrawModelWireBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall, bool amdHack) const;
	void DrawModelFlatBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const;
	void DrawModelFillBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const;

	void DrawUnitModelBeingBuiltOpaque(const CUnit* unit, bool noLuaCall) const override;
	void DrawModelWireBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall, bool amdHack) const;
	void DrawModelFlatBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const;
	void DrawModelFillBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall, bool amdHack) const;

	void PushIndividualOpaqueState(const CUnit* unit, bool deferredPass) const;
	void PushIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass)  const;
	void PushIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass) const;

	void PopIndividualOpaqueState(const CUnit* unit, bool deferredPass) const;
	void PopIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass) const;
	void PopIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass) const;

	void DrawUnitMiniMapIcon(const CUnit* unit, CVertexArray* va) const;

	static void DrawIcon(CUnit* unit, bool useDefaultIcon);
	void DrawIconScreenArray(const CUnit* unit, const icon::CIconData* icon, bool useDefaultIcon, const float dist, CVertexArray* va) const;
protected:

};

class CUnitDrawerFFP  final : public CUnitDrawerLegacy {};
class CUnitDrawerARB  final : public CUnitDrawerLegacy {};
class CUnitDrawerGLSL final : public CUnitDrawerLegacy {};

//TODO remove CUnitDrawerLegacy inheritance
class CUnitDrawerGL4 final : public CUnitDrawerLegacy {
public:
	void Draw(bool drawReflection, bool drawRefraction = false) const override {
		DrawImpl<false, LuaObjType::LUAOBJ_UNIT>(drawReflection, drawRefraction);
	}
	void DrawShadowPass() const override {
		DrawShadowPassImpl<false, LuaObjType::LUAOBJ_UNIT>();
	}

	// DrawUnit*
	/* TODO figure out
	void DrawUnitModel(const CUnit* unit, bool noLuaCall) const = 0;
	void DrawUnitModelBeingBuiltShadow(const CUnit* unit, bool noLuaCall) const = 0;
	void DrawUnitModelBeingBuiltOpaque(const CUnit* unit, bool noLuaCall) const = 0;
	void DrawUnitNoTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const = 0;
	void DrawUnitTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const = 0;
	void DrawIndividual(const CUnit* unit, bool noLuaCall) const = 0;
	void DrawIndividualNoTrans(const CUnit* unit, bool noLuaCall) const = 0;
	*/

	// DrawIndividualDef*
	/*
	void DrawIndividualDefOpaque(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const = 0;
	void DrawIndividualDefAlpha(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const = 0;
	*/
protected:
	void DrawObjectsShadow(int modelType) const override;
	void DrawOpaqueObjects(int modelType, bool drawReflection, bool drawRefraction) const override;
	void DrawAlphaObjects(int modelType) const override;

	void DrawAlphaObjectsAux(int modelType) const override;
	void DrawAlphaAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const;
	void DrawAlphaAIUnitBorder(const CUnitDrawerData::TempDrawUnit& unit) const {} //not implemented

	void DrawOpaqueObjectsAux(int modelType) const override;
	void DrawOpaqueAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const;

	void DrawGhostedBuildings(int modelType) const override {} //implemented in line
};

#define unitDrawer (CUnitDrawer::modelDrawer)