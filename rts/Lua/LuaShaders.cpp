/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaShaders.h"

#include "LuaInclude.h"

#include "LuaHashString.h"
#include "LuaHandle.h"
#include "LuaOpenGL.h"
#include "LuaOpenGLUtils.h"
#include "LuaUtils.h"

#include "Game/Camera.h"
#include "Rendering/ShadowHandler.h"
#include "System/Log/ILog.h"
#include "System/Util.h"

#include <string>
#include <vector>
using std::string;
using std::vector;


int LuaShaders::activeShaderDepth = 0;


/******************************************************************************/
/******************************************************************************/

bool LuaShaders::PushEntries(lua_State* L)
{
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(CreateShader);
	REGISTER_LUA_CFUNC(DeleteShader);
	REGISTER_LUA_CFUNC(UseShader);
	REGISTER_LUA_CFUNC(ActiveShader);

	REGISTER_LUA_CFUNC(GetActiveUniforms);
	REGISTER_LUA_CFUNC(GetUniformLocation);
	REGISTER_LUA_CFUNC(Uniform);
	REGISTER_LUA_CFUNC(UniformInt);
	REGISTER_LUA_CFUNC(UniformArray);
	REGISTER_LUA_CFUNC(UniformMatrix);

	REGISTER_LUA_CFUNC(SetShaderParameter);

	REGISTER_LUA_CFUNC(GetShaderLog);

	return true;
}


/******************************************************************************/
/******************************************************************************/

LuaShaders::LuaShaders()
{
	programs.push_back(Program(0));
}


LuaShaders::~LuaShaders()
{
	for (unsigned int p = 0; p < programs.size(); p++) {
		DeleteProgram(programs[p]);
	}

	programs.clear();
}


/******************************************************************************/
/******************************************************************************/

inline void CheckDrawingEnabled(lua_State* L, const char* caller)
{
	if (!LuaOpenGL::IsDrawingEnabled(L)) {
		luaL_error(L, "%s(): OpenGL calls can only be used in Draw() "
		              "call-ins, or while creating display lists", caller);
	}
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
	UNIFORM_TYPE_INT          = 0, // includes arrays
	UNIFORM_TYPE_FLOAT        = 1, // includes arrays
	UNIFORM_TYPE_FLOAT_MATRIX = 2,
};

static void ParseUniformType(lua_State* L, int loc, int type)
{
	switch (type) {
		case UNIFORM_TYPE_FLOAT: {
			if (lua_israwnumber(L, -1)) {
				glUniform1f(loc, lua_tofloat(L, -1));
			}
			else if (lua_istable(L, -1)) {
				float array[32] = {0.0f};
				const int count = LuaUtils::ParseFloatArray(L, -1, array, sizeof(array) / sizeof(float));

				switch (count) {
					case 1: { glUniform1f(loc, array[0]                              ); break; }
					case 2: { glUniform2f(loc, array[0], array[1]                    ); break; }
					case 3: { glUniform3f(loc, array[0], array[1], array[2]          ); break; }
					case 4: { glUniform4f(loc, array[0], array[1], array[2], array[3]); break; }
					default: { glUniform1fv(loc, count, &array[0]); } break;
				}
			}

			return;
		}
		case UNIFORM_TYPE_INT: {
			if (lua_israwnumber(L, -1)) {
				glUniform1i(loc, lua_toint(L, -1));
			}
			else if (lua_istable(L, -1)) {
				int array[32] = {0};
				const int count = LuaUtils::ParseIntArray(L, -1, array, sizeof(array) / sizeof(int));

				switch (count) {
					case 1: { glUniform1i(loc, array[0]                              ); break; }
					case 2: { glUniform2i(loc, array[0], array[1]                    ); break; }
					case 3: { glUniform3i(loc, array[0], array[1], array[2]          ); break; }
					case 4: { glUniform4i(loc, array[0], array[1], array[2], array[3]); break; }
					default: { glUniform1iv(loc, count, &array[0]); } break;
				}
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
			}
		} break;
	}
}

static bool ParseUniformsTable(lua_State* L, const char* fieldName, int index, int type, GLuint progName)
{
	lua_getfield(L, index, fieldName);

	if (lua_istable(L, -1)) {
		const int table = lua_gettop(L);

		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2))
				continue;

			const string name = lua_tostring(L, -2);
			const GLint loc = glGetUniformLocation(progName, name.c_str());

			if (loc < 0)
				continue;

			ParseUniformType(L, loc, type);
		}
	}

	lua_pop(L, 1);
	return true;
}


static bool ParseUniformSetupTables(lua_State* L, int index, GLuint progName)
{
	GLint currentProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

	glUseProgram(progName);

	const bool success =
		ParseUniformsTable(L, "uniform",       index, UNIFORM_TYPE_FLOAT,        progName) &&
		ParseUniformsTable(L, "uniformFloat",  index, UNIFORM_TYPE_FLOAT,        progName) &&
		ParseUniformsTable(L, "uniformInt",    index, UNIFORM_TYPE_INT,          progName) &&
		ParseUniformsTable(L, "uniformMatrix", index, UNIFORM_TYPE_FLOAT_MATRIX, progName);

	glUseProgram(currentProgram);

	return success;
}


/******************************************************************************/
/******************************************************************************/

static GLuint CompileObject(
	lua_State* L,
	const vector<string>& defs,
	const vector<string>& sources,
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

	glShaderSource(obj, text.size(), &text[0], NULL);
	glCompileShader(obj);

	GLint result;
	glGetShaderiv(obj, GL_COMPILE_STATUS, &result);
	GLchar log[4096];
	GLsizei logSize = sizeof(log);
	glGetShaderInfoLog(obj, logSize, &logSize, log);

	LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	shaders.errorLog = log;
	if (result != GL_TRUE) {
		if (shaders.errorLog.empty()) {
			shaders.errorLog = "Empty error message:  code = "
							+ IntToString(result) + " (0x"
							+ IntToString(result, "%04X") + ")";
		}

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

		if (!txt.empty()) {
			data.push_back(txt);
		}

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

			if (!txt.empty()) {
				data.push_back(txt);
			}
		}

		lua_pop(L, 1);
		return true;
	}

	LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	shaders.errorLog = "\"" + string(key) + "\" must be a string or a table value!";

	lua_pop(L, 1);
	return false;
}


static void ApplyGeometryParameters(lua_State* L, int table, GLuint prog)
{
	if (!IS_GL_FUNCTION_AVAILABLE(glProgramParameteriEXT))
		return;

	struct { const char* name; GLenum param; } parameters[] = {
		{ "geoInputType",   GL_GEOMETRY_INPUT_TYPE_EXT },
		{ "geoOutputType",  GL_GEOMETRY_OUTPUT_TYPE_EXT },
		{ "geoOutputVerts", GL_GEOMETRY_VERTICES_OUT_EXT }
	};

	const int count = sizeof(parameters) / sizeof(parameters[0]);
	for (int i = 0; i < count; i++) {
		lua_getfield(L, table, parameters[i].name);
		if (lua_israwnumber(L, -1)) {
			glProgramParameteriEXT(prog, parameters[i].param, /*type*/ lua_toint(L, -1));
		}
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
	std::vector<std::string> geomSrcs;
	std::vector<std::string> fragSrcs;

	ParseShaderTable(L, 1, "defines", shdrDefs);
	ParseShaderTable(L, 1, "definitions", shdrDefs);

	if (!ParseShaderTable(L, 1,   "vertex", vertSrcs))
		return 0;
	if (!ParseShaderTable(L, 1, "geometry", geomSrcs))
		return 0;
	if (!ParseShaderTable(L, 1, "fragment", fragSrcs))
		return 0;

	// tables might have contained empty strings
	if (vertSrcs.empty() && fragSrcs.empty() && geomSrcs.empty())
		return 0;

	bool success;
	const GLuint vertObj = CompileObject(L, shdrDefs, vertSrcs, GL_VERTEX_SHADER, success);

	if (!success)
		return 0;

	const GLuint geomObj = CompileObject(L, shdrDefs, geomSrcs, GL_GEOMETRY_SHADER_EXT, success);

	if (!success) {
		glDeleteShader(vertObj);
		return 0;
	}

	const GLuint fragObj = CompileObject(L, shdrDefs, fragSrcs, GL_FRAGMENT_SHADER, success);

	if (!success) {
		glDeleteShader(vertObj);
		glDeleteShader(geomObj);
		return 0;
	}

	const GLuint prog = glCreateProgram();

	Program p(prog);

	if (vertObj != 0) {
		glAttachShader(prog, vertObj);
		p.objects.push_back(Object(vertObj, GL_VERTEX_SHADER));
	}
	if (geomObj != 0) {
		glAttachShader(prog, geomObj);
		p.objects.push_back(Object(geomObj, GL_GEOMETRY_SHADER_EXT));
		ApplyGeometryParameters(L, 1, prog); // done before linking
	}
	if (fragObj != 0) {
		glAttachShader(prog, fragObj);
		p.objects.push_back(Object(fragObj, GL_FRAGMENT_SHADER));
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
	CheckDrawingEnabled(L, __FUNCTION__);

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
#define UNIFORM_STRING_CASE(x)                   \
  case (GL_ ## x): {                             \
    static const string str = StringToLower(#x); \
    return str.c_str();                          \
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
	  default: {
	  	return "unknown_type";
		}
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
		GLsizei length;
		GLint size;
		GLenum type;
		GLchar name[1024];
		glGetActiveUniform(progName, i, sizeof(name), &length, &size, &type, name);

		lua_newtable(L); {
			HSTR_PUSH_STRING(L, "name",   name);
			HSTR_PUSH_STRING(L, "type",   UniformTypeString(type));
			HSTR_PUSH_NUMBER(L, "length", length);
			HSTR_PUSH_NUMBER(L, "size",   size);
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

	const string name = luaL_checkstring(L, 2);
	const GLint location = glGetUniformLocation(progName, name.c_str());

	lua_pushnumber(L, location);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaShaders::Uniform(lua_State* L)
{
	if (activeShaderDepth <= 0)
		CheckDrawingEnabled(L, __FUNCTION__);

	const GLuint location = (GLuint) luaL_checknumber(L, 1);
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
		CheckDrawingEnabled(L, __FUNCTION__);

	const GLuint location = (GLuint) luaL_checknumber(L, 1);
	const int numValues = lua_gettop(L) - 1;

	switch (numValues) {
		case 1: {
			glUniform1i(location, luaL_checkint(L, 2));
			break;
		}
		case 2: {
			glUniform2i(location, luaL_checkint(L, 2), luaL_checkint(L, 3));
			break;
		}
		case 3: {
			glUniform3i(location, luaL_checkint(L, 2), luaL_checkint(L, 3), luaL_checkint(L, 4));
			break;
		}
		case 4: {
			glUniform4i(location, luaL_checkint(L, 2), luaL_checkint(L, 3), luaL_checkint(L, 4), luaL_checkint(L, 5));
			break;
		}
		default: {
			luaL_error(L, "Incorrect arguments to gl.UniformInt()");
		}
	}

	return 0;
}

int LuaShaders::UniformArray(lua_State* L)
{
	if (activeShaderDepth <= 0)
		CheckDrawingEnabled(L, __FUNCTION__);

	if (!lua_istable(L, 3))
		return 0;

	const GLuint location = (GLuint) luaL_checknumber(L, 1);

	switch (luaL_checkint(L, 2)) {
		case UNIFORM_TYPE_INT: {
			int array[32] = {0};
			const int count = LuaUtils::ParseIntArray(L, 3, array, sizeof(array) / sizeof(int));

			glUniform1iv(location, count, &array[0]);
		} break;

		case UNIFORM_TYPE_FLOAT: {
			float array[32] = {0.0f};
			const int count = LuaUtils::ParseFloatArray(L, 3, array, sizeof(array) / sizeof(float));

			glUniform1fv(location, count, &array[0]);
		} break;

		default: {
		} break;
	}

	return 0;
}

int LuaShaders::UniformMatrix(lua_State* L)
{
	if (activeShaderDepth <= 0)
		CheckDrawingEnabled(L, __FUNCTION__);

	const GLuint location = (GLuint)luaL_checknumber(L, 1);
	const int numValues = lua_gettop(L) - 1;

	switch (numValues) {
		case 1: {
			if (!lua_isstring(L, 2)) {
				luaL_error(L, "Incorrect arguments to gl.UniformMatrix()");
			}

			const string matName = lua_tostring(L, 2);
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


int LuaShaders::SetShaderParameter(lua_State* L)
{
	if (activeShaderDepth <= 0) {
		CheckDrawingEnabled(L, __FUNCTION__);
	}

	const LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);
	const GLuint progName = shaders.GetProgramName(L, 1);
	if (progName == 0) {
		return 0;
	}

	const GLenum param = (GLenum)luaL_checkint(L, 2);
	const GLint  value =  (GLint)luaL_checkint(L, 3);

	if (IS_GL_FUNCTION_AVAILABLE(glProgramParameteriEXT)) {
		glProgramParameteriEXT(progName, param, value);
	}

	return 0;
}


/******************************************************************************/
/******************************************************************************/
