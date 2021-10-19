/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <vector>
#include <array>

#include "Game/Camera.h"
#include "Rendering/Models/ModelRenderContainer.h"
#include "Rendering/Common/ModelDrawer.h"
#include "Rendering/Features/FeatureDrawerData.h"

class CFeature;

class CFeatureDrawer: public CModelDrawerBase<CFeatureDrawerData, CFeatureDrawer>
{
public:
	static void InitStatic();
	//static void KillStatic(bool reload); will use base
	//static void UpdateStatic();
public:
	// DrawFeature*
	virtual void DrawFeatureNoTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const = 0;
	virtual void DrawFeatureTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const = 0;

	/// LuaOpenGL::Feature{Raw}: draw a single feature with full state setup
	virtual void DrawIndividual(const CFeature* feature, bool noLuaCall) const = 0;
	virtual void DrawIndividualNoTrans(const CFeature* feature, bool noLuaCall) const = 0;
protected:
	virtual void DrawOpaqueFeature(CFeature* f, bool drawReflection, bool drawRefraction) const = 0;
	virtual void DrawAlphaFeature(CFeature* f) const = 0;
public:
	// modelDrawerData proxies
	void ConfigNotify(const std::string& key, const std::string& value) { modelDrawerData->ConfigNotify(key, value); }
	static const std::vector<CFeature*>& GetUnsortedFeatures() { return modelDrawerData->GetUnsortedObjects(); }
public:
	virtual void DrawFeatureModel(const CFeature* feature, bool noLuaCall) const = 0;
protected:
	static bool ShouldDrawOpaqueFeature(CFeature* f, bool drawReflection, bool drawRefraction);
	static bool ShouldDrawAlphaFeature(CFeature* f);
	static bool ShouldDrawFeatureShadow(CFeature* f);

	void PushIndividualState(const CFeature* feature, bool deferredPass) const;
	void PopIndividualState(const CFeature* feature, bool deferredPass) const;
};

class CFeatureDrawerBase : public CFeatureDrawer
{
public:
	void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const override {
		DrawOpaquePassImpl<LuaObjType::LUAOBJ_FEATURE>(deferredPass, drawReflection, drawRefraction);
	}
	void DrawAlphaPass() const override {
		DrawAlphaPassImpl<LuaObjType::LUAOBJ_FEATURE>();
	};
protected:
	void DrawOpaqueObjectsAux(int modelType) const override {} //no aux objects here
	void DrawAlphaObjectsAux(int modelType) const override {} //no aux objects here
	void Update() const override;
};

class CFeatureDrawerLegacy : public CFeatureDrawerBase
{
public:
	void Draw(bool drawReflection, bool drawRefraction) const override {
		DrawImpl<true, LuaObjType::LUAOBJ_FEATURE>(drawReflection, drawRefraction);
	}
	void DrawShadowPass() const override {
		DrawShadowPassImpl<true, LuaObjType::LUAOBJ_FEATURE>();
	}

	void DrawFeatureNoTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const override;
	void DrawFeatureTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const override;

	/// LuaOpenGL::Feature{Raw}: draw a single feature with full state setup
	void DrawIndividual(const CFeature* feature, bool noLuaCall) const override;
	void DrawIndividualNoTrans(const CFeature* feature, bool noLuaCall) const override;
protected:
	void DrawObjectsShadow(int modelType) const override;
	void DrawOpaqueObjects(int modelType, bool drawReflection, bool drawRefraction) const override;
	void DrawAlphaObjects(int modelType) const override;

	void DrawOpaqueFeature(CFeature* f, bool drawReflection, bool drawRefraction) const;
	void DrawAlphaFeature(CFeature* f) const override;
	void DrawFeatureShadow(CFeature* f) const;

	void DrawFeatureModel(const CFeature* feature, bool noLuaCall) const override;
};

class CFeatureDrawerFFP  final : public CFeatureDrawerLegacy {};
class CFeatureDrawerARB  final : public CFeatureDrawerLegacy {};
class CFeatureDrawerGLSL final : public CFeatureDrawerLegacy {};

//TODO remove CFeatureDrawerLegacy inheritance
class CFeatureDrawerGL4 final: public CFeatureDrawerLegacy//CFeatureDrawerBase
{
public:
	void Draw(bool drawReflection, bool drawRefraction) const override {
		DrawImpl<false, LuaObjType::LUAOBJ_FEATURE>(drawReflection, drawRefraction);
	}
	void DrawShadowPass() const override {
		DrawShadowPassImpl<false, LuaObjType::LUAOBJ_FEATURE>();
	}
protected:
	void DrawObjectsShadow(int modelType) const override;

	void DrawOpaqueObjects(int modelType, bool drawReflection, bool drawRefraction) const override;
	void DrawAlphaObjects(int modelType) const override;
};

#define featureDrawer (CFeatureDrawer::modelDrawer)