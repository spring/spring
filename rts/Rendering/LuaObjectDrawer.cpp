/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaObjectDrawer.h"
#include "Features/FeatureDrawer.h"
#include "Common/ModelDrawerHelpers.h"
#include "Units/UnitDrawer.h"
#include "Common/ModelDrawerState.hpp"
#include "Game/Camera.h"
#include "Game/Game.h" // drawMode
#include "Lua/LuaMaterial.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Common/ModelDrawerHelpers.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/ShadowHandler.h"
#include "Sim/Misc/GlobalConstants.h" // MAX_TEAMS
#include "Sim/Objects/SolidObject.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/SafeUtil.h"


// optimisation for team-color, but potentially breaks
// the alpha-pass and matrices can not be bucket-sorted
#define USE_OBJECT_RENDERING_BUCKETS

// applies to both units and features
CONFIG(bool, AllowDeferredModelRendering).defaultValue(false).safemodeValue(false);
CONFIG(bool, AllowDeferredModelBufferClear).defaultValue(false).safemodeValue(false);
CONFIG(bool, AllowDrawModelPostDeferredEvents).defaultValue(true);
CONFIG(bool, AllowMultiSampledFrameBuffers).defaultValue(false);

CONFIG(float, LODScale).defaultValue(1.0f);
CONFIG(float, LODScaleShadow).defaultValue(1.0f);
CONFIG(float, LODScaleReflection).defaultValue(1.0f);
CONFIG(float, LODScaleRefraction).defaultValue(1.0f);



GL::GeometryBuffer* LuaObjectDrawer::geomBuffer = nullptr;

bool LuaObjectDrawer::inDrawPass = false;
bool LuaObjectDrawer::inAlphaBin = false;

bool LuaObjectDrawer::drawDeferredEnabled = false;
bool LuaObjectDrawer::drawDeferredAllowed = false;
bool LuaObjectDrawer::bufferClearAllowed = false;

int LuaObjectDrawer::binObjTeam = -1;

float LuaObjectDrawer::LODScale[LUAOBJ_LAST];
float LuaObjectDrawer::LODScaleShadow[LUAOBJ_LAST];
float LuaObjectDrawer::LODScaleReflection[LUAOBJ_LAST];
float LuaObjectDrawer::LODScaleRefraction[LUAOBJ_LAST];



#ifdef USE_STD_FUNCTION
#define CALL_FUNC_NA(OBJ, FUN     ) (FUN)((OBJ)             )
#define CALL_FUNC_VA(OBJ, FUN, ...) (FUN)((OBJ), __VA_ARGS__)

typedef std::function<void(CEventHandler*)> EventFunc;
typedef std::function<void(CUnitDrawer*,    const CUnit*,    unsigned int, unsigned int, bool, bool)>    UnitDrawFunc;
typedef std::function<void(CFeatureDrawer*, const CFeature*, unsigned int, unsigned int, bool, bool)> FeatureDrawFunc;

#else
#define CALL_FUNC_NA(OBJ, FUN     ) ((OBJ)->*(FUN))(           )
#define CALL_FUNC_VA(OBJ, FUN, ...) ((OBJ)->*(FUN))(__VA_ARGS__)

// much nastier syntax especially for member functions
// (another reason to use statics), but also much more
// cache-friendly than std::function's ASM bloat
//
// (if only 'decltype(auto) unitDrawFuncs[2] = {&CUnitDrawer::DrawUnitNoTrans, ...}' would
// work, or even 'std::array<auto, 2> unitDrawFuncs = {&CUnitDrawer::DrawUnitNoTrans, ...}'
// while this can actually be done with C++11 forward magic, the code becomes unreadable)
//
typedef void(CEventHandler::*EventFunc)();
typedef void(CUnitDrawer   ::*   UnitDrawFunc)(const CUnit*,    unsigned int, unsigned int, bool, bool) const;
typedef void(CFeatureDrawer::*FeatureDrawFunc)(const CFeature*, unsigned int, unsigned int, bool, bool) const;

#endif

typedef const void (*TeamColorFunc)(const bool, const CSolidObject*, const LuaMaterial*, const float2);



#ifdef USE_STD_ARRAY
#define DECL_ARRAY(TYPE, NAME, SIZE) std::array<TYPE, SIZE> NAME
#else
#define DECL_ARRAY(TYPE, NAME, SIZE) TYPE NAME[SIZE]
#endif

// these should remain valid on reload
static DECL_ARRAY(EventFunc, eventFuncs, LUAOBJ_LAST) = {nullptr, nullptr};

// transform and no-transform variants
static DECL_ARRAY(   UnitDrawFunc,    unitDrawFuncs, 2) = {nullptr, nullptr};
static DECL_ARRAY(FeatureDrawFunc, featureDrawFuncs, 2) = {nullptr, nullptr};

static DECL_ARRAY(bool, notifyEventFlags, LUAOBJ_LAST) = {false, false};
static DECL_ARRAY(bool, bufferClearFlags, LUAOBJ_LAST) = { true,  true};

static const DECL_ARRAY(LuaMatType, opaqueMats, 2) = {LUAMAT_OPAQUE, LUAMAT_OPAQUE_REFLECT};
static const DECL_ARRAY(LuaMatType,  alphaMats, 2) = {LUAMAT_ALPHA, LUAMAT_ALPHA_REFLECT};

static const DECL_ARRAY(LuaMatShader::Pass, shaderPasses, 2) = {LuaMatShader::LUASHADER_PASS_FWD, LuaMatShader::LUASHADER_PASS_DFR};

#ifdef USE_OBJECT_RENDERING_BUCKETS
static DECL_ARRAY(std::vector<const CSolidObject*>, objectBuckets, MAX_TEAMS);
#endif



static float GetLODFloat(const std::string& name)
{
	// NOTE: the inverse of the value is used
	const float value = std::max(0.0f, configHandler->GetFloat(name));
	const float recip = spring::SafeDivide(1.0f, value);
	return recip;
}



// opaque-pass state management funcs
static void SetupOpaqueUnitDrawState(unsigned int modelType, bool deferredPass) {
	unitDrawer->SetupOpaqueDrawing(deferredPass);
	CModelDrawerHelper::PushModelRenderState(modelType);
}

static void ResetOpaqueUnitDrawState(unsigned int modelType, bool deferredPass) {
	CModelDrawerHelper::PopModelRenderState(modelType);
	unitDrawer->ResetOpaqueDrawing(deferredPass);
}

// NOTE: incomplete (FeatureDrawer::Draw sets more state)
static void SetupOpaqueFeatureDrawState(unsigned int modelType, bool deferredPass) { SetupOpaqueUnitDrawState(modelType, deferredPass); }
static void ResetOpaqueFeatureDrawState(unsigned int modelType, bool deferredPass) { ResetOpaqueUnitDrawState(modelType, deferredPass); }



// transparency-pass (reflection, ...) state management funcs
static void SetupAlphaUnitDrawState(unsigned int modelType, bool deferredPass) {
	unitDrawer->SetupAlphaDrawing(deferredPass);
	CModelDrawerHelper::PushModelRenderState(modelType);
}

static void ResetAlphaUnitDrawState(unsigned int modelType, bool deferredPass) {
	CModelDrawerHelper::PopModelRenderState(modelType);
	unitDrawer->ResetAlphaDrawing(deferredPass);
}

// NOTE: incomplete (FeatureDrawer::DrawAlphaPass sets more state)
static void SetupAlphaFeatureDrawState(unsigned int modelType, bool deferredPass) { SetupAlphaUnitDrawState(modelType, deferredPass); }
static void ResetAlphaFeatureDrawState(unsigned int modelType, bool deferredPass) { ResetAlphaUnitDrawState(modelType, deferredPass); }



// shadow-pass state management funcs
// FIXME: setup face culling for S3O?
static void SetupShadowUnitDrawState(unsigned int modelType, bool deferredPass) {
	glColor3f(1.0f, 1.0f, 1.0f);
	glDisable(GL_TEXTURE_2D);

	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);

	Shader::IProgramObject* po = shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);
	po->Enable();
}

static void ResetShadowUnitDrawState(unsigned int modelType, bool deferredPass) {
	Shader::IProgramObject* po = shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);

	po->Disable();
	glDisable(GL_POLYGON_OFFSET_FILL);
}

// NOTE: incomplete (FeatureDrawer::DrawShadowPass sets more state)
static void SetupShadowFeatureDrawState(unsigned int modelType, bool deferredPass) { SetupShadowUnitDrawState(modelType, deferredPass); }
static void ResetShadowFeatureDrawState(unsigned int modelType, bool deferredPass) { ResetShadowUnitDrawState(modelType, deferredPass); }


static const void SetObjectTeamColorNop(const CSolidObject*, const LuaMaterial*, float, bool) {}
static const void SetObjectTeamColorLua(const CSolidObject* o, const LuaMaterial* m, float a, bool deferredPass)
{
	assert(m->shaders[deferredPass].IsCustomType());
	m->ExecuteInstanceTeamColor(CModelDrawerHelper::GetTeamColor(o->team, a), deferredPass);
}

static const void SetObjectTeamColorDef(const CSolidObject* o, const LuaMaterial* m, float a, bool deferredPass)
{
	// only useful to set this if the object has a standard
	// (engine) shader attached, otherwise requires testing
	// if shader is bound in DrawerState etc
	assert(m->shaders[deferredPass].IsEngineType());

	CUnitDrawer::SetTeamColor(o->team, a);
}


static const void SetObjectUniformsNop(const CSolidObject* o, const LuaMaterial* m, int objectType, bool deferredPass) {} // no-op
static const void SetObjectUniformsLua(const CSolidObject* o, const LuaMaterial* m, int objectType, bool deferredPass) { m->ExecuteInstanceUniforms(o->id, objectType, deferredPass); }
static const void SetObjectUniformsDef(const CSolidObject* o, const LuaMaterial* m, int objectType, bool deferredPass) {} // no-op



static const decltype(&SetObjectTeamColorDef) tcUniformFuncs[] = {
	SetObjectTeamColorNop,
	SetObjectTeamColorLua,
	SetObjectTeamColorDef,
};

static const decltype(&SetObjectUniformsDef) soUniformFuncs[] = {
	SetObjectUniformsNop,
	SetObjectUniformsLua,
	SetObjectUniformsDef,
};


static inline unsigned int CalcTeamColorUniformFuncIndex(const CSolidObject* o, const LuaMatShader* s) {
	const unsigned int isCustomType = s->IsCustomType() << 0;
	const unsigned int isEngineType = s->IsEngineType() << 1;
	// if still in the same team{-bucket}, pick the no-op func
	return ((isCustomType | isEngineType) * (o->team != LuaObjectDrawer::GetBinObjTeam()));
}

static inline unsigned int CalcSetObjectUniformFuncIndex(const CSolidObject* o, const LuaMatShader* s) {
	const unsigned int isCustomType = s->IsCustomType() << 0;
	const unsigned int isEngineType = s->IsEngineType() << 1;
	return (isCustomType | isEngineType);
}



void LuaObjectDrawer::Init()
{
	eventFuncs[LUAOBJ_UNIT   ] = &CEventHandler::DrawUnitsPostDeferred;
	eventFuncs[LUAOBJ_FEATURE] = &CEventHandler::DrawFeaturesPostDeferred;

	unitDrawFuncs[false] = &CUnitDrawer::DrawUnitNoTrans;
	unitDrawFuncs[ true] = &CUnitDrawer::DrawUnitTrans;

	featureDrawFuncs[false] = &CFeatureDrawer::DrawFeatureNoTrans;
	featureDrawFuncs[ true] = &CFeatureDrawer::DrawFeatureTrans;

	drawDeferredAllowed = configHandler->GetBool("AllowDeferredModelRendering");
	bufferClearAllowed = configHandler->GetBool("AllowDeferredModelBufferClear");

	assert(geomBuffer == nullptr);

	// cannot be a unique_ptr because it is leaked
	geomBuffer = new GL::GeometryBuffer("LUAOBJECTDRAWER-GBUFFER");
}

void LuaObjectDrawer::Kill()
{
	eventFuncs[LUAOBJ_UNIT   ] = nullptr;
	eventFuncs[LUAOBJ_FEATURE] = nullptr;

	unitDrawFuncs[false] = nullptr;
	unitDrawFuncs[ true] = nullptr;

	featureDrawFuncs[false] = nullptr;
	featureDrawFuncs[ true] = nullptr;

	assert(geomBuffer != nullptr);
	spring::SafeDelete(geomBuffer);
}


void LuaObjectDrawer::Update(bool init)
{
	assert(geomBuffer != nullptr);

	if (!drawDeferredAllowed)
		return;

	// update buffer only if it is valid
	if ((drawDeferredEnabled = geomBuffer->Valid())) {
		drawDeferredEnabled &= (geomBuffer->Update(init));

		notifyEventFlags[LUAOBJ_UNIT   ] = !unitDrawer->DrawForward() || configHandler->GetBool("AllowDrawModelPostDeferredEvents");
		bufferClearFlags[LUAOBJ_UNIT   ] =  unitDrawer->DrawDeferred();
		notifyEventFlags[LUAOBJ_FEATURE] = !featureDrawer->DrawForward() || configHandler->GetBool("AllowDrawModelPostDeferredEvents");
		bufferClearFlags[LUAOBJ_FEATURE] =  featureDrawer->DrawDeferred();

		// if both object types are going to be drawn deferred, only
		// reset buffer for the first s.t. just a single shading pass
		// is needed (in Lua)
		if (bufferClearFlags[LUAOBJ_UNIT] && bufferClearFlags[LUAOBJ_FEATURE])
			bufferClearFlags[LUAOBJ_FEATURE] = bufferClearAllowed;
	}
}


void LuaObjectDrawer::ReadLODScales(LuaObjType objType)
{
	LODScale          [objType] = GetLODFloat("LODScale");
	LODScaleShadow    [objType] = GetLODFloat("LODScaleShadow");
	LODScaleReflection[objType] = GetLODFloat("LODScaleReflection");
	LODScaleRefraction[objType] = GetLODFloat("LODScaleRefraction");
}

void LuaObjectDrawer::SetDrawPassGlobalLODFactor(LuaObjType objType)
{
	if (shadowHandler.InShadowPass()) {
		LuaObjectMaterialData::SetGlobalLODFactor(objType, GetLODScaleShadow(objType) * camera->GetLPPScale());
		return;
	}

	if (water->DrawReflectionPass()) {
		LuaObjectMaterialData::SetGlobalLODFactor(objType, GetLODScaleReflection(objType) * camera->GetLPPScale());
		return;
	}

	if (water->DrawRefractionPass()) {
		LuaObjectMaterialData::SetGlobalLODFactor(objType, GetLODScaleRefraction(objType) * camera->GetLPPScale());
		return;
	}

	LuaObjectMaterialData::SetGlobalLODFactor(objType, GetLODScale(objType) * camera->GetLPPScale());
}


LuaMatType LuaObjectDrawer::GetDrawPassOpaqueMat() { return opaqueMats[water->DrawReflectionPass()]; }
LuaMatType LuaObjectDrawer::GetDrawPassAlphaMat() { return alphaMats[water->DrawReflectionPass()]; }


void LuaObjectDrawer::DrawMaterialBins(LuaObjType objType, LuaMatType matType, bool deferredPass)
{
	const LuaMatBinSet& bins = luaMatHandler.GetBins(matType);

	if (bins.empty())
		return;

	inDrawPass = true;
	inAlphaBin = (matType == LUAMAT_ALPHA || matType == LUAMAT_ALPHA_REFLECT);

	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_TRANSFORM_BIT);

	if (inAlphaBin) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.1f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);
	}

	const LuaMaterial* prevMat = &LuaMaterial::defMat;

	for (const auto& bin: bins) {
		assert(matType == bin->type);
		DrawMaterialBin(bin, prevMat, objType, matType, deferredPass, inAlphaBin);
		prevMat = bin;
	}

	LuaMaterial::defMat.Execute(*prevMat, deferredPass);
	luaMatHandler.ClearBins(objType, matType);

	glPopAttrib();

	inAlphaBin = false;
	inDrawPass = false;
}

void LuaObjectDrawer::DrawMaterialBin(
	const LuaMatBin* currBin,
	const LuaMaterial* prevMat,
	LuaObjType objType,
	LuaMatType matType,
	bool deferredPass,
	bool alphaMatBin
) {
	currBin->Execute(*prevMat, deferredPass);

	const std::vector<CSolidObject*>& objects = currBin->GetObjects(objType);
	const LuaMatShader* binShader = &currBin->shaders[deferredPass];

	// skip entire bin if we need a shader for this pass and have none
	if (!binShader->ValidForPass(shaderPasses[deferredPass]))
		return;

	// objType and matType can be inferred from the material's (custom) uuid
	// deferredPass and alphaMatBin can be inferred from drawMode and matType
	if (currBin->HasDrawCall() && eventHandler.DrawMaterial(currBin))
		return;

	// reset; also sort objects by team
	binObjTeam = -1;

	#ifdef USE_OBJECT_RENDERING_BUCKETS
	int minObjTeam = MAX_TEAMS;
	int maxObjTeam = 0;

	for (const CSolidObject* obj: objects) {
		objectBuckets[obj->team].push_back(obj);

		minObjTeam = std::min(minObjTeam, obj->team);
		maxObjTeam = std::max(maxObjTeam, obj->team);
	}

	for (int objTeam = minObjTeam; objTeam <= maxObjTeam; objTeam++) {
		for (const CSolidObject* obj: objectBuckets[objTeam]) {
			const LuaObjectMaterialData* matData = obj->GetLuaMaterialData();
			const LuaObjectLODMaterial* lodMat = matData->GetLuaLODMaterial(matType);

			DrawBinObject(obj, objType, lodMat, currBin,  deferredPass, alphaMatBin, true, false);
		}

		objectBuckets[objTeam].clear();
		objectBuckets[objTeam].reserve(128);
	}

	#else

	for (const CSolidObject* obj: objects) {
		const LuaObjectMaterialData* matData = obj->GetLuaMaterialData();
		const LuaObjectLODMaterial* lodMat = matData->GetLuaLODMaterial(matType);

		DrawBinObject(obj, objType, lodMat, currBin,  deferredPass, alphaMatBin, true, false);
	}
	#endif
}

void LuaObjectDrawer::DrawBinObject(
	const CSolidObject* obj,
	LuaObjType objType,
	const LuaObjectLODMaterial* lodMat,
	const LuaMaterial* luaMat,
	bool deferredPass,
	bool alphaMatBin,
	bool applyTrans,
	bool noLuaCall
) {
	const unsigned int preList  = lodMat->preDisplayList;
	const unsigned int postList = lodMat->postDisplayList;

	const unsigned int tcFuncIdx = CalcTeamColorUniformFuncIndex(obj, &luaMat->shaders[deferredPass]);
	const unsigned int soFuncIdx = CalcSetObjectUniformFuncIndex(obj, &luaMat->shaders[deferredPass]);

	switch (objType) {
		case LUAOBJ_UNIT: {
			const auto udFunc = unitDrawFuncs[applyTrans];
			const auto tcFunc = tcUniformFuncs[tcFuncIdx];
			const auto soFunc = soUniformFuncs[soFuncIdx];

			const CUnit* unit = static_cast<const CUnit*>(obj);

			tcFunc(unit, luaMat, 1.0f, deferredPass);
			soFunc(unit, luaMat, objType, deferredPass);
			CALL_FUNC_VA(unitDrawer, udFunc, unit, preList, postList, true, noLuaCall);
		} break;
		case LUAOBJ_FEATURE: {
			const auto fdFunc = featureDrawFuncs[applyTrans];
			const auto tcFunc = tcUniformFuncs[tcFuncIdx];
			const auto soFunc = soUniformFuncs[soFuncIdx];

			const CFeature* feat = static_cast<const CFeature*>(obj);

			tcFunc(feat, luaMat, feat->drawAlpha, deferredPass);
			soFunc(feat, luaMat, objType, deferredPass);
			CALL_FUNC_VA(featureDrawer, fdFunc, feat, preList, postList, true, noLuaCall);
		} break;
		default: {
			assert(false);
		} break;
	}

	//FIXME
	binObjTeam = obj->team; //FIXME alpha????
}


void LuaObjectDrawer::DrawDeferredPass(LuaObjType objType)
{
	if (!drawDeferredEnabled)
		return;
	if (!geomBuffer->Valid())
		return;

	// deferred pass must be executed with GLSL base shaders so
	// bail early if the FFP state *is going to be* selected by
	// SetupOpaqueDrawing, and also if our shader-path happens
	// to be ARB instead (saves an FBO bind)
	if (!(CUnitDrawer::CanDrawDeferred()))
		return;

	// note: should also set this during the map pass (in SMFGD)
	game->SetDrawMode(CGame::gameDeferredDraw);
	geomBuffer->Bind();
	geomBuffer->SetDepthRange(1.0f, 0.0f);

	// reset the buffer (since we do not perform a single pass
	// writing both units and features into it at the same time)
	// this however forces Lua to execute two shading passes and
	// listen to the *PostDeferred events, so not ideal
	// geomBuffer->Clear();

	if (bufferClearFlags[objType])
		geomBuffer->Clear();

	switch (objType) {
		case LUAOBJ_UNIT: {
			unitDrawer->DrawOpaquePass(true, false, false);
		} break;
		case LUAOBJ_FEATURE: {
			featureDrawer->DrawOpaquePass(true, false, false);
		} break;
		default: {
			assert(false);
		} break;
	}

	geomBuffer->SetDepthRange(0.0f, 1.0f);
	geomBuffer->UnBind();
	game->SetDrawMode(CGame::gameNormalDraw);

	#if 0
	geomBuffer->DrawDebug(geomBuffer->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_NORMTEX));
	#endif

	if (notifyEventFlags[objType]) {
		// at this point the buffer has been filled (all standard
		// models and custom Lua material bins have been rendered
		// into it) and unbound, notify scripts
		assert(eventFuncs[objType] != nullptr);
		CALL_FUNC_NA(&eventHandler, eventFuncs[objType]);
	}
}




bool LuaObjectDrawer::DrawSingleObjectCommon(const CSolidObject* obj, LuaObjType objType, bool applyTrans)
{
	const LuaObjectMaterialData* matData = obj->GetLuaMaterialData();
	const LuaObjectLODMaterial* lodMat = nullptr;

	if (!matData->Enabled())
		return false;

	// note: always uses an opaque material (for now)
	if ((lodMat = matData->GetLuaLODMaterial(LuaObjectDrawer::GetDrawPassOpaqueMat())) == nullptr)
		return false;
	if (!lodMat->IsActive())
		return false;

	switch (objType) {
		case LUAOBJ_UNIT: {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupOpaqueUnitDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetOpaqueUnitDrawState;
			}
		} break;
		case LUAOBJ_FEATURE: {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupOpaqueFeatureDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetOpaqueFeatureDrawState;
			}
		} break;
		default: {
			assert(false);
		} break;
	}

	// use the normal scale for single-object drawing
	LuaObjectMaterialData::SetGlobalLODFactor(objType, LuaObjectDrawer::GetLODScale(objType) * camera->GetLPPScale());

	// get the ref-counted actual material
	const LuaMatBin* currBin = lodMat->matref.GetBin();
	const LuaMaterial* currMat = currBin;

	// reset
	binObjTeam = -1;

	// NOTE: doesn't make sense to support deferred mode for this? (extra arg in gl.Unit, etc)
	currMat->Execute(LuaMaterial::defMat, false);

	DrawBinObject(obj, objType, lodMat, currMat, false, false, applyTrans, true);

	// switch back to default material
	LuaMaterial::defMat.Execute(*currMat, false);
	return true;
}


bool LuaObjectDrawer::DrawSingleObject(const CSolidObject* obj, LuaObjType objType)
{
	return (DrawSingleObjectCommon(obj, objType, true));
}

bool LuaObjectDrawer::DrawSingleObjectNoTrans(const CSolidObject* obj, LuaObjType objType)
{
	return (DrawSingleObjectCommon(obj, objType, false));
}




void LuaObjectDrawer::SetObjectLOD(CSolidObject* obj, LuaObjType objType, unsigned int lodCount)
{
	if (!obj->localModel.Initialized())
		return;

	obj->localModel.SetLODCount(lodCount);
}

bool LuaObjectDrawer::AddObjectForLOD(CSolidObject* obj, LuaObjType objType, bool useAlphaMat, bool useShadowMat)
{
	if (useShadowMat)
		return (AddShadowMaterialObject(obj, objType));
	if (useAlphaMat)
		return (AddAlphaMaterialObject(obj, objType));

	return (AddOpaqueMaterialObject(obj, objType));
}



bool LuaObjectDrawer::AddOpaqueMaterialObject(CSolidObject* obj, LuaObjType objType)
{
	LuaObjectMaterialData* matData = obj->GetLuaMaterialData();

	const LuaMatType matType = GetDrawPassOpaqueMat();
	const float      lodDist = camera->ProjectedDistance(obj->pos);

	return (matData->AddObjectForLOD(obj, objType, matType, lodDist));
}

bool LuaObjectDrawer::AddAlphaMaterialObject(CSolidObject* obj, LuaObjType objType)
{
	LuaObjectMaterialData* matData = obj->GetLuaMaterialData();

	const LuaMatType matType = GetDrawPassAlphaMat();
	const float      lodDist = camera->ProjectedDistance(obj->pos);

	return (matData->AddObjectForLOD(obj, objType, matType, lodDist));
}

bool LuaObjectDrawer::AddShadowMaterialObject(CSolidObject* obj, LuaObjType objType)
{
	LuaObjectMaterialData* matData = obj->GetLuaMaterialData();

	const LuaMatType matType = GetDrawPassShadowMat();
	const float      lodDist = camera->ProjectedDistance(obj->pos);

	return (matData->AddObjectForLOD(obj, objType, matType, lodDist));
}



void LuaObjectDrawer::DrawOpaqueMaterialObjects(LuaObjType objType, bool deferredPass)
{
	switch (objType) {
		case LUAOBJ_UNIT: {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupOpaqueUnitDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetOpaqueUnitDrawState;
			}
		} break;
		case LUAOBJ_FEATURE: {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupOpaqueFeatureDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetOpaqueFeatureDrawState;
			}
		} break;
		default: {
			assert(false);
		} break;
	}

	DrawMaterialBins(objType, GetDrawPassOpaqueMat(), deferredPass);
}

void LuaObjectDrawer::DrawAlphaMaterialObjects(LuaObjType objType, bool)
{
	switch (objType) {
		case LUAOBJ_UNIT: {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupAlphaUnitDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetAlphaUnitDrawState;
			}
		} break;
		case LUAOBJ_FEATURE: {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupAlphaFeatureDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetAlphaFeatureDrawState;
			}
		} break;
		default: {
			assert(false);
		} break;
	}

	// FIXME: deferred shading and transparency is a PITA
	DrawMaterialBins(objType, GetDrawPassAlphaMat(), false);
}

void LuaObjectDrawer::DrawShadowMaterialObjects(LuaObjType objType, bool)
{
	switch (objType) {
		case LUAOBJ_UNIT: {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupShadowUnitDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetShadowUnitDrawState;
			}
		} break;
		case LUAOBJ_FEATURE: {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupShadowFeatureDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetShadowFeatureDrawState;
			}
		} break;
		default: {
			assert(false);
		} break;
	}

	// we neither want nor need a deferred shadow
	// pass for custom- or default-shader models!
	DrawMaterialBins(objType, GetDrawPassShadowMat(), false);
}

