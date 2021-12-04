/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaMaterial.h"

#include "LuaHandle.h"
#include "LuaOpenGL.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h" // randVector
#include "Rendering/GlobalRendering.h" // drawFrame
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/ISky.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Misc/GlobalSynced.h" // simFrame
#include "System/Log/ILog.h"
#include "System/StringUtil.h"

#include <cctype>

struct ActiveUniform {
	GLint size = 0;
	GLenum type = 0;
	GLchar name[512] = {0};
};

struct LuaMatBinPtrLessThan {
	bool operator()(const LuaMatBin* a, const LuaMatBin* b) const {
		const LuaMaterial* ma = static_cast<const LuaMaterial*>(a);
		const LuaMaterial* mb = static_cast<const LuaMaterial*>(b);
		return (*ma < *mb);
	}
};

LuaMatHandler LuaMatHandler::handler;
LuaMatHandler& luaMatHandler = LuaMatHandler::handler;
LuaMatBinPtrLessThan matBinCmp;

/******************************************************************************/
/******************************************************************************/
//
//  Helper
//

static const char* GLUniformTypeToString(const GLenum uType)
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

int LuaMatShader::Compare(const LuaMatShader& a, const LuaMatShader& b)
{
	if (a.type != b.type)
		return ((a.type > b.type) * 2 - 1);

	if (a.openglID != b.openglID)
		return ((a.openglID > b.openglID) * 2 - 1);

	return 0;
}


void LuaMatShader::Execute(const LuaMatShader& prev, bool deferredPass) const
{
	static_assert(int(LUASHADER_3DO) == int(MODELTYPE_3DO  ), "");
	static_assert(int(LUASHADER_S3O) == int(MODELTYPE_S3O  ), "");
	static_assert(int(LUASHADER_ASS) == int(MODELTYPE_ASS  ), "");
	static_assert(int(LUASHADER_GL ) == int(MODELTYPE_OTHER), "");

	if (type != prev.type) {
		// reset old, setup new
		switch (prev.type) {
			case LUASHADER_GL: {
				glUseProgram(0);

				assert(luaMatHandler.resetDrawStateFuncs[prev.type] != nullptr);
				luaMatHandler.resetDrawStateFuncs[prev.type](prev.type, deferredPass);
			} break;
			case LUASHADER_3DO:
			case LUASHADER_S3O:
			case LUASHADER_ASS: {
				assert(luaMatHandler.resetDrawStateFuncs[prev.type] != nullptr);
				luaMatHandler.resetDrawStateFuncs[prev.type](prev.type, deferredPass);
			} break;
			default: { assert(false); } break;
		}

		switch (type) {
			case LUASHADER_GL: {
				// custom shader
				glUseProgram(openglID);

				assert(luaMatHandler.setupDrawStateFuncs[type] != nullptr);
				luaMatHandler.setupDrawStateFuncs[type](type, deferredPass);
			} break;
			case LUASHADER_3DO:
			case LUASHADER_S3O:
			case LUASHADER_ASS: {
				assert(luaMatHandler.setupDrawStateFuncs[type] != nullptr);
				luaMatHandler.setupDrawStateFuncs[type](type, deferredPass);
			} break;
			default: { assert(false); } break;
		}

		return;
	}

	if (type != LUASHADER_GL)
		return;
	if (openglID == prev.openglID)
		return;

	glUseProgram(openglID);
}


void LuaMatShader::Print(const string& indent, bool isDeferred) const
{
	const char* typeName = "Unknown";

	switch (type) {
		case LUASHADER_GL : { typeName = "LUASHADER_GL"  ; } break;
		case LUASHADER_3DO: { typeName = "LUASHADER_3DO" ; } break;
		case LUASHADER_S3O: { typeName = "LUASHADER_S3O" ; } break;
		case LUASHADER_ASS: { typeName = "LUASHADER_ASS" ; } break;
		default           : {               assert(false); } break;
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
	void(*ParseShader)(lua_State*, int, LuaMatShader&),
	void(*ParseTexture)(lua_State*, int, LuaMatTexture&),
	GLuint(*ParseDisplayList)(lua_State*, int)
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

		// misc
		if (key == "uuid") {
			uuid = luaL_checkint(L, -1);
			continue;
		}
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
			LOG_L(L_WARNING, "[LuaMaterial::%s] '%s' parameter is deprecated", __func__, key.c_str());
			continue;
		}

		LOG_L(L_WARNING, "[LuaMaterial::%s] incorrect key \"%s\"", __func__, key.c_str());
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

	if (cullingMode != prev.cullingMode) {
		if (cullingMode != 0) {
			glAttribStatePtr->EnableCullFace();
			glAttribStatePtr->CullFace(cullingMode);
		} else {
			glAttribStatePtr->DisableCullFace();
		}
	}
}

void LuaMaterial::ExecuteInstanceUniforms(int objId, int objType, bool deferredPass) const
{
	const LuaMatShader&   matShader   =  shaders[deferredPass];
	const LuaMatUniforms& matUniforms = uniforms[deferredPass];

	const auto& objUniforms = matUniforms.objectUniforms[objType];
	const auto& objUniformsIt = objUniforms.find(objId);

	if (objUniformsIt == objUniforms.end())
		return;

	// apply custom per-object LuaMaterial uniforms (if any)
	// can stop at first empty slot, Clear ensures contiguity
	for (const LuaMatUniform& u: objUniformsIt->second) {
		switch (u.loc) {
			case -3: {   return; } break;
			case -2: {
				if ((u.loc = glGetUniformLocation(matShader.openglID, u.name)) == -1) {
					LOG_L(L_WARNING, "[LuaMaterial::%s(objId=%d objType=%d)][uuid=%d] uniform \"%s\" not present in %s shader for material %s", __func__, objId, objType, uuid, u.name, deferredPass? "deferred": "forward", GetMatTypeName(type));
					continue;
				}
			} break;
			case -1: { continue; } break;
			default: {           } break;
		}

		switch (u.type) {
			case GL_INT       : { glUniform1iv      (u.loc, u.size       , u.data.i); } break;
			case GL_INT_VEC2  : { glUniform2iv      (u.loc, u.size       , u.data.i); } break;
			case GL_INT_VEC3  : { glUniform3iv      (u.loc, u.size       , u.data.i); } break;
			case GL_INT_VEC4  : { glUniform4iv      (u.loc, u.size       , u.data.i); } break;

			case GL_FLOAT     : { glUniform1fv      (u.loc, u.size       , u.data.f); } break;
			case GL_FLOAT_VEC2: { glUniform2fv      (u.loc, u.size       , u.data.f); } break;
			case GL_FLOAT_VEC3: { glUniform3fv      (u.loc, u.size       , u.data.f); } break;
			case GL_FLOAT_VEC4: { glUniform4fv      (u.loc, u.size       , u.data.f); } break;

			case GL_FLOAT_MAT3: { glUniformMatrix3fv(u.loc, u.size, false, u.data.f); } break;
			case GL_FLOAT_MAT4: { glUniformMatrix4fv(u.loc, u.size, false, u.data.f); } break;
			default           : {                                                     } break;
		}
	}
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

	if (a.cullingMode != b.cullingMode)
		return ((a.cullingMode > b.cullingMode) * 2 - 1);


	// these have no influence on the performance cost
	// previously LuaMaterial/LuaMatBin were kept in a std::set (LuaMatBinSet, now a sorted std::vector)
	// for which the comparison operator < was also used to detect equality (a == b iff !(a<b) && !(b<a))
	// so check _all_ properties
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

	if (a.modelMatrix.loc != b.modelMatrix.loc)
		return ((a.modelMatrix.loc > b.modelMatrix.loc) * 2 - 1);
	if (a.pieceMatrices.loc != b.pieceMatrices.loc)
		return ((a.pieceMatrices.loc > b.pieceMatrices.loc) * 2 - 1);

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


void LuaMaterial::Print(const string& indent) const
{
#define CULL_TO_STR(x) \
	(x==GL_FRONT) ? "front" : (x==GL_BACK) ? "back" : (x!=0) ? "false" : "unknown"

	LOG("%s%s", indent.c_str(), GetMatTypeName(type));
	LOG("%suuid = %i", indent.c_str(), uuid);
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

	LOG("%scullingMode = %s", indent.c_str(), CULL_TO_STR(cullingMode));
}





/******************************************************************************/
/******************************************************************************/
//
//  LuaMatUniforms
//

spring::unsynced_map<LuaMatUniforms::IUniform*, std::string> LuaMatUniforms::GetEngineUniformNamePairs()
{
	return {
		{&viewMatrix,    "ViewMatrix"},
		{&projMatrix,    "ProjectionMatrix"},
		{&viprMatrix,    "ViewProjectionMatrix"},
		{&viewMatrixInv, "ViewMatrixInverse"},
		{&projMatrixInv, "ProjectionMatrixInverse"},
		{&viprMatrixInv, "ViewProjectionMatrixInverse"},
		{&modelMatrix,   "ModelMatrix"},
		{&pieceMatrices, "PieceMatrices"},
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

spring::unsynced_map<std::string, LuaMatUniforms::IUniform*> LuaMatUniforms::GetEngineNameUniformPairs()
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
		{"ModelMatrix",             &modelMatrix},
		{"PieceMatrices",           &pieceMatrices},

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

	for (const auto& p: GetEngineNameUniformPairs()) {
		IUniform* u = p.second;
		ActiveUniform au;

		if (u->IsValid())
			continue;

		std::memset(au.name, 0, sizeof(au.name));
		std::strncpy(au.name, p.first.c_str(), std::min(sizeof(au.name) - 1, p.first.size()));

		{
			// try getting location by CamelCase name, e.g. "ViewMatrix"
			au.name[0] = std::toupper(au.name[0]);

			if (u->SetLoc(glGetUniformLocation(shader->openglID, au.name)))
				continue;
		}
		{
			// try getting location by camelCase name, e.g. "viewMatrix"
			au.name[0] = std::tolower(au.name[0]);

			if (u->SetLoc(glGetUniformLocation(shader->openglID, au.name)))
				continue;
		}

		// assume this uniform is not in the standard-set, mark as invalid for Execute
		u->SetLoc(GL_INVALID_INDEX);
	}
}


void LuaMatUniforms::Validate(LuaMatShader* s)
{
	constexpr const char* fmts[2] = {
		"[LuaMatUniforms::%s] engine shaders prohibit the usage of uniform \"%s\"",
		"[LuaMatUniforms::%s] incorrect uniform-type for \"%s\" at location %d (declared %s, expected %s)",
	};

	if (!s->IsCustomType()) {
		// print warning when uniforms are given for engine shaders
		for (const auto& p: GetEngineUniformNamePairs()) {
			if (!p.first->IsValid())
				continue;

			LOG_L(L_WARNING, fmts[0], __func__, p.second.c_str());
		}

		return;
	}

	const decltype(GetEngineNameUniformPairs())& uniforms = GetEngineNameUniformPairs();
	      decltype(uniforms.end()) uniformsIt;


	ActiveUniform au;

	GLsizei numUniforms = 0;
	GLsizei uniformIndx = 0;

	glGetProgramiv(s->openglID, GL_ACTIVE_UNIFORMS, &numUniforms);

	while (uniformIndx < numUniforms) {
		glGetActiveUniform(s->openglID, uniformIndx++, sizeof(au.name) - 1, nullptr, &au.size, &au.type, au.name);

		// check if active uniform is in the standard-set
		au.name[0] = std::toupper(au.name[0]);
		uniformsIt = uniforms.find(au.name);

		if (uniformsIt == uniforms.end()) {
			au.name[0] = std::tolower(au.name[0]);
			uniformsIt = uniforms.find(au.name);
		}

		if (uniformsIt == uniforms.end())
			continue;


		// if so, check if its type matches the expected type
		const GLint uniformLoc = (uniformsIt->second)->loc;
		const GLenum expectedType = (uniformsIt->second)->GetType();

		if (au.type == expectedType)
			continue;

		LOG_L(L_WARNING, fmts[1], __func__, au.name, uniformLoc,  GLUniformTypeToString(au.type), GLUniformTypeToString(expectedType));
	}
}


void LuaMatUniforms::Parse(lua_State* L, const int tableIdx)
{
	decltype(GetEngineNameUniformPairs()) lcNameUniformPairs;

	for (const auto& p: GetEngineNameUniformPairs()) {
		lcNameUniformPairs[ StringToLower(p.first) ] = p.second;
	}

	for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2) || !lua_isnumber(L, -1))
			continue;

		// convert LuaMaterial key to lower-case and remove the
		// "loc" postfix from it ("cameraPosLoc" -> "camerapos")
		std::string uniformLocStr = StringToLower(luaL_tosstring(L, -2));

		if (StringEndsWith(uniformLocStr, "loc"))
			uniformLocStr.resize(uniformLocStr.size() - 3);

		const auto uniformLocIt = lcNameUniformPairs.find(uniformLocStr);

		if (uniformLocIt == lcNameUniformPairs.end()) {
			LOG_L(L_WARNING, "[LuaMatUniforms::%s] unknown uniform \"%s\"", __func__, uniformLocStr.c_str());
			continue;
		}

		if ((uniformLocIt->second)->SetLoc(static_cast<GLint>(lua_tonumber(L, -1))))
			continue;

		// could log this too, but would be confusing without the shader/material name
		// LOG_L(L_WARNING, "[LuaMatUniforms::%s] unused uniform \"%s\"", __func__, uniformLocStr.c_str());
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

	shadowMatrix.Execute(shadowHandler.GetShadowViewMatrix());
	shadowParams.Execute(shadowHandler.GetShadowParams());

	camPos.Execute(camera->GetPos());
	camDir.Execute(camera->GetDir());
	sunDir.Execute(sky->GetLight()->GetLightDir());
	rndVec.Execute(guRNG.NextVector());

	simFrame.Execute(gs->frameNum);
	visFrame.Execute(globalRendering->drawFrame);
}


void LuaMatUniforms::Print(const string& indent, bool isDeferred) const
{
	LOG("%s[uniforms][%s]", indent.c_str(), (isDeferred? "deferred": "standard"));
	LOG("%s  viewMatrixLoc    = %i", indent.c_str(), viewMatrix.loc);
	LOG("%s  projMatrixLoc    = %i", indent.c_str(), projMatrix.loc);
	LOG("%s  viewMatrixInvLoc = %i", indent.c_str(), viewMatrixInv.loc);
	LOG("%s  projMatrixInvLoc = %i", indent.c_str(), projMatrixInv.loc);

	LOG("%s    modelMatrixLoc = %i", indent.c_str(), modelMatrix.loc);
	LOG("%s  pieceMatricesLoc = %i", indent.c_str(), pieceMatrices.loc);

	LOG("%s  shadowMatrixLoc  = %i", indent.c_str(), shadowMatrix.loc);
	LOG("%s  shadowParamsLoc  = %i", indent.c_str(), shadowParams.loc);

	LOG("%s     teamColorLoc  = %i", indent.c_str(), teamColor.loc);

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
	if ((bin = _bin) == nullptr)
		return;

	bin->Ref();
}

LuaMatRef::~LuaMatRef()
{
	if (bin == nullptr)
		return;

	bin->UnRef();
}

LuaMatRef::LuaMatRef(const LuaMatRef& mr)
{
	if ((bin = mr.bin) == nullptr)
		return;

	bin->Ref();
}


void LuaMatRef::Reset()
{
	if (bin != nullptr)
		bin->UnRef();

	bin = nullptr;
}


LuaMatRef& LuaMatRef::operator=(const LuaMatRef& mr)
{
	if (mr.bin != nullptr) { mr.bin->Ref();   }
	if (   bin != nullptr) {    bin->UnRef(); }
	bin = mr.bin;
	return *this;
}


void LuaMatRef::AddUnit(CSolidObject* o)
{
	if (bin == nullptr)
		return;
	bin->AddUnit(o);
}

void LuaMatRef::AddFeature(CSolidObject* o)
{
	if (bin == nullptr)
		return;
	bin->AddFeature(o);
}



/******************************************************************************/
/******************************************************************************/
//
//  LuaMatBin
//

void LuaMatBin::UnRef()
{
	if ((--refCount) > 0)
		return;

	luaMatHandler.FreeBin(this);
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
	for (unsigned int i = LuaMatShader::LUASHADER_3DO; i < LuaMatShader::LUASHADER_LAST; i++) {
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
		LOG_L(L_WARNING, "[LuaMatHandler::%s] untyped material %d", __func__, mat.type);
		return LuaMatRef();
	}

	LuaMatBin* fakeBin = (LuaMatBin*) &mat;

	LuaMatBinSet& binSet = binTypes[mat.type];
	LuaMatBinSet::iterator it = std::find_if(binSet.begin(), binSet.end(), [&](LuaMatBin* bin) {
		return (!matBinCmp(bin, fakeBin) && !matBinCmp(fakeBin, bin));
	});

	if (it != binSet.end()) {
		assert(*fakeBin == *(*it));
		return (LuaMatRef(*it));
	}

	binSet.push_back(new LuaMatBin(mat));

	LuaMatBin* bin = binSet.back();
	LuaMatRef ref(bin);

	// swap new bin into place, faster than resorting
	for (size_t n = binSet.size() - 1; n > 0; n--) {
		if (matBinCmp(binSet[n - 1], binSet[n]))
			break;

		std::swap(binSet[n - 1], binSet[n]);
	}

	return ref;
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


void LuaMatHandler::FreeBin(LuaMatBin* argBin)
{
	LuaMatBinSet& binSet = binTypes[argBin->type];
	LuaMatBinSet::iterator it = std::find_if(binSet.begin(), binSet.end(), [&](LuaMatBin* bin) {
		return (!matBinCmp(bin, argBin) && !matBinCmp(argBin, bin));
	});

	if (it == binSet.end())
		return;

	assert((*it) == argBin);

	// swap to-be-deleted bin to the back, faster than resorting
	for (size_t n = (it - binSet.begin()); n < (binSet.size() - 1); n++) {
		std::swap(binSet[n], binSet[n + 1]);
	}

	assert(binSet.back() == argBin);
	binSet.pop_back();

	delete argBin;
}


void LuaMatHandler::PrintBins(const string& indent, LuaMatType type) const
{
	if ((type < 0) || (type >= LUAMAT_TYPE_COUNT))
		return;

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
		PrintBins(indent + GetMatTypeName(LuaMatType(m)) + "  ", LuaMatType(m));
	}
}


/******************************************************************************/
/******************************************************************************/

float LuaObjectMaterialData::GLOBAL_LOD_FACTORS[LUAOBJ_LAST] = {1.0f, 1.0f};

