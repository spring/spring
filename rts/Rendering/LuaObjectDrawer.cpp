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
#include "System/Util.h"


CONFIG(float, LODScale).defaultValue(1.0f);
CONFIG(float, LODScaleShadow).defaultValue(1.0f);
CONFIG(float, LODScaleReflection).defaultValue(1.0f);
CONFIG(float, LODScaleRefraction).defaultValue(1.0f);


bool LuaObjectDrawer::inDrawPass = false;

float LuaObjectDrawer::LODScale[LUAOBJ_LAST];
float LuaObjectDrawer::LODScaleShadow[LUAOBJ_LAST];
float LuaObjectDrawer::LODScaleReflection[LUAOBJ_LAST];
float LuaObjectDrawer::LODScaleRefraction[LUAOBJ_LAST];


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




void LuaObjectDrawer::ReadLODScales(LuaObjType objType)
{
	LODScale          [objType] = GetLODFloat("LODScale");
	LODScaleShadow    [objType] = GetLODFloat("LODScaleShadow");
	LODScaleReflection[objType] = GetLODFloat("LODScaleReflection");
	LODScaleRefraction[objType] = GetLODFloat("LODScaleRefraction");
}

void LuaObjectDrawer::SetGlobalDrawPassLODFactor(LuaObjType objType)
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
				// note: must use static_cast here because of GetTransformMatrix (!)
				unitDrawer->SetTeamColour(obj->team);
				unitDrawer->DrawUnitWithLists(static_cast<const CUnit*>(obj), lodMat->preDisplayList, lodMat->postDisplayList);
			} break;
			case LUAOBJ_FEATURE: {
				// also sets team-color (needs a specific alpha-value)
				featureDrawer->DrawFeatureWithLists(static_cast<const CFeature*>(obj), lodMat->preDisplayList, lodMat->postDisplayList);
			} break;
			default: {
				assert(false);
			} break;
		}
	}

	return currBin;
}

bool LuaObjectDrawer::DrawSingleObject(CSolidObject* obj, LuaObjType objType)
{
	LuaObjectMaterialData* matData = obj->GetLuaMaterialData();
	LuaObjectLODMaterial* lodMat = nullptr;

	if (!matData->Enabled())
		return false;

	if ((lodMat = matData->GetLuaLODMaterial((water->DrawReflectionPass())? LUAMAT_OPAQUE_REFLECT: LUAMAT_OPAQUE)) == nullptr)
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
			unitDrawer->DrawUnitRawWithLists(static_cast<const CUnit*>(obj), lodMat->preDisplayList, lodMat->postDisplayList);
		} break;
		case LUAOBJ_FEATURE: {
			// not implemented (nor "raw" versions in general)
			// featureDrawer->DrawFeatureRawWithLists(static_cast<const CFeature*>(obj), lodMat->preDisplayList, lodMat->postDisplayList);
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
	// features do not have a LocalModel, sometimes not even a 3DModel
	// LuaMaterialData should be stored inside one of those structures,
	// but for now is held by SolidObject to avoid introducing dummies
	obj->GetLuaMaterialData()->SetLODCount(lodCount);

	if (objType == LUAOBJ_UNIT) {
		(static_cast<CUnit*>(obj))->localModel->SetLODCount(lodCount);
	}
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

	DrawMaterialBins(objType, (water->DrawReflectionPass())? LUAMAT_OPAQUE_REFLECT: LUAMAT_OPAQUE, deferredPass);
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
	DrawMaterialBins(objType, (water->DrawReflectionPass())? LUAMAT_ALPHA_REFLECT: LUAMAT_ALPHA, false);
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
	DrawMaterialBins(objType, LUAMAT_SHADOW, false);
}

