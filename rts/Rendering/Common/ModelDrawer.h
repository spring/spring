#pragma once

#include <array>
#include <string>
#include <string_view>
#include <stack>
#include <tuple>

#include "ModelDrawerData.h"
#include "ModelDrawerState.hpp"
#include "ModelDrawerHelpers.h"
#include "System/Log/ILog.h"
#include "System/TypeToStr.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/FarTextureHandler.h"
#include "Rendering/GL/LightHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"

namespace GL { struct GeometryBuffer; }
template<typename T> class ScopedModelDrawerImpl;
class ScopedMatricesMemAlloc;

enum ModelDrawerTypes {
	MODEL_DRAWER_FFP =  0, // fixed-function path
	MODEL_DRAWER_ARB =  1, // standard-shader path (ARB)
	MODEL_DRAWER_GLSL = 2, // standard-shader path (GLSL)
	MODEL_DRAWER_GL4 =  3, // GL4-shader path (GLSL)
	MODEL_DRAWER_CNT =  4
};

static constexpr std::string_view ModelDrawerNames[ModelDrawerTypes::MODEL_DRAWER_CNT] = {
	"FFP : fixed-function path",
	"ARB : legacy standard shader path",
	"GLSL: legacy standard shader path",
	"GL4 : modern standard shader path",
};

class CModelDrawerConcept {
public:
	CModelDrawerConcept() {}
	virtual ~CModelDrawerConcept() {}
public:
	static void InitStatic();
	static void KillStatic(bool reload);
public:
	static bool  UseAdvShading() { return advShading; }
	static bool  DeferredAllowed() { return deferredAllowed; }
	static bool& WireFrameModeRef() { return wireFrameMode; }
public:
	// lightHandler
	static GL::LightHandler* GetLightHandler() { return &lightHandler; }

	// geomBuffer
	static GL::GeometryBuffer* GetGeometryBuffer() { return geomBuffer; }
protected:
	inline static bool initialized = false;

	inline static bool advShading = false;
	inline static bool wireFrameMode = false;

	inline static bool deferredAllowed = false;
protected:
	inline static GL::LightHandler lightHandler;
	inline static GL::GeometryBuffer* geomBuffer = nullptr;
};

template <typename TDrawerData, typename TDrawer>
class CModelDrawerBase : public CModelDrawerConcept {
public:
	using ObjType = typename TDrawerData::ObjType;
public:
	template<typename TDrawerDerivative>
	static void InitInstance(int t) {
		static_assert(std::is_base_of_v<TDrawer, TDrawerDerivative>, "");
		static_assert(std::is_base_of_v<CModelDrawerBase<TDrawerData, TDrawer>, TDrawer>, "");

		if (modelDrawers[t] == nullptr) {
			modelDrawers[t] = new TDrawerDerivative{};
			modelDrawers[t]->mdType = static_cast<ModelDrawerTypes>(t);
		}
	}
	static void KillInstance(int t) {
		spring::SafeDelete(modelDrawers[t]);
	}

	static void InitStatic();
	static void KillStatic(bool reload);
	static void UpdateStatic() {
		SelectImplementation(reselectionRequested);
		reselectionRequested = false;
		modelDrawer->Update();
	}
public:
	// Set/Get state from outside
	static bool& UseAdvShadingRef() { reselectionRequested = true; return advShading; }
	static bool& WireFrameModeRef() { return wireFrameMode; }

	static int  PreferedDrawerType() { return preferedDrawerType; }
	static int& PreferedDrawerTypeRef() { reselectionRequested = true; return preferedDrawerType; }

	static bool& MTDrawerTypeRef() { return mtModelDrawer; } //no reselectionRequested needed

	static void SetDrawForwardPass(bool b) { drawForward = b; }
	static void SetDrawDeferredPass(bool b) { drawDeferred = b; }
	static bool DrawForward() { return drawForward; }
	static bool DrawDeferred() { return drawDeferred; }

	static void ForceLegacyPath();

	static void SelectImplementation(bool forceReselection = false, bool legacy = true, bool modern = true);
	static void SelectImplementation(int targetImplementation);

	template<typename T> friend class ScopedModelDrawerImpl;

	/// Proxy interface for modelDrawerState
	static bool CanDrawDeferred() { return modelDrawerState->CanDrawDeferred(); }
	static bool SetTeamColor(int team, const float alpha = 1.0f) { return modelDrawerState->SetTeamColor(team, alpha); }
	static void SetNanoColor(const float4& color) { modelDrawerState->SetNanoColor(color); }
	static const ScopedMatricesMemAlloc& GetMatricesMemAlloc(const ObjType* o) { return const_cast<const TDrawerData*>(modelDrawerData)->GetObjectMatricesMemAlloc(o); }
public:
	virtual void Update() const = 0;
	// Draw*
	virtual void Draw(bool drawReflection, bool drawRefraction = false) const = 0;

	virtual void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const = 0;
	virtual void DrawShadowPass() const = 0;
	virtual void DrawAlphaPass() const = 0;
protected:
	virtual void DrawOpaqueObjects(int modelType, bool drawReflection, bool drawRefraction) const = 0;
	virtual void DrawOpaqueObjectsAux(int modelType) const = 0;

	virtual void DrawAlphaObjects(int modelType) const = 0;
	virtual void DrawAlphaObjectsAux(int modelType) const = 0;

	virtual void DrawObjectsShadow(int modelType) const = 0;
public:
	// Setup Fixed State, reused by a few other classes, thus public:
	void SetupOpaqueDrawing(bool deferredPass) const { modelDrawerState->SetupOpaqueDrawing(deferredPass); }
	void ResetOpaqueDrawing(bool deferredPass) const { modelDrawerState->ResetOpaqueDrawing(deferredPass); }

	void SetupAlphaDrawing(bool deferredPass) const { modelDrawerState->SetupAlphaDrawing(deferredPass); }
	void ResetAlphaDrawing(bool deferredPass) const { modelDrawerState->ResetAlphaDrawing(deferredPass); }
protected:
	template<bool legacy, LuaObjType lot>
	void DrawImpl(bool drawReflection, bool drawRefraction) const;

	template<LuaObjType lot>
	void DrawOpaquePassImpl(bool deferredPass, bool drawReflection, bool drawRefraction) const;

	template<LuaObjType lot>
	void DrawAlphaPassImpl() const;

	template<bool legacy, LuaObjType lot>
	void DrawShadowPassImpl() const;
protected:
	// TODO move into TDrawerData?
	static bool CheckLegacyDrawing(const CSolidObject* so, bool noLuaCall);
	static bool CheckLegacyDrawing(const CSolidObject* so, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall);
private:
	static void Push(bool legacy, bool modern, bool mt) {
		implStack.emplace(std::make_tuple(modelDrawer, modelDrawerState, mtModelDrawer));
		mtModelDrawer = mt;
		SelectImplementation(true, legacy, modern);
	}
	static void Pop() {
		std::tie(modelDrawer, modelDrawerState, mtModelDrawer) = implStack.top();
		implStack.pop();
	}
private:
	ModelDrawerTypes mdType = ModelDrawerTypes::MODEL_DRAWER_CNT;
public:
	inline static TDrawer* modelDrawer = nullptr;
protected:
	inline static int preferedDrawerType = ModelDrawerTypes::MODEL_DRAWER_CNT; //no preference
	inline static bool mtModelDrawer = true;

	inline static bool reselectionRequested = true;

	inline static bool forceLegacyPath = false;

	inline static bool drawForward = true;
	inline static bool drawDeferred = true;

	inline static TDrawerData* modelDrawerData = nullptr;
	inline static IModelDrawerState* modelDrawerState = nullptr;
	inline static std::array<TDrawer*, ModelDrawerTypes::MODEL_DRAWER_CNT> modelDrawers = {};

	inline static std::stack<std::tuple<TDrawer*, IModelDrawerState*, bool>> implStack;
protected:
	static constexpr std::string_view className = spring::TypeToStr<TDrawer>();
};

template<typename T>
class ScopedModelDrawerImpl {
public:
	ScopedModelDrawerImpl(bool legacy, bool modern, bool mt = false) {
		T::Push(legacy, modern, mt);
	}
	~ScopedModelDrawerImpl() {
		T::Pop();
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

template<typename TDrawerData, typename TDrawer>
inline void CModelDrawerBase<TDrawerData, TDrawer>::InitStatic()
{
	CModelDrawerConcept::InitStatic();
	//CModelDrawerBase<TDrawerData, TDrawer>::InitInstance is done in TDrawer::InitStatic()

	mtModelDrawer = true;
	reselectionRequested = true;
	forceLegacyPath = false;
	drawForward = true;
	drawDeferred = true;

	while (!implStack.empty()) { implStack.pop(); } //clear just in case

	modelDrawerData = new TDrawerData{ mtModelDrawer };
}

template<typename TDrawerData, typename TDrawer>
inline void CModelDrawerBase<TDrawerData, TDrawer>::KillStatic(bool reload)
{
	for (int t = ModelDrawerTypes::MODEL_DRAWER_FFP; t < ModelDrawerTypes::MODEL_DRAWER_CNT; ++t) {
		CModelDrawerBase<TDrawerData, TDrawer>::KillInstance(t);
	}

	CModelDrawerConcept::KillStatic(reload);

	spring::SafeDelete(modelDrawerData);

	modelDrawer = nullptr;
	modelDrawerState = nullptr;
}

template<typename TDrawerData, typename TDrawer>
inline void CModelDrawerBase<TDrawerData, TDrawer>::ForceLegacyPath()
{
	reselectionRequested = true;
	forceLegacyPath = true;
	LOG_L(L_WARNING, "[%s::%s] Using legacy (slow) %s renderer! This is caused by insufficient GPU/driver capabilities or by using of old Lua rendering API", className.data(), __func__, className.data());
}

template<typename TDrawerData, typename TDrawer>
inline void CModelDrawerBase<TDrawerData, TDrawer>::SelectImplementation(bool forceReselection, bool legacy, bool modern)
{
	if (!forceReselection)
		return;

	if (!advShading) {
		SelectImplementation(ModelDrawerTypes::MODEL_DRAWER_FFP);
		return;
	}

	const auto qualifyDrawerFunc = [legacy, modern](const TDrawer* d, const IModelDrawerState* s) -> bool {
		if (d == nullptr || s == nullptr)
			return false;

		if (CModelDrawerBase<TDrawerData, TDrawer>::forceLegacyPath && !s->IsLegacy())
			return false;

		if (s->IsLegacy() && !legacy)
			return false;

		if (!s->IsLegacy() && !modern)
			return false;

		if (!s->CanEnable())
			return false;

		if (!s->IsValid())
			return false;

		return true;
	};

	if (preferedDrawerType < ModelDrawerTypes::MODEL_DRAWER_CNT) {
		auto d = modelDrawers[preferedDrawerType];
		auto s = IModelDrawerState::modelDrawerStates[preferedDrawerType];
		if (qualifyDrawerFunc(d, s)) {
			LOG_L(L_INFO, "[%s::%s] Force-switching to %s(%s)", className.data(), __func__, ModelDrawerNames[preferedDrawerType].data(), mtModelDrawer ? "MT" : "ST");
			SelectImplementation(preferedDrawerType);
		}
		else {
			LOG_L(L_ERROR, "[%s::%s] Couldn't force-switch to %s(%s)", className.data(), __func__, ModelDrawerNames[preferedDrawerType].data(), mtModelDrawer ? "MT" : "ST");
		}
		preferedDrawerType = ModelDrawerTypes::MODEL_DRAWER_CNT; //reset;
		return;
	}

	int best = ModelDrawerTypes::MODEL_DRAWER_FFP;
	for (int t = ModelDrawerTypes::MODEL_DRAWER_ARB; t < ModelDrawerTypes::MODEL_DRAWER_CNT; ++t) {
		auto d = modelDrawers[t];
		auto s = IModelDrawerState::modelDrawerStates[t];
		if (qualifyDrawerFunc(d, s)) {
			best = t;
		}
	}

	SelectImplementation(best);
}

template<typename TDrawerData, typename TDrawer>
inline void CModelDrawerBase<TDrawerData, TDrawer>::SelectImplementation(int targetImplementation)
{
	modelDrawer = modelDrawers[targetImplementation];
	assert(modelDrawer);

	modelDrawerState = IModelDrawerState::modelDrawerStates[targetImplementation];
	assert(modelDrawerState);
	assert(modelDrawerState->CanEnable());
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename TDrawerData, typename TDrawer>
template<bool legacy, LuaObjType lot>
inline void CModelDrawerBase<TDrawerData, TDrawer>::DrawImpl(bool drawReflection, bool drawRefraction) const
{
	static const std::string methodName = std::string(className) + "::Draw";
	SCOPED_TIMER(methodName.c_str());

	if constexpr (legacy) {
		glEnable(GL_ALPHA_TEST);
		sky->SetupFog();
	}

	assert((CCameraHandler::GetActiveCamera())->GetCamType() != CCamera::CAMTYPE_SHADOW);

	// first do the deferred pass; conditional because
	// most of the water renderers use their own FBO's
	if (drawDeferred && !drawReflection && !drawRefraction)
		LuaObjectDrawer::DrawDeferredPass(lot);

	// now do the regular forward pass
	if (drawForward)
		DrawOpaquePass(false, drawReflection, drawRefraction);

	farTextureHandler->Draw();

	if constexpr (legacy) {
		glDisable(GL_FOG);
		glDisable(GL_TEXTURE_2D);
	}
}

template<typename TDrawerData, typename TDrawer>
template<LuaObjType lot>
inline void CModelDrawerBase<TDrawerData, TDrawer>::DrawOpaquePassImpl(bool deferredPass, bool drawReflection, bool drawRefraction) const
{
	static const std::string methodName = std::string(className) + "::DrawOpaquePass";
	SCOPED_TIMER(methodName.c_str());

	SetupOpaqueDrawing(deferredPass);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; ++modelType) {
		if (modelDrawerData->GetModelRenderer(modelType).empty())
			continue;

		CModelDrawerHelper::PushModelRenderState(modelType);
		DrawOpaqueObjects(modelType, drawReflection, drawRefraction);
		DrawOpaqueObjectsAux(modelType);
		CModelDrawerHelper::PopModelRenderState(modelType);
	}

	ResetOpaqueDrawing(deferredPass);

	ScopedModelDrawerImpl<CModelDrawerBase<TDrawerData, TDrawer>> smdi(true, false, false);
	// draw all custom'ed units that were bypassed in the loop above
	LuaObjectDrawer::SetDrawPassGlobalLODFactor(lot);
	LuaObjectDrawer::DrawOpaqueMaterialObjects(lot, deferredPass);
}

template<typename TDrawerData, typename TDrawer>
template<LuaObjType lot>
inline void CModelDrawerBase<TDrawerData, TDrawer>::DrawAlphaPassImpl() const
{
	static const std::string methodName = std::string(className) + "::DrawAlphaPass";
	SCOPED_TIMER(methodName.c_str());

	SetupAlphaDrawing(false);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; ++modelType) {
		if (modelDrawerData->GetModelRenderer(modelType).empty())
			continue;

		CModelDrawerHelper::PushModelRenderState(modelType);
		DrawAlphaObjects(modelType);
		DrawAlphaObjectsAux(modelType);
		CModelDrawerHelper::PopModelRenderState(modelType);
	}

	ResetAlphaDrawing(false);

	ScopedModelDrawerImpl<CModelDrawerBase<TDrawerData, TDrawer>> smdi(true, false, false);
	// draw all custom'ed units that were bypassed in the loop above
	LuaObjectDrawer::SetDrawPassGlobalLODFactor(lot);
	LuaObjectDrawer::DrawAlphaMaterialObjects(lot, /*deferredPass*/false);
}

template<typename TDrawerData, typename TDrawer>
template<bool legacy, LuaObjType lot>
inline void CModelDrawerBase<TDrawerData, TDrawer>::DrawShadowPassImpl() const
{
	static const std::string methodName = std::string(className) + "::DrawShadowPass";
	SCOPED_TIMER(methodName.c_str());

	assert((CCameraHandler::GetActiveCamera())->GetCamType() == CCamera::CAMTYPE_SHADOW);

	if constexpr (legacy) {
		glColor3f(1.0f, 1.0f, 1.0f);
		glPolygonOffset(1.0f, 1.0f);
		glEnable(GL_POLYGON_OFFSET_FILL);

		glAlphaFunc(GL_GREATER, 0.5f);
		glEnable(GL_ALPHA_TEST);
	}

	CShadowHandler::ShadowGenProgram shadowGenProgram;
	if constexpr (legacy)
		shadowGenProgram = CShadowHandler::SHADOWGEN_PROGRAM_MODEL;
	else
		shadowGenProgram = CShadowHandler::SHADOWGEN_PROGRAM_MODEL_GL4;

	Shader::IProgramObject* po = shadowHandler.GetShadowGenProg(shadowGenProgram);
	assert(po);
	assert(po->IsValid());
	po->Enable();

	// 3DO's have clockwise-wound faces and
	// (usually) holes, so disable backface
	// culling for them
	// glDisable(GL_CULL_FACE); Draw(); glEnable(GL_CULL_FACE);
	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; ++modelType) {
		if (modelDrawerData->GetModelRenderer(modelType).empty())
			continue;

		if (modelType == MODELTYPE_3DO) glDisable(GL_CULL_FACE);
		DrawObjectsShadow(modelType);
		if (modelType == MODELTYPE_3DO) glEnable(GL_CULL_FACE);
	}

	po->Disable();

	if constexpr (legacy) {
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	ScopedModelDrawerImpl<CModelDrawerBase<TDrawerData, TDrawer>> smdi(true, false, false);
	// draw all custom'ed units that were bypassed in the loop above
	LuaObjectDrawer::SetDrawPassGlobalLODFactor(lot);
	LuaObjectDrawer::DrawShadowMaterialObjects(lot, /*deferredPass*/false);
}

template<typename TDrawerData, typename TDrawer>
inline bool CModelDrawerBase<TDrawerData, TDrawer>::CheckLegacyDrawing(const CSolidObject* so, bool noLuaCall)
{
	if (so->luaDraw || !noLuaCall)
		return false;

	return true;
}

template<typename TDrawerData, typename TDrawer>
inline bool CModelDrawerBase<TDrawerData, TDrawer>::CheckLegacyDrawing(const CSolidObject* so, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall)
{
	//#pragma message("TODO: Make use of it")
	if (forceLegacyPath)
		return false;

	if (lodCall || preList != 0 || postList != 0 || CheckLegacyDrawing(so, noLuaCall)) { //TODO: sanitize
		ForceLegacyPath();
		return false;
	}

	return true;
}
