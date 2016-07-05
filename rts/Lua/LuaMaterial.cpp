/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaMaterial.h"

#include "LuaHandle.h"
#include "LuaOpenGL.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h" // randVector
#include "Rendering/GlobalRendering.h" // drawFrame
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawerState.hpp"
#include "Rendering/Env/ISky.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Misc/GlobalSynced.h" // simFrame
#include "System/Log/ILog.h"
#include "System/Util.h"


LuaMatHandler LuaMatHandler::handler;
LuaMatHandler& luaMatHandler = LuaMatHandler::handler;


/******************************************************************************/
/******************************************************************************/
//
//  Helper
//

static const char* GlUniformTypeToString(const GLenum uType)
{
	switch (uType) {
		case      GL_FLOAT: return "float";
		case GL_FLOAT_VEC2: return "vec2";
		case GL_FLOAT_VEC3: return "vec3";
		case GL_FLOAT_VEC4: return "vec4";
		case        GL_INT: return "sampler/int/bool";
		case   GL_INT_VEC2: return "ivec2/bvec2";
		case   GL_INT_VEC3: return "ivec3/bvec3";
		case   GL_INT_VEC4: return "ivec4/bvec4";
		case GL_FLOAT_MAT2: return "mat2";
		case GL_FLOAT_MAT3: return "mat3";
		case GL_FLOAT_MAT4: return "mat4";
	}
	return "unknown";
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaObjectMaterial
//

bool LuaObjectMaterial::SetLODCount(unsigned int count)
{
	lodCount = count;
	lastLOD = lodCount - 1;
	lodMats.resize(count);
	return true;
}


bool LuaObjectMaterial::SetLastLOD(unsigned int lod)
{
	lastLOD = std::min(lod, lodCount - 1);
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaMatShader
//

void LuaMatShader::Finalize()
{
	if (type != LUASHADER_GL) {
		openglID = 0;
	}
}


int LuaMatShader::Compare(const LuaMatShader& a, const LuaMatShader& b)
{
	if (a.type != b.type)
		return ((a.type > b.type) * 2 - 1);

	if (a.openglID != b.openglID) {
		return ((a.openglID > b.openglID) * 2 - 1);
	}

	return 0;
}


void LuaMatShader::Execute(const LuaMatShader& prev, bool deferredPass) const
{
	if (type != prev.type) {
		switch (prev.type) {
			case LUASHADER_GL: {
				glUseProgram(0);
			} break;
			case LUASHADER_3DO:
			case LUASHADER_S3O:
			case LUASHADER_OBJ:
			case LUASHADER_ASS: {
				if (luaMatHandler.resetDrawStateFuncs[prev.type]) {
					luaMatHandler.resetDrawStateFuncs[prev.type](prev.type, deferredPass);
				}
			} break;
			case LUASHADER_NONE: {} break;
			default: { assert(false); } break;
		}

		switch (type) {
			case LUASHADER_GL: {
				// custom shader
				glUseProgram(openglID);
			} break;
			case LUASHADER_3DO:
			case LUASHADER_S3O:
			case LUASHADER_OBJ:
			case LUASHADER_ASS: {
				if (luaMatHandler.setupDrawStateFuncs[type]) {
					luaMatHandler.setupDrawStateFuncs[type](type, deferredPass);
				}
			} break;
			case LUASHADER_NONE: {} break;
			default: { assert(false); } break;
		}
	}
	else if (type == LUASHADER_GL) {
		if (openglID != prev.openglID) {
			glUseProgram(openglID);
		}
	}
}


void LuaMatShader::Print(const string& indent, bool isDeferred) const
{
	const char* typeName = "Unknown";

	switch (type) {
		case LUASHADER_NONE: { typeName = "LUASHADER_NONE"; } break;
		case LUASHADER_GL:   { typeName = "LUASHADER_GL"  ; } break;
		case LUASHADER_3DO:  { typeName = "LUASHADER_3DO" ; } break;
		case LUASHADER_S3O:  { typeName = "LUASHADER_S3O" ; } break;
		case LUASHADER_OBJ:  { typeName = "LUASHADER_OBJ" ; } break;
		case LUASHADER_ASS:  { typeName = "LUASHADER_ASS" ; } break;
		default: { assert(false); } break;
	}

	LOG("%s[shader][%s]", indent.c_str(), (isDeferred? "deferred": "standard"));
	LOG("%s  type=%s", indent.c_str(), typeName);
	LOG("%s  glID=%u", indent.c_str(), openglID);
}




/******************************************************************************/
/******************************************************************************/
//
//  LuaMaterial
//

const LuaMaterial LuaMaterial::defMat;

void LuaMaterial::Parse(
	lua_State* L,
	const int tableIdx,
	std::function<void(lua_State*, int, LuaMatShader&)> ParseShader,
	std::function<void(lua_State*, int, LuaMatTexture&)> ParseTexture,
	std::function<GLuint(lua_State*, int)> ParseDisplayList
) {
	for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2))
			continue;

		const std::string key = StringToLower(lua_tostring(L, -2));

		// uniforms
		if (key.find("uniforms") != std::string::npos) {
			if (!lua_istable(L, -1))
				continue;

			if (key.find("standard") != std::string::npos) {
				uniforms[LuaMatShader::LUASHADER_PASS_FWD].Parse(L, lua_gettop(L));
				continue;
			}
			if (key.find("deferred") != std::string::npos) {
				uniforms[LuaMatShader::LUASHADER_PASS_DFR].Parse(L, lua_gettop(L));
				continue;
			}

			// fallback
			uniforms[LuaMatShader::LUASHADER_PASS_FWD].Parse(L, lua_gettop(L));
			continue;
		}

		// shaders
		if (key.find("shader") != std::string::npos) {
			if (key.find("standard") != std::string::npos) {
				ParseShader(L, -1, shaders[LuaMatShader::LUASHADER_PASS_FWD]);
				continue;
			}
			if (key.find("deferred") != std::string::npos) {
				ParseShader(L, -1, shaders[LuaMatShader::LUASHADER_PASS_DFR]);
				continue;
			}

			// fallback
			ParseShader(L, -1, shaders[LuaMatShader::LUASHADER_PASS_FWD]);
			continue;
		}

		// textures
		if (key.substr(0, 7) == "texunit") {
			if (key.size() < 8)
				continue;

			if (key[7] == 's') {
				// "texunits" = {[0] = string|table, ...}
				if (!lua_istable(L, -1))
					continue;

				const int texTable = lua_gettop(L);

				for (lua_pushnil(L); lua_next(L, texTable) != 0; lua_pop(L, 1)) {
					if (!lua_israwnumber(L, -2))
						continue;

					const unsigned int texUnit = lua_toint(L, -2);

					if (texUnit >= LuaMatTexture::maxTexUnits)
						continue;

					ParseTexture(L, -1, textures[texUnit]);
				}
			} else {
				// "texunitX" = string|table
				const unsigned int texUnit = atoi(key.c_str() + 7);

				if (texUnit >= LuaMatTexture::maxTexUnits)
					continue;

				ParseTexture(L, -1, textures[texUnit]);
			}

			continue;
		}

		// dlists
		if (key == "prelist") {
			preList = ParseDisplayList(L, -1);
			continue;
		}
		if (key == "postlist") {
			postList = ParseDisplayList(L, -1);
			continue;
		}

		// misc
		if (key == "order") {
			order = luaL_checkint(L, -1);
			continue;
		}
		if (key == "culling") {
			if (lua_isnumber(L, -1))
				cullingMode = (GLenum)lua_tonumber(L, -1);

			continue;
		}

		if (key == "usecamera") {
			useCamera = lua_isboolean(L, -1) && lua_toboolean(L, -1);
			continue;
		}

		LOG_L(L_WARNING, "LuaMaterial: incorrect key \"%s\"", key.c_str());
	}
}


void LuaMaterial::Finalize()
{
	for (int i: {LuaMatShader::LUASHADER_PASS_FWD, LuaMatShader::LUASHADER_PASS_DFR}) {
		shaders[i].Finalize();
		uniforms[i].AutoLink(&shaders[i]);
		uniforms[i].Validate(&shaders[i]);
	}

	for (int t = 0; t < LuaMatTexture::maxTexUnits; t++) {
		textures[t].Finalize();

		if (textures[t].type != LuaMatTexture::LUATEX_NONE) {
			texCount = t + 1;
		}
	}
}


void LuaMaterial::Execute(const LuaMaterial& prev, bool deferredPass) const
{
	if (prev.postList != 0)
		glCallList(prev.postList);
	if (preList != 0)
		glCallList(preList);

	shaders[deferredPass].Execute(prev.shaders[deferredPass], deferredPass);
	uniforms[deferredPass].Execute();

	for (int t = 0; t < std::max(texCount, prev.texCount); ++t) {
		if (prev.textures[t] != textures[t]) {
			glActiveTexture(GL_TEXTURE0 + t);
			prev.textures[t].Unbind();
			textures[t].Bind();
		}
	}
	glActiveTexture(GL_TEXTURE0);

	if (useCamera != prev.useCamera) {
		if (useCamera) {
			glPopMatrix();
		} else {
			glPushMatrix();
			glLoadIdentity();
		}
	}

	if (cullingMode != prev.cullingMode) {
		if (cullingMode != 0) {
			glEnable(GL_CULL_FACE);
			glCullFace(cullingMode);
		} else {
			glDisable(GL_CULL_FACE);
		}
	}
}

void LuaMaterial::ExecuteInstance(bool deferredPass, const CSolidObject* o, const float2 alpha) const
{
	uniforms[deferredPass].ExecuteInstance(o, alpha);
}


int LuaMaterial::Compare(const LuaMaterial& a, const LuaMaterial& b)
{
	// NOTE: the order of the comparisons is important (it's sorted by the GL perf cost of switching those states)
	int cmp = 0;

	if (a.type != b.type)
		return ((a.type > b.type) * 2 - 1);  // should not happen
	if (a.order != b.order)
		return ((a.order > b.order) * 2 - 1);


	if ((cmp = LuaMatShader::Compare(a.shaders[LuaMatShader::LUASHADER_PASS_FWD], b.shaders[LuaMatShader::LUASHADER_PASS_FWD])) != 0)
		return cmp;
	if ((cmp = LuaMatShader::Compare(a.shaders[LuaMatShader::LUASHADER_PASS_DFR], b.shaders[LuaMatShader::LUASHADER_PASS_DFR])) != 0)
		return cmp;

	for (int t = 0; t < LuaMatTexture::maxTexUnits; t++) {
		if ((cmp = LuaMatTexture::Compare(a.textures[t], b.textures[t])) != 0) {
			return cmp;
		}
	}

	if (a.useCamera != b.useCamera)
		return ((!a.useCamera) * 2 - 1);

	if (a.cullingMode != b.cullingMode)
		return ((a.cullingMode > b.cullingMode) * 2 - 1);


	if (a.preList != b.preList)
		return ((a.preList > b.preList) * 2 - 1);
	if (a.postList != b.postList)
		return ((a.postList > b.postList) * 2 - 1);

	// have no influence on the performance cost
	// still LuaMaterial/LuaMatBin are used in a std::set (:=LuaMatBinSet)
	// and there there < operator is also used to detect equality (a == b iff !(a<b) && !(b<a))
	// that's why need to check _all_ properties
	if ((cmp = LuaMatUniforms::Compare(a.uniforms[LuaMatShader::LUASHADER_PASS_FWD], b.uniforms[LuaMatShader::LUASHADER_PASS_FWD])) != 0)
		return cmp;
	if ((cmp = LuaMatUniforms::Compare(a.uniforms[LuaMatShader::LUASHADER_PASS_DFR], b.uniforms[LuaMatShader::LUASHADER_PASS_DFR])) != 0)
		return cmp;

	return 0;
}



int LuaMatUniforms::Compare(const LuaMatUniforms& a, const LuaMatUniforms& b)
{
	if (a.viewMatrix.loc != b.viewMatrix.loc)
		return ((a.viewMatrix.loc > b.viewMatrix.loc) * 2 - 1);
	if (a.projMatrix.loc != b.projMatrix.loc)
		return ((a.projMatrix.loc > b.projMatrix.loc) * 2 - 1);
	if (a.viprMatrix.loc != b.viprMatrix.loc)
		return ((a.viprMatrix.loc > b.viprMatrix.loc) * 2 - 1);

	if (a.viewMatrixInv.loc != b.viewMatrixInv.loc)
		return ((a.viewMatrixInv.loc > b.viewMatrixInv.loc) * 2 - 1);
	if (a.projMatrixInv.loc != b.projMatrixInv.loc)
		return ((a.projMatrixInv.loc > b.projMatrixInv.loc) * 2 - 1);
	if (a.viprMatrixInv.loc != b.viprMatrixInv.loc)
		return ((a.viprMatrixInv.loc > b.viprMatrixInv.loc) * 2 - 1);

	if (a.shadowMatrix.loc != b.shadowMatrix.loc)
		return ((a.shadowMatrix.loc > b.shadowMatrix.loc) * 2 - 1);
	if (a.shadowParams.loc != b.shadowParams.loc)
		return ((a.shadowParams.loc > b.shadowParams.loc) * 2 - 1);

	if (a.camPos.loc != b.camPos.loc)
		return ((a.camPos.loc > b.camPos.loc) * 2 - 1);
	if (a.camDir.loc != b.camDir.loc)
		return ((a.camDir.loc > b.camDir.loc) * 2 - 1);
	if (a.sunDir.loc != b.sunDir.loc)
		return ((a.sunDir.loc > b.sunDir.loc) * 2 - 1);
	if (a.rndVec.loc != b.rndVec.loc)
		return ((a.rndVec.loc > b.rndVec.loc) * 2 - 1);

	if (a.simFrame.loc != b.simFrame.loc)
		return ((a.simFrame.loc > b.simFrame.loc) * 2 - 1);
	if (a.visFrame.loc != b.visFrame.loc)
		return ((a.visFrame.loc > b.visFrame.loc) * 2 - 1);

	if (a.teamColor.loc != b.teamColor.loc)
		return ((a.teamColor.loc > b.teamColor.loc) * 2 - 1);

	return 0;
}

static const char* GetMatTypeName(LuaMatType type)
{
	const char* typeName = "Unknown";

	switch (type) {
		case LUAMAT_ALPHA:          { typeName = "LUAMAT_ALPHA";          } break;
		case LUAMAT_OPAQUE:         { typeName = "LUAMAT_OPAQUE";         } break;
		case LUAMAT_ALPHA_REFLECT:  { typeName = "LUAMAT_ALPHA_REFLECT";  } break;
		case LUAMAT_OPAQUE_REFLECT: { typeName = "LUAMAT_OPAQUE_REFLECT"; } break;
		case LUAMAT_SHADOW:         { typeName = "LUAMAT_SHADOW";         } break;

		case LUAMAT_TYPE_COUNT: {
		} break;
	}

	return typeName;
}


void LuaMaterial::Print(const string& indent) const
{
#define CULL_TO_STR(x) \
	(x==GL_FRONT) ? "front" : (x==GL_BACK) ? "back" : (x!=0) ? "false" : "unknown"

	LOG("%s%s", indent.c_str(), GetMatTypeName(type));
	LOG("%sorder = %i", indent.c_str(), order);

	shaders[LuaMatShader::LUASHADER_PASS_FWD].Print(indent, false);
	shaders[LuaMatShader::LUASHADER_PASS_DFR].Print(indent,  true);

	uniforms[LuaMatShader::LUASHADER_PASS_FWD].Print(indent, false);
	uniforms[LuaMatShader::LUASHADER_PASS_DFR].Print(indent,  true);

	LOG("%stexCount = %i", indent.c_str(), texCount);

	for (int t = 0; t < texCount; t++) {
		char buf[32];
		SNPRINTF(buf, sizeof(buf), "%s  tex[%i] ", indent.c_str(), t);
		textures[t].Print(buf);
	}

	LOG("%spreList  = %i",  indent.c_str(), preList);
	LOG("%spostList = %i",  indent.c_str(), postList);

	LOG("%suseCamera   = %s", indent.c_str(), (useCamera ? "true" : "false"));
	LOG("%scullingMode = %s", indent.c_str(), CULL_TO_STR(cullingMode));
}





/******************************************************************************/
/******************************************************************************/
//
//  LuaMatUniforms
//

std::unordered_map<LuaMatUniforms::IUniform*, std::string> LuaMatUniforms::GetUniformsAndStandardName()
{
	return {
		{&viewMatrix,    "ViewMatrix"},
		{&projMatrix,    "ProjectionMatrix"},
		{&viprMatrix,    "ViewProjectionMatrix"},
		{&viewMatrixInv, "ViewMatrixInverse"},
		{&projMatrixInv, "ProjectionMatrixInverse"},
		{&viprMatrixInv, "ViewProjectionMatrixInverse"},
		{&shadowMatrix,  "ShadowMatrix"},
		{&shadowParams,  "ShadowParams"},
		{&camPos,        "CameraPos"},
		{&camDir,        "CameraDir"},
		{&sunDir,        "SunDir"},
		{&rndVec,        "RndVec"},
		{&simFrame,      "SimFrame"},
		{&visFrame,      "VisFrame"},
		{&teamColor,     "TeamColor"},
	};
}


std::unordered_map<std::string, LuaMatUniforms::IUniform*> LuaMatUniforms::GetUniformsAndPossibleNames()
{
	return {
		{"Camera",                  &viewMatrix},
		{"ViewMatrix",              &viewMatrix},
		{"ProjMatrix",              &projMatrix},
		{"ProjectionMatrix",        &projMatrix},
		{"ViPrMatrix",              &viprMatrix},
		{"ViewProjMatrix",          &viprMatrix},
		{"ViewProjectionMatrix",    &viprMatrix},
		{"CameraInv",               &viewMatrixInv},
		{"ViewMatrixInv",           &viewMatrixInv},
		{"ProjMatrixInv",           &projMatrixInv},
		{"ProjectionMatrixInv",     &projMatrixInv},
		{"ViPrMatrixInv",           &viprMatrixInv},
		{"ViewProjectionMatrixInv", &viprMatrixInv},

		{"Shadow",        &shadowMatrix},
		{"ShadowMatrix",  &shadowMatrix},
		{"ShadowParams",  &shadowParams},

		{"CameraPos",     &camPos},
		{"CameraDir",     &camDir},
		{"SunPos",        &sunDir},
		{"SunDir",        &sunDir},
		{"RndVec",        &rndVec},

		{"SimFrame",      &simFrame},
		{"VisFrame",      &visFrame},
		{"DrawFrame",     &visFrame},

		//{"speed", },
		//{"health", },
		{"TeamColor", &teamColor},
	};
}


void LuaMatUniforms::AutoLink(LuaMatShader* shader)
{
	if (!shader->IsCustomType())
		return;

	for (const auto& p: GetUniformsAndPossibleNames()) {
		if (p.second->IsValid())
			continue;

		// try CamelCase
		p.second->loc = glGetUniformLocation(shader->openglID, p.first.c_str());
		if (p.second->IsValid()) continue;

		// try camelCase
		std::string s = p.first; s[0] = tolower(s[0]);
		p.second->loc = glGetUniformLocation(shader->openglID, s.c_str());
		if (p.second->IsValid()) continue;

		// try lowercase
		StringToLowerInPlace(s);
		p.second->loc = glGetUniformLocation(shader->openglID, s.c_str());
	}
}


void LuaMatUniforms::Validate(LuaMatShader* s)
{
	if (!s->IsCustomType()) {
		// print warning when uniforms are given for engine shaders
		for (const auto& p: GetUniformsAndStandardName()) {
			if (!p.first->IsValid())
				continue;

			LOG_L(L_WARNING, "LuaMaterial: engine shaders prohibit the usage of uniform \"%s\"", p.second.c_str());
		}
		return;
	}

	// print warning when teamcolor is not bound
	if (!teamColor.IsValid()) {
		LOG_L(L_WARNING, "LuaMaterial: missing TeamColor uniform");
	}

	// check uniform types
	for (const auto& p: GetUniformsAndStandardName()) {
		if (!p.first->IsValid())
			continue;

		GLint size = 0;
		GLenum uniformType = 0;
		glGetActiveUniform(s->openglID, p.first->loc, 0, nullptr, &size, &uniformType, nullptr);

		if (p.first->GetType() == uniformType)
			continue;

		LOG_L(L_WARNING, "LuaMaterial: incorrect uniform type for \"%s\", got: %s expected: %s", p.second.c_str(), GlUniformTypeToString(uniformType), GlUniformTypeToString(p.first->GetType()));
	}
}


void LuaMatUniforms::Parse(lua_State* L, const int tableIdx)
{
	auto uniformAndNames = GetUniformsAndPossibleNames();
	decltype(uniformAndNames) uniformAndNamesLower;
	for (auto& p: uniformAndNames) {
		uniformAndNamesLower[StringToLower(p.first)] = p.second;
	}

	for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2) || !lua_isnumber(L, -1))
			continue;

		// get the lua table key
		// and remove the "loc" at the end (cameraPosLoc -> camerapos)
		std::string uniformLocStr = luaL_tosstring(L, -2);
		StringToLowerInPlace(uniformLocStr);
		if (StringEndsWith(uniformLocStr, "loc")) {
			uniformLocStr.resize(uniformLocStr.size() - 3);
		}

		const auto uniformLocIt = uniformAndNamesLower.find(uniformLocStr);
		if (uniformLocIt == uniformAndNamesLower.end()) {
			LOG_L(L_WARNING, "LuaMaterial: unknown uniform \"%s\"", lua_tostring(L, -2));
			continue;
		}

		uniformLocIt->second->loc = static_cast<GLint>(lua_tonumber(L, -1));
	}
}

void LuaMatUniforms::Execute() const
{
	viewMatrix.Execute(camera->GetViewMatrix());
	projMatrix.Execute(camera->GetProjectionMatrix());
	viprMatrix.Execute(camera->GetViewProjectionMatrix());

	viewMatrixInv.Execute(camera->GetViewMatrixInverse());
	projMatrixInv.Execute(camera->GetProjectionMatrixInverse());
	viprMatrixInv.Execute(camera->GetViewProjectionMatrixInverse());

	shadowMatrix.Execute(shadowHandler->GetShadowMatrix());
	shadowParams.Execute(shadowHandler->GetShadowParams());

	camPos.Execute(camera->GetPos());
	camDir.Execute(camera->GetDir());
	sunDir.Execute(sky->GetLight()->GetLightDir());
	rndVec.Execute(gu->RandVector());

	simFrame.Execute(gs->frameNum);
	visFrame.Execute(globalRendering->drawFrame);
}

void LuaMatUniforms::ExecuteInstance(const CSolidObject* o, const float2 alpha) const
{
	teamColor.Execute(IUnitDrawerState::GetTeamColor(o->team, alpha.x));
	//{"speed", },
	//{"health", },
}

void LuaMatUniforms::Print(const string& indent, bool isDeferred) const
{
	LOG("%s[uniforms][%s]", indent.c_str(), (isDeferred? "deferred": "standard"));
	LOG("%s  viewMatrixLoc    = %i", indent.c_str(), viewMatrix.loc);
	LOG("%s  projMatrixLoc    = %i", indent.c_str(), projMatrix.loc);
	LOG("%s  viewMatrixInvLoc = %i", indent.c_str(), viewMatrixInv.loc);
	LOG("%s  projMatrixInvLoc = %i", indent.c_str(), projMatrixInv.loc);

	LOG("%s  shadowMatrixLoc  = %i", indent.c_str(), shadowMatrix.loc);
	LOG("%s  shadowParamsLoc  = %i", indent.c_str(), shadowParams.loc);

	LOG("%s  camPosLoc        = %i", indent.c_str(), camPos.loc);
	LOG("%s  camDirLoc        = %i", indent.c_str(), camDir.loc);
	LOG("%s  sunDirLoc        = %i", indent.c_str(), sunDir.loc);
	LOG("%s  rndVecLoc        = %i", indent.c_str(), rndVec.loc);

	LOG("%s  simFrameLoc      = %i", indent.c_str(), simFrame.loc);
	LOG("%s  visFrameLoc      = %i", indent.c_str(), visFrame.loc);
}






/******************************************************************************/
/******************************************************************************/
//
//  LuaMatRef
//

LuaMatRef::LuaMatRef(LuaMatBin* _bin)
{
	bin = _bin;
	if (bin != NULL) {
		bin->Ref();
	}
}


LuaMatRef::~LuaMatRef()
{
	if (bin != NULL) {
		bin->UnRef();
	}
}


void LuaMatRef::Reset()
{
	if (bin != NULL) {
		bin->UnRef();
	}
	bin = NULL;
}


LuaMatRef& LuaMatRef::operator=(const LuaMatRef& mr)
{
	if (mr.bin != NULL) { mr.bin->Ref();   }
	if (   bin != NULL) {    bin->UnRef(); }
	bin = mr.bin;
	return *this;
}


LuaMatRef::LuaMatRef(const LuaMatRef& mr)
{
	bin = mr.bin;
	if (bin != NULL) {
		bin->Ref();
	}
}


void LuaMatRef::AddUnit(CSolidObject* o)
{
	if (bin != NULL) {
		bin->AddUnit(o);
	}
}

void LuaMatRef::AddFeature(CSolidObject* o)
{
	if (bin != NULL) {
		bin->AddFeature(o);
	}
}



/******************************************************************************/
/******************************************************************************/
//
//  LuaMatBin
//

void LuaMatBin::Ref()
{
	refCount++;
}


void LuaMatBin::UnRef()
{
	refCount--;
	if (refCount <= 0) {
		luaMatHandler.FreeBin(this);
	}
}


void LuaMatBin::Print(const string& indent) const
{
	LOG("%s|units| = " _STPF_, indent.c_str(), units.size());
	LOG("%s|features| = " _STPF_, indent.c_str(), features.size());
	LOG("%spointer = %p", indent.c_str(), this);
	LuaMaterial::Print(indent + "  ");
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaMatHandler
//

LuaMatHandler::LuaMatHandler()
{
	for (unsigned int i = LuaMatShader::LUASHADER_NONE; i < LuaMatShader::LUASHADER_LAST; i++) {
		setupDrawStateFuncs[i] = nullptr;
		resetDrawStateFuncs[i] = nullptr;
	}
}


LuaMatHandler::~LuaMatHandler()
{
	for (int m = 0; m < LUAMAT_TYPE_COUNT; m++) {
		for (LuaMatBin* bin: binTypes[LuaMatType(m)]) {
			delete bin;
		}
	}
}


LuaMatRef LuaMatHandler::GetRef(const LuaMaterial& mat)
{
	if ((mat.type < 0) || (mat.type >= LUAMAT_TYPE_COUNT)) {
		LOG_L(L_WARNING, "LuaMatHandler::GetRef() untyped material");
		return LuaMatRef();
	}

	LuaMatBin* fakeBin = (LuaMatBin*) &mat;

	LuaMatBinSet& binSet = binTypes[mat.type];
	LuaMatBinSet::iterator it = binSet.find(fakeBin);

	if (it != binSet.end()) {
		assert(*fakeBin == **it);
		return LuaMatRef(*it);
	}

	LuaMatBin* bin = new LuaMatBin(mat);
	binSet.insert(bin);
	return LuaMatRef(bin);
}


void LuaMatHandler::ClearBins(LuaObjType objType, LuaMatType matType)
{
	if ((matType < 0) || (matType >= LUAMAT_TYPE_COUNT))
		return;

	for (LuaMatBin* bin: binTypes[matType]) {
		switch (objType) {
			case LUAOBJ_UNIT   : { bin->ClearUnits   (); } break;
			case LUAOBJ_FEATURE: { bin->ClearFeatures(); } break;
			default            : {        assert(false); } break;
		}
	}
}


void LuaMatHandler::ClearBins(LuaObjType objType)
{
	for (int m = 0; m < LUAMAT_TYPE_COUNT; m++) {
		ClearBins(objType, LuaMatType(m));
	}
}


void LuaMatHandler::FreeBin(LuaMatBin* bin)
{
	LuaMatBinSet& binSet = binTypes[bin->type];
	LuaMatBinSet::iterator it = binSet.find(bin);
	if (it != binSet.end()) {
		delete bin;
		binSet.erase(it);
	}
}


void LuaMatHandler::PrintBins(const string& indent, LuaMatType type) const
{
	if ((type < 0) || (type >= LUAMAT_TYPE_COUNT)) {
		return;
	}
	int num = 0;
	LOG("%sBINCOUNT = " _STPF_, indent.c_str(), binTypes[type].size());
	for (LuaMatBin* bin: binTypes[type]) {
		LOG("%sBIN %i:", indent.c_str(), num);
		bin->Print(indent + "    ");
		num++;
	}
}


void LuaMatHandler::PrintAllBins(const string& indent) const
{
	for (int m = 0; m < LUAMAT_TYPE_COUNT; m++) {
		std::string newIndent = indent + GetMatTypeName(LuaMatType(m));
		newIndent += "  ";
		PrintBins(newIndent, LuaMatType(m));
	}
}


/******************************************************************************/
/******************************************************************************/

float LuaObjectMaterialData::GLOBAL_LOD_FACTORS[LUAOBJ_LAST] = {1.0f, 1.0f};

