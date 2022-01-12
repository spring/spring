/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaShaders.h"

#include "LuaInclude.h"
#include "LuaHashString.h"
#include "LuaHandle.h"
#include "LuaOpenGL.h"
#include "LuaOpenGLUtils.h"
#include "LuaUtils.h"

#include "Game/Camera.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"

#include <string>
#include <vector>

struct ActiveUniform {
	GLint size = 0;
	GLenum type = 0;
	GLchar name[512] = {0};
};

int   intUniformArrayBuf[1024] = {0   };
float fltUniformArrayBuf[1024] = {0.0f};

int LuaShaders::activeShaderDepth = 0;


/******************************************************************************/
/******************************************************************************/

bool LuaShaders::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(CreateShader);
	REGISTER_LUA_CFUNC(DeleteShader);
	REGISTER_LUA_CFUNC(UseShader);
	REGISTER_LUA_CFUNC(ActiveShader);

	REGISTER_LUA_CFUNC(GetActiveUniforms);
	REGISTER_LUA_CFUNC(GetUniformLocation);
	REGISTER_LUA_CFUNC(GetSubroutineIndex);
	REGISTER_LUA_CFUNC(Uniform);
	REGISTER_LUA_CFUNC(UniformInt);
	REGISTER_LUA_CFUNC(UniformArray);
	REGISTER_LUA_CFUNC(UniformMatrix);
	REGISTER_LUA_CFUNC(UniformSubroutine);

	REGISTER_LUA_CFUNC(SetGeometryShaderParameter);
	REGISTER_LUA_CFUNC(SetTesselationShaderParameter);

	REGISTER_LUA_CFUNC(GetShaderLog);

	return true;
}


/******************************************************************************/
/******************************************************************************/

LuaShaders::LuaShaders()
{
	programs.emplace_back(0);
}


LuaShaders::~LuaShaders()
{
	for (auto& program: programs) {
		DeleteProgram(program);
	}

	programs.clear();
}


/******************************************************************************/
/******************************************************************************/

inline void CheckDrawingEnabled(lua_State* L, const char* caller)
{
	if (LuaOpenGL::IsDrawingEnabled(L))
		return;

	luaL_error(L, "%s(): OpenGL calls can only be used in Draw() " "call-ins, or while creating display lists", caller);
}


/******************************************************************************/
/******************************************************************************/

GLuint LuaShaders::GetProgramName(unsigned int progIdx) const
{
	if (progIdx < programs.size())
		return programs[progIdx].id;

	return 0;
}

GLuint LuaShaders::GetProgramName(lua_State* L, int index) const
{
	if (luaL_checkint(L, index) <= 0)
		return 0;

	return (GetProgramName(luaL_checkint(L, index)));
}


unsigned int LuaShaders::AddProgram(const Program& p)
{
	if (!unused.empty()) {
		const unsigned int index = unused.back();
		programs[index] = p;
		unused.pop_back();
		return index;
	}

	programs.push_back(p);
	return (programs.size() - 1);
}


bool LuaShaders::RemoveProgram(unsigned int progIdx)
{
	if (progIdx >= programs.size())
		return false;
	if (!DeleteProgram(programs[progIdx]))
		return false;

	unused.push_back(progIdx);
	return true;
}


bool LuaShaders::DeleteProgram(Program& p)
{
	if (p.id == 0)
		return false;

	for (unsigned int o = 0; o < p.objects.size(); o++) {
		Object& obj = p.objects[o];
		glDetachShader(p.id, obj.id);
		glDeleteShader(obj.id);
	}

	glDeleteProgram(p.id);

	p.objects.clear();
	p.id = 0;
	return true;
}


/******************************************************************************/
/******************************************************************************/

int LuaShaders::GetShaderLog(lua_State* L)
{
	const LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	lua_pushsstring(L, shaders.errorLog);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

enum {
	UNIFORM_TYPE_MIXED        = 0, // includes arrays; float or int
	UNIFORM_TYPE_INT          = 1, // includes arrays
	UNIFORM_TYPE_FLOAT        = 2, // includes arrays
	UNIFORM_TYPE_FLOAT_MATRIX = 3,
};

static void ParseUniformType(lua_State* L, int loc, int type)
{
	switch (type) {
		case UNIFORM_TYPE_FLOAT: {
			if (lua_israwnumber(L, -1)) {
				glUniform1f(loc, lua_tofloat(L, -1));
				return;
			}
			if (lua_istable(L, -1)) {
				const float* array = fltUniformArrayBuf;
				const int count = LuaUtils::ParseFloatArray(L, -1, fltUniformArrayBuf, sizeof(fltUniformArrayBuf) / sizeof(float));

				switch (count) {
					case 1: { glUniform1f(loc, array[0]                              ); break; }
					case 2: { glUniform2f(loc, array[0], array[1]                    ); break; }
					case 3: { glUniform3f(loc, array[0], array[1], array[2]          ); break; }
					case 4: { glUniform4f(loc, array[0], array[1], array[2], array[3]); break; }
					default: { glUniform1fv(loc, count, &array[0]); } break;
				}

				return;
			}
		} break;

		case UNIFORM_TYPE_INT: {
			if (lua_israwnumber(L, -1)) {
				glUniform1i(loc, lua_toint(L, -1));
				return;
			}
			if (lua_istable(L, -1)) {
				const int* array = intUniformArrayBuf;
				const int count = LuaUtils::ParseIntArray(L, -1, intUniformArrayBuf, sizeof(intUniformArrayBuf) / sizeof(int));

				switch (count) {
					case 1: { glUniform1i(loc, array[0]                              ); break; }
					case 2: { glUniform2i(loc, array[0], array[1]                    ); break; }
					case 3: { glUniform3i(loc, array[0], array[1], array[2]          ); break; }
					case 4: { glUniform4i(loc, array[0], array[1], array[2], array[3]); break; }
					default: { glUniform1iv(loc, count, &array[0]); } break;
				}

				return;
			}
		} break;

		case UNIFORM_TYPE_FLOAT_MATRIX: {
			if (lua_istable(L, -1)) {
				float array[16] = {0.0f};
				const int count = LuaUtils::ParseFloatArray(L, -1, array, 16);

				switch (count) {
					case (2 * 2): { glUniformMatrix2fv(loc, 1, GL_FALSE, array); break; }
					case (3 * 3): { glUniformMatrix3fv(loc, 1, GL_FALSE, array); break; }
					case (4 * 4): { glUniformMatrix4fv(loc, 1, GL_FALSE, array); break; }
				}

				return;
			}
		} break;
	}
}

static bool ParseUniformsTable(
	lua_State* L,
	const std::vector<ActiveUniform>& activeUniforms,
	const std::function<bool(const ActiveUniform&, const ActiveUniform&)>& uniformPred,
	int index,
	int type,
	GLuint progName
) {
	constexpr const char* fieldNames[] = {"uniform", "uniformInt", "uniformFloat", "uniformMatrix"};
	          const char* fieldName    = fieldNames[type];

	ActiveUniform tmpUniform;

	lua_getfield(L, index, fieldName);

	if (lua_istable(L, -1)) {
		const int tableIdx = lua_gettop(L);

		for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2))
				continue;

			const char* uniformName = lua_tostring(L, -2);
			const GLint uniformLoc = glGetUniformLocation(progName, uniformName);

			if (uniformLoc < 0)
				continue;

			std::memset(tmpUniform.name, 0, sizeof(tmpUniform.name));
			std::strncpy(tmpUniform.name, uniformName, std::min(sizeof(tmpUniform.name) - 1, std::strlen(uniformName)));

			const auto iter = std::lower_bound(activeUniforms.begin(), activeUniforms.end(), tmpUniform, uniformPred);

			if (iter == activeUniforms.end() || std::strcmp(iter->name, uniformName) != 0) {
				LOG_L(L_WARNING, "[%s] uniform \"%s\" from table \"%s\" not active in shader", __func__, uniformName, fieldName);
				continue;
			}

			// should only need to auto-correct if type == UNIFORM_TYPE_MIXED, but GL debug-errors say otherwise
			switch (iter->type) {
				case GL_SAMPLER_1D            : { type = UNIFORM_TYPE_INT; } break;
				case GL_SAMPLER_2D            : { type = UNIFORM_TYPE_INT; } break;
				case GL_SAMPLER_3D            : { type = UNIFORM_TYPE_INT; } break;
				case GL_SAMPLER_1D_SHADOW     : { type = UNIFORM_TYPE_INT; } break;
				case GL_SAMPLER_2D_SHADOW     : { type = UNIFORM_TYPE_INT; } break;
				case GL_SAMPLER_CUBE          : { type = UNIFORM_TYPE_INT; } break;
				case GL_SAMPLER_2D_MULTISAMPLE: { type = UNIFORM_TYPE_INT; } break;

				case GL_INT     : { type = UNIFORM_TYPE_INT; } break;
				case GL_INT_VEC2: { type = UNIFORM_TYPE_INT; } break;
				case GL_INT_VEC3: { type = UNIFORM_TYPE_INT; } break;
				case GL_INT_VEC4: { type = UNIFORM_TYPE_INT; } break;

				case GL_UNSIGNED_INT     : { type = UNIFORM_TYPE_INT; } break;
				case GL_UNSIGNED_INT_VEC2: { type = UNIFORM_TYPE_INT; } break;
				case GL_UNSIGNED_INT_VEC3: { type = UNIFORM_TYPE_INT; } break;
				case GL_UNSIGNED_INT_VEC4: { type = UNIFORM_TYPE_INT; } break;

				case GL_FLOAT     : { type = UNIFORM_TYPE_FLOAT; } break;
				case GL_FLOAT_VEC2: { type = UNIFORM_TYPE_FLOAT; } break;
				case GL_FLOAT_VEC3: { type = UNIFORM_TYPE_FLOAT; } break;
				case GL_FLOAT_VEC4: { type = UNIFORM_TYPE_FLOAT; } break;

				case GL_FLOAT_MAT2: { type = UNIFORM_TYPE_FLOAT_MATRIX; } break;
				case GL_FLOAT_MAT3: { type = UNIFORM_TYPE_FLOAT_MATRIX; } break;
				case GL_FLOAT_MAT4: { type = UNIFORM_TYPE_FLOAT_MATRIX; } break;

				default: {
					LOG_L(L_WARNING, "[%s] value for uniform \"%s\" from table \"%s\" (GL-type 0x%x) set as int", __func__, uniformName, fieldName, iter->type);
					type = UNIFORM_TYPE_INT;
				} break;
			}

			ParseUniformType(L, uniformLoc, type);
		}
	}

	lua_pop(L, 1);
	return true;
}


static bool ParseUniformSetupTables(lua_State* L, int index, GLuint progName)
{
	bool ret = true;

	GLint currentProgram = 0;
	GLint numUniforms = 0;
	GLsizei uniformLen = 0;

	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
	glUseProgram(progName);
	glGetProgramiv(progName, GL_ACTIVE_UNIFORMS, &numUniforms);


	std::vector<ActiveUniform> uniforms;
	std::function<bool(const ActiveUniform&, const ActiveUniform&)> uniformPred = [](const ActiveUniform& a, const ActiveUniform& b) {
		return (std::strcmp(a.name, b.name) < 0);
	};

	if (numUniforms > 0) {
		uniforms.resize(numUniforms);

		for (ActiveUniform& u: uniforms) {
			glGetActiveUniform(progName, &u - &uniforms[0], sizeof(ActiveUniform::name) - 1, &uniformLen, &u.size, &u.type, &u.name[0]);

			if (u.name[uniformLen - 1] != ']')
				continue;

			// strip "[0]" postfixes from array-uniform names
			u.name[uniformLen - 3] = 0;
			u.name[uniformLen - 2] = 0;
			u.name[uniformLen - 1] = 0;
		}

		std::sort(uniforms.begin(), uniforms.end(), uniformPred);
	}

	ret = ret && ParseUniformsTable(L, uniforms, uniformPred, index, UNIFORM_TYPE_MIXED,        progName);
	ret = ret && ParseUniformsTable(L, uniforms, uniformPred, index, UNIFORM_TYPE_INT,          progName);
	ret = ret && ParseUniformsTable(L, uniforms, uniformPred, index, UNIFORM_TYPE_FLOAT,        progName);
	ret = ret && ParseUniformsTable(L, uniforms, uniformPred, index, UNIFORM_TYPE_FLOAT_MATRIX, progName);

	glUseProgram(currentProgram);
	return ret;
}


/******************************************************************************/
/******************************************************************************/

static GLuint CompileObject(
	lua_State* L,
	const std::vector<std::string>& defs,
	const std::vector<std::string>& sources,
	const GLenum type,
	bool& success
) {
	if (sources.empty()) {
		success = true;
		return 0;
	}

	GLuint obj = glCreateShader(type);
	if (obj == 0) {
		LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
		shaders.errorLog = "Could not create shader object";
		return 0;
	}

	std::vector<const GLchar*> text(defs.size() + sources.size());

	for (unsigned int i = 0; i < defs.size(); i++)
		text[i] = defs[i].c_str();
	for (unsigned int i = 0; i < sources.size(); i++)
		text[defs.size() + i] = sources[i].c_str();

	glShaderSource(obj, text.size(), &text[0], nullptr);
	glCompileShader(obj);

	GLint result;
	glGetShaderiv(obj, GL_COMPILE_STATUS, &result);
	GLchar log[4096];
	GLsizei logSize = sizeof(log);
	glGetShaderInfoLog(obj, logSize, &logSize, log);

	LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	shaders.errorLog = log;

	if (result != GL_TRUE) {
		if (shaders.errorLog.empty())
			shaders.errorLog = "Empty error message:  code = " + IntToString(result) + " (0x" + IntToString(result, "%04X") + ")";

		glDeleteShader(obj);

		success = false;
		return 0;
	}

	success = true;
	return obj;
}


static bool ParseShaderTable(
	lua_State* L,
	const int table,
	const char* key,
	std::vector<std::string>& data
) {
	lua_getfield(L, table, key);

	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return true;
	}

	if (lua_israwstring(L, -1)) {
		const std::string& txt = lua_tostring(L, -1);

		if (!txt.empty())
			data.push_back(txt);

		lua_pop(L, 1);
		return true;
	}

	if (lua_istable(L, -1)) {
		const int subtable = lua_gettop(L);

		for (lua_pushnil(L); lua_next(L, subtable) != 0; lua_pop(L, 1)) {
			if (!lua_israwnumber(L, -2)) // key (idx)
				continue;
			if (!lua_israwstring(L, -1)) // val
				continue;

			const std::string& txt = lua_tostring(L, -1);

			if (!txt.empty())
				data.push_back(txt);
		}

		lua_pop(L, 1);
		return true;
	}

	LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	shaders.errorLog = "\"" + std::string(key) + "\" must be a string or a table value!";

	lua_pop(L, 1);
	return false;
}


static void ApplyGeometryParameters(lua_State* L, int table, GLuint prog)
{
	constexpr struct { const char* name; GLenum param; } parameters[] = {
		{ "geoInputType",   GL_GEOMETRY_INPUT_TYPE },
		{ "geoOutputType",  GL_GEOMETRY_OUTPUT_TYPE },
		{ "geoOutputVerts", GL_GEOMETRY_VERTICES_OUT }
	};

	for (const auto& param: parameters) {
		lua_getfield(L, table, param.name);

		if (lua_israwnumber(L, -1))
			glProgramParameteri(prog, param.param, /*type*/ lua_toint(L, -1));

		lua_pop(L, 1);
	}
}


int LuaShaders::CreateShader(lua_State* L)
{
	const int args = lua_gettop(L);

	if ((args != 1) || !lua_istable(L, 1))
		luaL_error(L, "Incorrect arguments to gl.CreateShader()");

	std::vector<std::string> shdrDefs;
	std::vector<std::string> vertSrcs;
	std::vector<std::string> tcsSrcs;
	std::vector<std::string> tesSrcs;
	std::vector<std::string> geomSrcs;
	std::vector<std::string> fragSrcs;

	ParseShaderTable(L, 1, "defines", shdrDefs);
	ParseShaderTable(L, 1, "definitions", shdrDefs);

	if (!ParseShaderTable(L, 1,   "vertex", vertSrcs))
		return 0;
	if (!ParseShaderTable(L, 1,      "tcs",  tcsSrcs))
		return 0;
	if (!ParseShaderTable(L, 1,      "tes",  tesSrcs))
		return 0;
	if (!ParseShaderTable(L, 1, "geometry", geomSrcs))
		return 0;
	if (!ParseShaderTable(L, 1, "fragment", fragSrcs))
		return 0;

	// tables might have contained empty strings
	if (vertSrcs.empty() && fragSrcs.empty() && geomSrcs.empty() && tcsSrcs.empty() && tesSrcs.empty())
		return 0;

	bool success;
	const GLuint vertObj = CompileObject(L, shdrDefs, vertSrcs, GL_VERTEX_SHADER, success);

	if (!success)
		return 0;

	const GLuint tcsObj = CompileObject(L, shdrDefs, tcsSrcs, GL_TESS_CONTROL_SHADER, success);

	if (!success) {
		glDeleteShader(vertObj);
		return 0;
	}

	const GLuint tesObj = CompileObject(L, shdrDefs, tesSrcs,  GL_TESS_EVALUATION_SHADER, success);

	if (!success) {
		glDeleteShader(vertObj);
		glDeleteShader(tcsObj);
		return 0;
	}
	const GLuint geomObj = CompileObject(L, shdrDefs, geomSrcs, GL_GEOMETRY_SHADER, success);

	if (!success) {
		glDeleteShader(vertObj);
		glDeleteShader(tcsObj);
		glDeleteShader(tesObj);
		return 0;
	}

	const GLuint fragObj = CompileObject(L, shdrDefs, fragSrcs, GL_FRAGMENT_SHADER, success);

	if (!success) {
		glDeleteShader(vertObj);
		glDeleteShader(tcsObj);
		glDeleteShader(tesObj);
		glDeleteShader(geomObj);
		return 0;
	}

	const GLuint prog = glCreateProgram();

	Program p(prog);

	if (vertObj != 0) {
		glAttachShader(prog, vertObj);
		p.objects.emplace_back(vertObj, GL_VERTEX_SHADER);
	}

	if (tcsObj != 0) {
		glAttachShader(prog, tcsObj);
		p.objects.emplace_back(tcsObj, GL_TESS_CONTROL_SHADER);
	}
	if (tesObj != 0) {
		glAttachShader(prog, tesObj);
		p.objects.emplace_back(tesObj, GL_TESS_EVALUATION_SHADER);
	}

	if (geomObj != 0) {
		glAttachShader(prog, geomObj);
		p.objects.emplace_back(geomObj, GL_GEOMETRY_SHADER);
		ApplyGeometryParameters(L, 1, prog); // done before linking
	}
	if (fragObj != 0) {
		glAttachShader(prog, fragObj);
		p.objects.emplace_back(fragObj, GL_FRAGMENT_SHADER);
	}

	GLint linkStatus;
	GLint validStatus;

	glLinkProgram(prog);
	glGetProgramiv(prog, GL_LINK_STATUS, &linkStatus);

	// Allows setting up uniforms when drawing is disabled
	// (much more convenient for sampler uniforms, and static
	//  configuration values)
	// needs to be called before validation
	ParseUniformSetupTables(L, 1, prog);

	glValidateProgram(prog);
	glGetProgramiv(prog, GL_VALIDATE_STATUS, &validStatus);

	LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);

	if (linkStatus != GL_TRUE || validStatus != GL_TRUE) {
		GLchar log[4096];
		GLsizei logSize = sizeof(log);
		glGetProgramInfoLog(prog, logSize, &logSize, log);
		shaders.errorLog = log;

		DeleteProgram(p);
		return 0;
	}

	// note: index, not raw ID
	lua_pushnumber(L, shaders.AddProgram(p));
	return 1;
}


int LuaShaders::DeleteShader(lua_State* L)
{
	if (lua_isnil(L, 1))
		return 0;

	LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);

	lua_pushboolean(L, shaders.RemoveProgram(luaL_checkint(L, 1)));
	return 1;
}


int LuaShaders::UseShader(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	const int progIdx = luaL_checkint(L, 1);
	if (progIdx == 0) {
		glUseProgram(0);
		lua_pushboolean(L, true);
		return 1;
	}

	LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	const GLuint progName = shaders.GetProgramName(progIdx);
	if (progName == 0) {
		lua_pushboolean(L, false);
	} else {
		glUseProgram(progName);
		lua_pushboolean(L, true);
	}
	return 1;
}


int LuaShaders::ActiveShader(lua_State* L)
{
	const int progIdx = luaL_checkint(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	GLuint progName = 0;

	if (progIdx != 0) {
		LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);

		if ((progName = shaders.GetProgramName(progIdx)) == 0) {
			return 0;
		}
	}

	GLint currentProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

	glUseProgram(progName);
	activeShaderDepth++;
	const int error = lua_pcall(L, lua_gettop(L) - 2, 0, 0);
	activeShaderDepth--;
	glUseProgram(currentProgram);

	if (error != 0) {
		LOG_L(L_ERROR, "gl.ActiveShader: error(%i) = %s", error, lua_tostring(L, -1));
		lua_error(L);
	}

	return 0;
}


/******************************************************************************/
/******************************************************************************/

static const char* UniformTypeString(GLenum type)
{
#define UNIFORM_STRING_CASE(x)                        \
  case (GL_ ## x): {                                  \
    static const std::string str = StringToLower(#x); \
    return str.c_str();                               \
  }

	switch (type) {
		UNIFORM_STRING_CASE(FLOAT)
		UNIFORM_STRING_CASE(FLOAT_VEC2)
		UNIFORM_STRING_CASE(FLOAT_VEC3)
		UNIFORM_STRING_CASE(FLOAT_VEC4)
		UNIFORM_STRING_CASE(FLOAT_MAT2)
		UNIFORM_STRING_CASE(FLOAT_MAT3)
		UNIFORM_STRING_CASE(FLOAT_MAT4)
		UNIFORM_STRING_CASE(SAMPLER_1D)
		UNIFORM_STRING_CASE(SAMPLER_2D)
		UNIFORM_STRING_CASE(SAMPLER_3D)
		UNIFORM_STRING_CASE(SAMPLER_CUBE)
		UNIFORM_STRING_CASE(SAMPLER_1D_SHADOW)
		UNIFORM_STRING_CASE(SAMPLER_2D_SHADOW)
		UNIFORM_STRING_CASE(INT)
		UNIFORM_STRING_CASE(INT_VEC2)
		UNIFORM_STRING_CASE(INT_VEC3)
		UNIFORM_STRING_CASE(INT_VEC4)
		UNIFORM_STRING_CASE(BOOL)
		UNIFORM_STRING_CASE(BOOL_VEC2)
		UNIFORM_STRING_CASE(BOOL_VEC3)
		UNIFORM_STRING_CASE(BOOL_VEC4)
		default: { return "unknown_type"; }
	}
}


int LuaShaders::GetActiveUniforms(lua_State* L)
{
	const LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	const GLuint progName = shaders.GetProgramName(L, 1);

	if (progName == 0)
		return 0;

	GLint uniformCount;
	glGetProgramiv(progName, GL_ACTIVE_UNIFORMS, &uniformCount);

	lua_newtable(L);

	for (GLint i = 0; i < uniformCount; i++) {
		ActiveUniform au;
		GLsizei length;

		glGetActiveUniform(progName, i, sizeof(au.name), &length, &au.size, &au.type, au.name);

		lua_newtable(L); {
			HSTR_PUSH_STRING(L, "name",   au.name);
			HSTR_PUSH_STRING(L, "type",   UniformTypeString(au.type));
			HSTR_PUSH_NUMBER(L, "length", length);
			HSTR_PUSH_NUMBER(L, "size",   au.size);
		}
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}


int LuaShaders::GetUniformLocation(lua_State* L)
{
	const LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	const GLuint progName = shaders.GetProgramName(L, 1);

	if (progName == 0)
		return 0;

	const char* name = luaL_checkstring(L, 2);
	const GLint location = glGetUniformLocation(progName, name);

	lua_pushnumber(L, location);
	return 1;
}

int LuaShaders::GetSubroutineIndex(lua_State* L)
{
	const LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	const GLuint progName = shaders.GetProgramName(L, 1);

	if (progName == 0)
		return 0;

	const GLenum shaderType = (GLenum)luaL_checkint(L, 2);

	const char* name = luaL_checkstring(L, 3);
	const GLint location = glGetSubroutineIndex(progName, shaderType, name);

	lua_pushnumber(L, location);
	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaShaders::Uniform(lua_State* L)
{
	if (activeShaderDepth <= 0)
		CheckDrawingEnabled(L, __func__);

	const GLuint location = luaL_checkint(L, 1);
	const int numValues = lua_gettop(L) - 1;

	switch (numValues) {
		case 1: {
			glUniform1f(location, luaL_checkfloat(L, 2));
		} break;
		case 2: {
			glUniform2f(location, luaL_checkfloat(L, 2), luaL_checkfloat(L, 3));
		} break;
		case 3: {
			glUniform3f(location, luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4));
		} break;
		case 4: {
			glUniform4f(location, luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4), luaL_checkfloat(L, 5));
		} break;
		default: {
			luaL_error(L, "Incorrect arguments to gl.Uniform()");
		} break;
	}

	return 0;
}


int LuaShaders::UniformInt(lua_State* L)
{
	if (activeShaderDepth <= 0)
		CheckDrawingEnabled(L, __func__);

	const GLuint location = luaL_checkint(L, 1);
	const int numValues = lua_gettop(L) - 1;

	switch (numValues) {
		case 1: {
			glUniform1i(location, luaL_checkint(L, 2));
		} break;
		case 2: {
			glUniform2i(location, luaL_checkint(L, 2), luaL_checkint(L, 3));
		} break;
		case 3: {
			glUniform3i(location, luaL_checkint(L, 2), luaL_checkint(L, 3), luaL_checkint(L, 4));
		} break;
		case 4: {
			glUniform4i(location, luaL_checkint(L, 2), luaL_checkint(L, 3), luaL_checkint(L, 4), luaL_checkint(L, 5));
		} break;
		default: {
			luaL_error(L, "Incorrect arguments to gl.UniformInt()");
		}
	}

	return 0;
}


#if 0
template<typename type, typename glUniformFunc, typename ParseArrayFunc>
static bool GLUniformArray(lua_State* L, UniformFunc uf, ParseArrayFunc pf)
{
	const GLuint loc = luaL_checkint(L, 1);

	switch (next_power_of_2(luaL_optint(L, 4, 32))) {
		case (1 <<  3): { type array[1 <<  3] = {type(0)}; uf(loc, pf(L, 3, array, 1 <<  3), &array[0]); return true; } break;
		case (1 <<  4): { type array[1 <<  4] = {type(0)}; uf(loc, pf(L, 3, array, 1 <<  4), &array[0]); return true; } break;
		case (1 <<  5): { type array[1 <<  5] = {type(0)}; uf(loc, pf(L, 3, array, 1 <<  5), &array[0]); return true; } break;
		case (1 <<  6): { type array[1 <<  6] = {type(0)}; uf(loc, pf(L, 3, array, 1 <<  6), &array[0]); return true; } break;
		case (1 <<  7): { type array[1 <<  7] = {type(0)}; uf(loc, pf(L, 3, array, 1 <<  7), &array[0]); return true; } break;
		case (1 <<  8): { type array[1 <<  8] = {type(0)}; uf(loc, pf(L, 3, array, 1 <<  8), &array[0]); return true; } break;
		case (1 <<  9): { type array[1 <<  9] = {type(0)}; uf(loc, pf(L, 3, array, 1 <<  9), &array[0]); return true; } break;
		case (1 << 10): { type array[1 << 10] = {type(0)}; uf(loc, pf(L, 3, array, 1 << 10), &array[0]); return true; } break;
		default: {} break;
	}

	return false;
}
#endif

int LuaShaders::UniformArray(lua_State* L)
{
	if (activeShaderDepth <= 0)
		CheckDrawingEnabled(L, __func__);

	if (!lua_istable(L, 3))
		return 0;

	switch (luaL_checkint(L, 2)) {
		case UNIFORM_TYPE_INT: {
			#if 0
			GLUniformArray<int>(L, glUniform1iv, LuaUtils::ParseIntArray);
			#else
			const int loc = luaL_checkint(L, 1);
			const int cnt = LuaUtils::ParseIntArray(L, 3, intUniformArrayBuf, sizeof(intUniformArrayBuf) / sizeof(int));

			glUniform1iv(loc, cnt, &intUniformArrayBuf[0]);
			#endif
		} break;

		case UNIFORM_TYPE_FLOAT: {
			#if 0
			GLUniformArray<float>(L, glUniform1fv, LuaUtils::ParseFloatArray);
			#else
			const int loc = luaL_checkint(L, 1);
			const int cnt = LuaUtils::ParseFloatArray(L, 3, fltUniformArrayBuf, sizeof(fltUniformArrayBuf) / sizeof(float));

			glUniform1fv(loc, cnt, &fltUniformArrayBuf[0]);
			#endif
		} break;

		default: {
		} break;
	}

	return 0;
}

int LuaShaders::UniformMatrix(lua_State* L)
{
	if (activeShaderDepth <= 0)
		CheckDrawingEnabled(L, __func__);

	const GLuint location = (GLuint)luaL_checknumber(L, 1);
	const int numValues = lua_gettop(L) - 1;

	switch (numValues) {
		case 1: {
			if (!lua_isstring(L, 2))
				luaL_error(L, "Incorrect arguments to gl.UniformMatrix()");

			const char* matName = lua_tostring(L, 2);
			const CMatrix44f* mat = LuaOpenGLUtils::GetNamedMatrix(matName);

			if (mat) {
				glUniformMatrix4fv(location, 1, GL_FALSE, *mat);
			} else {
				luaL_error(L, "Incorrect arguments to gl.UniformMatrix()");
			}
			break;
		}
		case (2 * 2): {
			float array[2 * 2];

			for (int i = 0; i < (2 * 2); i++) {
				array[i] = luaL_checkfloat(L, i + 2);
			}

			glUniformMatrix2fv(location, 1, GL_FALSE, array);
			break;
		}
		case (3 * 3): {
			float array[3 * 3];

			for (int i = 0; i < (3 * 3); i++) {
				array[i] = luaL_checkfloat(L, i + 2);
			}

			glUniformMatrix3fv(location, 1, GL_FALSE, array);
			break;
		}
		case (4 * 4): {
			float array[4 * 4];

			for (int i = 0; i < (4 * 4); i++) {
				array[i] = luaL_checkfloat(L, i + 2);
			}

			glUniformMatrix4fv(location, 1, GL_FALSE, array);
			break;
		}
		default: {
			luaL_error(L, "Incorrect arguments to gl.UniformMatrix()");
		}
	}

	return 0;
}

int LuaShaders::UniformSubroutine(lua_State* L)
{
	if (activeShaderDepth <= 0)
		CheckDrawingEnabled(L, __func__);

	const GLenum shaderType = (GLenum)luaL_checkint(L, 1);
	const GLuint index = (GLuint)luaL_checknumber(L, 2);
	glUniformSubroutinesuiv(shaderType, 1, &index); //this supports array and even array of arrays, but let's keep it simple
	return 0;
}

int LuaShaders::SetGeometryShaderParameter(lua_State* L)
{
	if (activeShaderDepth <= 0)
		CheckDrawingEnabled(L, __func__);

	const LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	const GLuint progName = shaders.GetProgramName(L, 1);
	if (progName == 0)
		return 0;

	const GLenum param = (GLenum)luaL_checkint(L, 2);
	const GLint  value =  (GLint)luaL_checkint(L, 3);

	glProgramParameteri(progName, param, value);
	return 0;
}

int LuaShaders::SetTesselationShaderParameter(lua_State* L)
{
	const GLenum param = (GLenum)luaL_checkint(L, 1);

	if (lua_israwnumber(L, 2)) {
		const GLint value =  (GLint)luaL_checkint(L, 2);
		glPatchParameteri(param, value);
		return 0;
	}
	if (lua_istable(L, 2)) {
		float tessArrayBuf[4] = {0.0f};
		LuaUtils::ParseFloatArray(L, 2, tessArrayBuf, 4);
		glPatchParameterfv(param, &tessArrayBuf[0]);
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/
