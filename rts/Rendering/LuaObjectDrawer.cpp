/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaObjectDrawer.h"
#include "FeatureDrawer.h"
#include "UnitDrawer.h"
#include "Game/Camera.h"
#include "Lua/LuaMaterial.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/ShadowHandler.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Util.h"

// applies to both units and features
CONFIG(bool, AllowDeferredModelRendering).defaultValue(false).safemodeValue(false);
CONFIG(bool, AllowDeferredModelBufferClear).defaultValue(false).safemodeValue(false);

CONFIG(float, LODScale).defaultValue(1.0f);
CONFIG(float, LODScaleShadow).defaultValue(1.0f);
CONFIG(float, LODScaleReflection).defaultValue(1.0f);
CONFIG(float, LODScaleRefraction).defaultValue(1.0f);


bool LuaObjectDrawer::inDrawPass = false;
bool LuaObjectDrawer::drawDeferredEnabled = false;
bool LuaObjectDrawer::drawDeferredAllowed = false;
bool LuaObjectDrawer::bufferClearAllowed = false;

float LuaObjectDrawer::LODScale[LUAOBJ_LAST];
float LuaObjectDrawer::LODScaleShadow[LUAOBJ_LAST];
float LuaObjectDrawer::LODScaleReflection[LUAOBJ_LAST];
float LuaObjectDrawer::LODScaleRefraction[LUAOBJ_LAST];


// these should remain valid on reload
static std::function<void(CEventHandler*)> eventFuncs[LUAOBJ_LAST] = {nullptr, nullptr};
static GL::GeometryBuffer* geomBuffer = nullptr;

static bool notifyEventFlags[LUAOBJ_LAST] = {false, false};
static bool bufferClearFlags[LUAOBJ_LAST] = { true,  true};

static const LuaMatType opaqueMats[2] = {LUAMAT_OPAQUE, LUAMAT_OPAQUE_REFLECT};
static const LuaMatType  alphaMats[2] = {LUAMAT_ALPHA, LUAMAT_ALPHA_REFLECT};


static float GetLODFloat(const std::string& name)
{
	// NOTE: the inverse of the value is used
	const float value = std::max(0.0f, configHandler->GetFloat(name));
	const float recip = SafeDivide(1.0f, value);
	return recip;
}



// opaque-pass state management funcs
static void SetupOpaqueUnitDrawState(unsigned int modelType, bool deferredPass) {
	unitDrawer->SetupForUnitDrawing(deferredPass);
	unitDrawer->GetOpaqueModelRenderer(modelType)->PushRenderState();
}

static void ResetOpaqueUnitDrawState(unsigned int modelType, bool deferredPass) {
	unitDrawer->GetOpaqueModelRenderer(modelType)->PopRenderState();
	unitDrawer->CleanUpUnitDrawing(deferredPass);
}

// NOTE: incomplete (FeatureDrawer::Draw sets more state)
static void SetupOpaqueFeatureDrawState(unsigned int modelType, bool deferredPass) { SetupOpaqueUnitDrawState(modelType, deferredPass); }
static void ResetOpaqueFeatureDrawState(unsigned int modelType, bool deferredPass) { ResetOpaqueUnitDrawState(modelType, deferredPass); }



// transparency-pass (reflection, ...) state management funcs
static void SetupAlphaUnitDrawState(unsigned int modelType, bool deferredPass) {
	unitDrawer->SetupForGhostDrawing();
	unitDrawer->GetCloakedModelRenderer(modelType)->PushRenderState();
}

static void ResetAlphaUnitDrawState(unsigned int modelType, bool deferredPass) {
	unitDrawer->GetCloakedModelRenderer(modelType)->PopRenderState();
	unitDrawer->CleanUpGhostDrawing();
}

// NOTE: incomplete (FeatureDrawer::DrawFadeFeatures sets more state)
static void SetupAlphaFeatureDrawState(unsigned int modelType, bool deferredPass) { SetupAlphaUnitDrawState(modelType, deferredPass); }
static void ResetAlphaFeatureDrawState(unsigned int modelType, bool deferredPass) { ResetAlphaUnitDrawState(modelType, deferredPass); }



// shadow-pass state management funcs
// FIXME: setup face culling for S3O?
static void SetupShadowUnitDrawState(unsigned int modelType, bool deferredPass) {
	glColor3f(1.0f, 1.0f, 1.0f);
	glDisable(GL_TEXTURE_2D);

	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);

	Shader::IProgramObject* po =
		shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);
	po->Enable();
}

static void ResetShadowUnitDrawState(unsigned int modelType, bool deferredPass) {
	Shader::IProgramObject* po =
		shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);

	po->Disable();
	glDisable(GL_POLYGON_OFFSET_FILL);
}

// NOTE: incomplete (FeatureDrawer::DrawShadowPass sets more state)
static void SetupShadowFeatureDrawState(unsigned int modelType, bool deferredPass) { SetupShadowUnitDrawState(modelType, deferredPass); }
static void ResetShadowFeatureDrawState(unsigned int modelType, bool deferredPass) { ResetShadowUnitDrawState(modelType, deferredPass); }




void LuaObjectDrawer::Update(bool init)
{
	if (init) {
		eventFuncs[LUAOBJ_UNIT   ] = &CEventHandler::DrawUnitsPostDeferred;
		eventFuncs[LUAOBJ_FEATURE] = &CEventHandler::DrawFeaturesPostDeferred;

		drawDeferredAllowed = configHandler->GetBool("AllowDeferredModelRendering");
		bufferClearAllowed = configHandler->GetBool("AllowDeferredModelBufferClear");

		geomBuffer = GetGeometryBuffer();

		// handle a potential reload since our buffer is static
		geomBuffer->Kill();
		geomBuffer->Init();
		geomBuffer->SetName("LUAOBJECTDRAWER-GBUFFER");
	}

	assert(geomBuffer != nullptr);

	// update buffer only if it is valid
	if (drawDeferredAllowed && (drawDeferredEnabled = geomBuffer->Valid())) {
		drawDeferredEnabled &= (geomBuffer->Update(init));

		notifyEventFlags[LUAOBJ_UNIT   ] = !unitDrawer->DrawForward();
		bufferClearFlags[LUAOBJ_UNIT   ] =  unitDrawer->DrawDeferred();
		notifyEventFlags[LUAOBJ_FEATURE] = !featureDrawer->DrawForward();
		bufferClearFlags[LUAOBJ_FEATURE] =  featureDrawer->DrawDeferred();

		// if both object types are going to be drawn deferred, only
		// reset buffer for the first s.t. just a single shading pass
		// is needed (in Lua)
		if (bufferClearFlags[LUAOBJ_UNIT] && bufferClearFlags[LUAOBJ_FEATURE]) {
			bufferClearFlags[LUAOBJ_FEATURE] = bufferClearAllowed;
		}
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
	if (shadowHandler->inShadowPass) {
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

	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_TRANSFORM_BIT);

	if (matType == LUAMAT_ALPHA || matType == LUAMAT_ALPHA_REFLECT) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.1f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);
	}

	const LuaMaterial* currMat = &LuaMaterial::defMat;

	for (auto it = bins.cbegin(); it != bins.cend(); ++it) {
		currMat = DrawMaterialBin(*it, currMat, objType, matType, deferredPass);
	}

	LuaMaterial::defMat.Execute(*currMat, deferredPass);
	luaMatHandler.ClearBins(objType, matType);

	glPopAttrib();

	inDrawPass = false;
}

const LuaMaterial* LuaObjectDrawer::DrawMaterialBin(
	const LuaMatBin* currBin,
	const LuaMaterial* currMat,
	LuaObjType objType,
	LuaMatType matType,
	bool deferredPass
) {
	currBin->Execute(*currMat, deferredPass);

	const std::vector<CSolidObject*>& objects = currBin->GetObjects(objType);

	for (const CSolidObject* obj: objects) {
		const LuaObjectMaterialData* matData = obj->GetLuaMaterialData();
		const LuaObjectLODMaterial* lodMat = matData->GetLuaLODMaterial(matType);

		lodMat->uniforms.Execute(obj);

		switch (objType) {
			case LUAOBJ_UNIT: {
				// note: only makes sense to set team-color if the
				// material has a standard (engine) shader attached
				// (check if *Shader.type == LUASHADER_{3DO,S3O})
				//
				// must use static_cast here because of GetTransformMatrix (!)
				unitDrawer->SetTeamColour(obj->team);
				unitDrawer->DrawUnit(static_cast<const CUnit*>(obj), lodMat->preDisplayList, lodMat->postDisplayList, true, false);
			} break;
			case LUAOBJ_FEATURE: {
				// also sets team-color (needs a specific alpha-value)
				featureDrawer->DrawFeature(static_cast<const CFeature*>(obj), lodMat->preDisplayList, lodMat->postDisplayList, true, false);
			} break;
			default: {
				assert(false);
			} break;
		}
	}

	return currBin;
}


void LuaObjectDrawer::DrawDeferredPass(const CSolidObject* excludeObj, LuaObjType objType)
{
	if (!drawDeferredEnabled)
		return;
	if (!geomBuffer->Valid())
		return;

	// deferred pass must be executed with GLSL base shaders so
	// bail early if the FFP state *is going to be* selected by
	// SetupForUnitDrawing, and also if our shader-path happens
	// to be ARB instead (saves an FBO bind)
	if (!unitDrawer->DrawDeferredSupported())
		return;

	geomBuffer->Bind();
	// reset the buffer (since we do not perform a single pass
	// writing both units and features into it at the same time)
	// this however forces Lua to execute two shading passes and
	// listen to the *PostDeferred events, so not ideal
	// geomBuffer->Clear();

	if (bufferClearFlags[objType])
		geomBuffer->Clear();

	switch (objType) {
		case LUAOBJ_UNIT: {
			unitDrawer->DrawOpaquePass(static_cast<const CUnit*>(excludeObj), true, false, false);
		} break;
		case LUAOBJ_FEATURE: {
			featureDrawer->DrawOpaquePass(true, false, false);
		} break;
		default: {
			assert(false);
		} break;
	}

	geomBuffer->UnBind();

	#if 0
	geomBuffer->DrawDebug(geomBuffer->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_NORMTEX));
	#endif

	if (notifyEventFlags[objType]) {
		// at this point the buffer has been filled (all standard
		// models and custom Lua material bins have been rendered
		// into it) and unbound, notify scripts
		assert(eventFuncs[objType] != nullptr);
		eventFuncs[objType](&eventHandler);
	}
}



bool LuaObjectDrawer::DrawSingleObject(CSolidObject* obj, LuaObjType objType)
{
	LuaObjectMaterialData* matData = obj->GetLuaMaterialData();
	LuaObjectLODMaterial* lodMat = nullptr;

	if (!matData->Enabled())
		return false;

	if ((lodMat = matData->GetLuaLODMaterial(GetDrawPassOpaqueMat())) == nullptr)
		return false;
	if (!lodMat->IsActive())
		return false;

	switch (objType) {
		case LUAOBJ_UNIT: {
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = SetupOpaqueUnitDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = ResetOpaqueUnitDrawState;
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = SetupOpaqueUnitDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = ResetOpaqueUnitDrawState;
		} break;
		case LUAOBJ_FEATURE: {
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = SetupOpaqueFeatureDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = ResetOpaqueFeatureDrawState;
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = SetupOpaqueFeatureDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = ResetOpaqueFeatureDrawState;
		} break;
		default: {
			assert(false);
		} break;
	}

	// use the normal scale for single-object drawing
	LuaObjectMaterialData::SetGlobalLODFactor(objType, LODScale[objType] * camera->GetLPPScale());

	// get the ref-counted actual material
	const LuaMaterial& mat = (LuaMaterial&) *lodMat->matref.GetBin();

	// NOTE: doesn't make sense to supported deferred mode for this?
	mat.Execute(LuaMaterial::defMat, false);
	lodMat->uniforms.Execute(obj);

	switch (objType) {
		case LUAOBJ_UNIT: {
			unitDrawer->SetTeamColour(obj->team);
			unitDrawer->DrawUnit(static_cast<const CUnit*>(obj), lodMat->preDisplayList, lodMat->postDisplayList, true, true);
		} break;
		case LUAOBJ_FEATURE: {
			featureDrawer->DrawFeature(static_cast<const CFeature*>(obj), lodMat->preDisplayList, lodMat->postDisplayList, true, true);
		} break;
		default: {
			assert(false);
		} break;
	}

	LuaMaterial::defMat.Execute(mat, false);
	return true;
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
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = SetupOpaqueUnitDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = ResetOpaqueUnitDrawState;
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = SetupOpaqueUnitDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = ResetOpaqueUnitDrawState;
		} break;
		case LUAOBJ_FEATURE: {
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = SetupOpaqueFeatureDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = ResetOpaqueFeatureDrawState;
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = SetupOpaqueFeatureDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = ResetOpaqueFeatureDrawState;
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
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = SetupAlphaUnitDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = ResetAlphaUnitDrawState;
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = SetupAlphaUnitDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = ResetAlphaUnitDrawState;
		} break;
		case LUAOBJ_FEATURE: {
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = SetupAlphaFeatureDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = ResetAlphaFeatureDrawState;
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = SetupAlphaFeatureDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = ResetAlphaFeatureDrawState;
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
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = SetupShadowUnitDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = ResetShadowUnitDrawState;
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = SetupShadowUnitDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = ResetShadowUnitDrawState;
		} break;
		case LUAOBJ_FEATURE: {
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = SetupShadowFeatureDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_3DO] = ResetShadowFeatureDrawState;
			luaMatHandler.setupDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = SetupShadowFeatureDrawState;
			luaMatHandler.resetDrawStateFuncs[LuaMatShader::LUASHADER_S3O] = ResetShadowFeatureDrawState;
		} break;
		default: {
			assert(false);
		} break;
	}

	// we neither want nor need a deferred shadow
	// pass for custom- or default-shader models!
	DrawMaterialBins(objType, GetDrawPassShadowMat(), false);
}

