/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaObjectDrawer.h"
#include "FeatureDrawer.h"
#include "UnitDrawer.h"
#include "UnitDrawerState.hpp"
#include "Game/Camera.h"
#include "Game/Game.h" // drawMode
#include "Lua/LuaMaterial.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/ShadowHandler.h"
#include "Sim/Misc/GlobalConstants.h" // MAX_TEAMS
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/SafeUtil.h"


// optimisation for team-color, but potentially breaks
// the alpha-pass and matrices can not be bucket-sorted
// #define USE_OBJECT_RENDERING_BUCKETS

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
// (if only 'decltype(auto) unitDrawFuncs[2] = {&CUnitDrawer::DrawUnitLuaTrans, ...}' would
// work, or even 'std::array<auto, 2> unitDrawFuncs = {&CUnitDrawer::DrawUnitLuaTrans, ...}'
// while this can actually be done with C++11 forward magic, the code becomes unreadable)
//
typedef void(CEventHandler::*EventFunc)();
typedef void(*   UnitDrawFunc)(const CUnit*,    bool, bool, bool);
typedef void(*FeatureDrawFunc)(const CFeature*, bool, bool);

#endif



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
static void SetupDefOpaqueUnitDrawState(unsigned int modelType, bool deferredPass) {
	unitDrawer->SetupOpaqueDrawing(deferredPass);
	unitDrawer->PushModelRenderState(modelType);
	unitDrawer->SetAlphaTest({0.5f, 1.0f, 0.0f, 0.0}); // test > 0.5 (engine shaders)
}

static void ResetDefOpaqueUnitDrawState(unsigned int modelType, bool deferredPass) {
	unitDrawer->SetAlphaTest({0.0f, 0.0f, 0.0f, 1.0}); // no test
	unitDrawer->PopModelRenderState(modelType);
	unitDrawer->ResetOpaqueDrawing(deferredPass);
}

// NOTE: incomplete (FeatureDrawer::Draw sets more state)
static void SetupDefOpaqueFeatureDrawState(unsigned int modelType, bool deferredPass) { SetupDefOpaqueUnitDrawState(modelType, deferredPass); }
static void ResetDefOpaqueFeatureDrawState(unsigned int modelType, bool deferredPass) { ResetDefOpaqueUnitDrawState(modelType, deferredPass); }


static void SetupLuaOpaqueUnitDrawState(unsigned int modelType, bool deferredPass) { unitDrawer->SetDrawerState(DRAWER_STATE_LUA); }
static void ResetLuaOpaqueUnitDrawState(unsigned int modelType, bool deferredPass) { unitDrawer->SetDrawerState(DRAWER_STATE_SSP); }

static void SetupLuaOpaqueFeatureDrawState(unsigned int modelType, bool deferredPass) { unitDrawer->SetDrawerState(DRAWER_STATE_LUA); }
static void ResetLuaOpaqueFeatureDrawState(unsigned int modelType, bool deferredPass) { unitDrawer->SetDrawerState(DRAWER_STATE_SSP); }



// transparency-pass (reflection, ...) state management funcs
static void SetupDefAlphaUnitDrawState(unsigned int modelType, bool deferredPass) {
	unitDrawer->SetupAlphaDrawing(deferredPass, true);
	unitDrawer->PushModelRenderState(modelType);
	unitDrawer->SetAlphaTest({0.1f, 1.0f, 0.0f, 0.0}); // test > 0.1 (engine shaders)
}

static void ResetDefAlphaUnitDrawState(unsigned int modelType, bool deferredPass) {
	unitDrawer->SetAlphaTest({0.0f, 0.0f, 0.0f, 1.0}); // no test
	unitDrawer->PopModelRenderState(modelType);
	unitDrawer->ResetAlphaDrawing(deferredPass);
}

// NOTE: incomplete (FeatureDrawer::DrawAlphaPass sets more state)
static void SetupDefAlphaFeatureDrawState(unsigned int modelType, bool deferredPass) { SetupDefAlphaUnitDrawState(modelType, deferredPass); }
static void ResetDefAlphaFeatureDrawState(unsigned int modelType, bool deferredPass) { ResetDefAlphaUnitDrawState(modelType, deferredPass); }


static void SetupLuaAlphaUnitDrawState(unsigned int modelType, bool deferredPass) { unitDrawer->SetDrawerState(DRAWER_STATE_LUA); }
static void ResetLuaAlphaUnitDrawState(unsigned int modelType, bool deferredPass) { unitDrawer->SetDrawerState(DRAWER_STATE_SSP); }

static void SetupLuaAlphaFeatureDrawState(unsigned int modelType, bool deferredPass) { unitDrawer->SetDrawerState(DRAWER_STATE_LUA); }
static void ResetLuaAlphaFeatureDrawState(unsigned int modelType, bool deferredPass) { unitDrawer->SetDrawerState(DRAWER_STATE_SSP); }




// shadow-pass state management funcs
// FIXME: setup face culling for S3O?
static void SetupDefShadowUnitDrawState(unsigned int modelType, bool deferredPass) {
	glAttribStatePtr->PolygonOffset(1.0f, 1.0f);
	glAttribStatePtr->PolygonOffsetFill(GL_TRUE);

	Shader::IProgramObject* po = shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);
	po->Enable();
	po->SetUniformMatrix4fv(1, false, shadowHandler.GetShadowViewMatrix());
	po->SetUniformMatrix4fv(2, false, shadowHandler.GetShadowProjMatrix());
}

static void ResetDefShadowUnitDrawState(unsigned int modelType, bool deferredPass) {
	Shader::IProgramObject* po = shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);

	po->Disable();
	glAttribStatePtr->PolygonOffsetFill(GL_FALSE);
}

// NOTE: incomplete (FeatureDrawer::DrawShadowPass sets more state)
static void SetupDefShadowFeatureDrawState(unsigned int modelType, bool deferredPass) { SetupDefShadowUnitDrawState(modelType, deferredPass); }
static void ResetDefShadowFeatureDrawState(unsigned int modelType, bool deferredPass) { ResetDefShadowUnitDrawState(modelType, deferredPass); }


static void SetupLuaShadowUnitDrawState(unsigned int modelType, bool deferredPass) { unitDrawer->SetDrawerState(DRAWER_STATE_LUA); }
static void ResetLuaShadowUnitDrawState(unsigned int modelType, bool deferredPass) { unitDrawer->SetDrawerState(DRAWER_STATE_SSP); }

static void SetupLuaShadowFeatureDrawState(unsigned int modelType, bool deferredPass) { unitDrawer->SetDrawerState(DRAWER_STATE_LUA); }
static void ResetLuaShadowFeatureDrawState(unsigned int modelType, bool deferredPass) { unitDrawer->SetDrawerState(DRAWER_STATE_SSP); }




static const void SetObjectMatricesNop(const CSolidObject* o, const LuaMaterial* m, bool deferredPass) {}
static const void SetObjectMatricesLua(const CSolidObject* o, const LuaMaterial* m, bool deferredPass) {
	LocalModel* lm = const_cast<LocalModel*>(&o->localModel);

	assert(m->shaders[deferredPass].IsCustomType());
	lm->UpdatePieceMatrices(gs->frameNum);
	m->ExecuteInstanceMatrices(o->GetTransformMatrix(), lm->GetPieceMatrices(), deferredPass);
}
// no-op; *Drawer::Draw*Trans sets the uniforms via default SSP state
static const void SetObjectMatricesDef(const CSolidObject* o, const LuaMaterial* m, bool deferredPass) {}


static const void SetObjectTeamColorNop(const CSolidObject*, const LuaMaterial*, const float2, bool) {}
static const void SetObjectTeamColorLua(const CSolidObject* o, const LuaMaterial* m, const float2 a, bool deferredPass)
{
	assert(m->shaders[deferredPass].IsCustomType());
	m->ExecuteInstanceTeamColor(IUnitDrawerState::GetTeamColor(o->team, a.x), deferredPass);
}

static const void SetObjectTeamColorDef(const CSolidObject* o, const LuaMaterial* m, const float2 a, bool deferredPass)
{
	// only useful to set this if the object has a standard
	// (engine) shader attached, otherwise requires testing
	// if shader is bound in DrawerState etc
	assert(m->shaders[deferredPass].IsEngineType());
	unitDrawer->SetTeamColour(o->team, a);
}


static const void SetObjectUniformsNop(const CSolidObject* o, const LuaMaterial* m, int objectType, bool deferredPass) {} // no-op
static const void SetObjectUniformsLua(const CSolidObject* o, const LuaMaterial* m, int objectType, bool deferredPass) { m->ExecuteInstanceUniforms(o->id, objectType, deferredPass); }
static const void SetObjectUniformsDef(const CSolidObject* o, const LuaMaterial* m, int objectType, bool deferredPass) {} // no-op



static const decltype(&SetObjectTeamColorDef) tcUniformFuncs[] = {
	SetObjectTeamColorNop,
	SetObjectTeamColorLua,
	SetObjectTeamColorDef,
};

static const decltype(&SetObjectMatricesDef) tmUniformFuncs[] = {
	SetObjectMatricesNop,
	SetObjectMatricesLua,
	SetObjectMatricesDef,
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

static inline unsigned int CalcTransMatrUniformFuncIndex(const CSolidObject* o, const LuaMatShader* s) {
	const unsigned int isCustomType = s->IsCustomType() << 0;
	const unsigned int isEngineType = s->IsEngineType() << 1;
	return (isCustomType | isEngineType);
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

	unitDrawFuncs[false] = &CUnitDrawer::DrawUnitLuaTrans;
	unitDrawFuncs[ true] = &CUnitDrawer::DrawUnitDefTrans;

	featureDrawFuncs[false] = &CFeatureDrawer::DrawFeatureLuaTrans;
	featureDrawFuncs[ true] = &CFeatureDrawer::DrawFeatureDefTrans;

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

	glAttribStatePtr->PushBits(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_TRANSFORM_BIT);

	if (inAlphaBin) {
		glAttribStatePtr->EnableBlendMask();
		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	const LuaMaterial* prevMat = &LuaMaterial::defMat;

	for (const auto& bin: bins) {
		assert(matType == bin->type);
		DrawMaterialBin(bin, prevMat, objType, matType, deferredPass, inAlphaBin);
		prevMat = bin;
	}

	LuaMaterial::defMat.Execute(*prevMat, deferredPass);
	luaMatHandler.ClearBins(objType, matType);

	glAttribStatePtr->PopBits();

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

	// skip entire bin if we have no shader for this pass
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

	// FIXME: alpha-bin might be sorted
	for (const CSolidObject* obj: objects) {
		objectBuckets[obj->team].push_back(obj);

		minObjTeam = std::min(minObjTeam, obj->team);
		maxObjTeam = std::max(maxObjTeam, obj->team);
	}

	for (int objTeam = minObjTeam; objTeam <= maxObjTeam; objTeam++) {
		for (const CSolidObject* obj: objectBuckets[objTeam]) {
			const LuaObjectMaterialData* matData = obj->GetLuaMaterialData();
			const LuaMatRef* lodMatRef = matData->GetLODMatRef(matType);

			DrawBinObject(obj, objType, lodMatRef, currBin,  deferredPass, alphaMatBin, true, false);
		}

		objectBuckets[objTeam].clear();
		objectBuckets[objTeam].reserve(128);
	}

	#else

	for (const CSolidObject* obj: objects) {
		const LuaObjectMaterialData* matData = obj->GetLuaMaterialData();
		const LuaMatRef* lodMatRef = matData->GetLODMatRef(matType);

		DrawBinObject(obj, objType, lodMatRef, currBin,  deferredPass, alphaMatBin, true, false);
	}
	#endif
}

void LuaObjectDrawer::DrawBinObject(
	const CSolidObject* obj,
	LuaObjType objType,
	const LuaMatRef* matRef,
	const LuaMaterial* luaMat,
	bool deferredPass,
	bool alphaMatBin,
	bool applyTrans,
	bool noLuaCall
) {
	const unsigned int tcFuncIdx = CalcTeamColorUniformFuncIndex(obj, &luaMat->shaders[deferredPass]);
	const unsigned int tmFuncIdx = CalcTransMatrUniformFuncIndex(obj, &luaMat->shaders[deferredPass]);
	const unsigned int soFuncIdx = CalcSetObjectUniformFuncIndex(obj, &luaMat->shaders[deferredPass]);

	switch (objType) {
		case LUAOBJ_UNIT: {
			const auto udFunc = unitDrawFuncs[applyTrans];
			const auto tcFunc = tcUniformFuncs[tcFuncIdx];
			const auto tmFunc = tmUniformFuncs[tmFuncIdx];
			const auto soFunc = soUniformFuncs[soFuncIdx];

			const CUnit* u = static_cast<const CUnit*>(obj);

			tcFunc(u, luaMat, {1.0f, 1.0f * alphaMatBin}, deferredPass);
			tmFunc(u, luaMat, deferredPass);
			soFunc(u, luaMat, objType, deferredPass);
			udFunc(u, true, noLuaCall, true);
		} break;
		case LUAOBJ_FEATURE: {
			const auto fdFunc = featureDrawFuncs[applyTrans];
			const auto tcFunc = tcUniformFuncs[tcFuncIdx];
			const auto tmFunc = tmUniformFuncs[tmFuncIdx];
			const auto soFunc = soUniformFuncs[soFuncIdx];

			const CFeature* f = static_cast<const CFeature*>(obj);

			tcFunc(f, luaMat, {f->drawAlpha, 1.0f * alphaMatBin}, deferredPass);
			tmFunc(f, luaMat, deferredPass);
			soFunc(f, luaMat, objType, deferredPass);
			fdFunc(f, true, noLuaCall);
		} break;
		default: {
			assert(false);
		} break;
	}

	binObjTeam = obj->team;
}


void LuaObjectDrawer::DrawDeferredPass(LuaObjType objType)
{
	if (!drawDeferredEnabled)
		return;
	if (!geomBuffer->Valid())
		return;

	// deferred pass must be executable with base shaders
	assert((unitDrawer->GetWantedDrawerState(false))->CanDrawDeferred());

	// note: should also set this during the map pass (in SMFGD)
	game->SetDrawMode(Game::DeferredDraw);
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
			unitDrawer->DrawOpaquePass(true);
		} break;
		case LUAOBJ_FEATURE: {
			featureDrawer->DrawOpaquePass(true);
		} break;
		default: {
			assert(false);
		} break;
	}

	geomBuffer->SetDepthRange(0.0f, 1.0f);
	geomBuffer->UnBind();
	game->SetDrawMode(Game::NormalDraw);

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
	const LuaMatRef* lodMatRef = nullptr;

	if (!matData->Enabled())
		return false;

	// note: always uses an opaque material (for now)
	if ((lodMatRef = matData->GetLODMatRef(LuaObjectDrawer::GetDrawPassOpaqueMat())) == nullptr)
		return false;
	if (!lodMatRef->IsActive())
		return false;

	switch (objType) {
		case LUAOBJ_UNIT: {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupDefOpaqueUnitDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetDefOpaqueUnitDrawState;
			}

			luaMatHandler.setupDrawStateFuncs[MODELTYPE_OTHER] = SetupLuaOpaqueUnitDrawState;
			luaMatHandler.resetDrawStateFuncs[MODELTYPE_OTHER] = ResetLuaOpaqueUnitDrawState;
		} break;
		case LUAOBJ_FEATURE: {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupDefOpaqueFeatureDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetDefOpaqueFeatureDrawState;
			}

			luaMatHandler.setupDrawStateFuncs[MODELTYPE_OTHER] = SetupLuaOpaqueFeatureDrawState;
			luaMatHandler.resetDrawStateFuncs[MODELTYPE_OTHER] = ResetLuaOpaqueFeatureDrawState;
		} break;
		default: {
			assert(false);
		} break;
	}

	// use the normal scale for single-object drawing
	LuaObjectMaterialData::SetGlobalLODFactor(objType, LuaObjectDrawer::GetLODScale(objType) * camera->GetLPPScale());

	// get the ref-counted actual material
	const LuaMatBin* currBin = lodMatRef->GetBin();
	const LuaMaterial* currMat = currBin;

	// reset
	binObjTeam = -1;

	// NOTE: doesn't make sense to support deferred mode for this? (extra arg in gl.Unit, etc)
	currMat->Execute(LuaMaterial::defMat, false);

	DrawBinObject(obj, objType, lodMatRef, currMat, false, false, applyTrans, true);

	// switch back to default material
	LuaMaterial::defMat.Execute(*currMat, false);
	return true;
}


bool LuaObjectDrawer::DrawSingleObject(const CSolidObject* obj, LuaObjType objType)
{
	return (DrawSingleObjectCommon(obj, objType, true));
}

bool LuaObjectDrawer::DrawSingleObjectLuaTrans(const CSolidObject* obj, LuaObjType objType)
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
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupDefOpaqueUnitDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetDefOpaqueUnitDrawState;
			}

			luaMatHandler.setupDrawStateFuncs[MODELTYPE_OTHER] = SetupLuaOpaqueUnitDrawState;
			luaMatHandler.resetDrawStateFuncs[MODELTYPE_OTHER] = ResetLuaOpaqueUnitDrawState;
		} break;
		case LUAOBJ_FEATURE: {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupDefOpaqueFeatureDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetDefOpaqueFeatureDrawState;
			}

			luaMatHandler.setupDrawStateFuncs[MODELTYPE_OTHER] = SetupLuaOpaqueFeatureDrawState;
			luaMatHandler.resetDrawStateFuncs[MODELTYPE_OTHER] = ResetLuaOpaqueFeatureDrawState;
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
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupDefAlphaUnitDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetDefAlphaUnitDrawState;
			}

			luaMatHandler.setupDrawStateFuncs[MODELTYPE_OTHER] = SetupLuaAlphaUnitDrawState;
			luaMatHandler.resetDrawStateFuncs[MODELTYPE_OTHER] = ResetLuaAlphaUnitDrawState;
		} break;
		case LUAOBJ_FEATURE: {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupDefAlphaFeatureDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetDefAlphaFeatureDrawState;
			}

			luaMatHandler.setupDrawStateFuncs[MODELTYPE_OTHER] = SetupLuaAlphaFeatureDrawState;
			luaMatHandler.resetDrawStateFuncs[MODELTYPE_OTHER] = ResetLuaAlphaFeatureDrawState;
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
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupDefShadowUnitDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetDefShadowUnitDrawState;
			}

			luaMatHandler.setupDrawStateFuncs[MODELTYPE_OTHER] = SetupLuaShadowUnitDrawState;
			luaMatHandler.resetDrawStateFuncs[MODELTYPE_OTHER] = ResetLuaShadowUnitDrawState;
		} break;
		case LUAOBJ_FEATURE: {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
				luaMatHandler.setupDrawStateFuncs[modelType] = SetupDefShadowFeatureDrawState;
				luaMatHandler.resetDrawStateFuncs[modelType] = ResetDefShadowFeatureDrawState;
			}

			luaMatHandler.setupDrawStateFuncs[MODELTYPE_OTHER] = SetupLuaShadowFeatureDrawState;
			luaMatHandler.resetDrawStateFuncs[MODELTYPE_OTHER] = ResetLuaShadowFeatureDrawState;
		} break;
		default: {
			assert(false);
		} break;
	}

	// we neither want nor need a deferred shadow
	// pass for custom- or default-shader models!
	DrawMaterialBins(objType, GetDrawPassShadowMat(), false);
}

