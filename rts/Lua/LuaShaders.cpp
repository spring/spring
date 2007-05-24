#include "StdAfx.h"
// LuaShaders.cpp: implementation of the CLuaShaders class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaShaders.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "LuaInclude.h"

#include "LuaHashString.h"
#include "LuaHandle.h"
#include "LuaOpenGL.h"

#include "Game/Camera.h"
#include "Rendering/GL/myGL.h"
#include "System/LogOutput.h"
#include "System/Platform/ConfigHandler.h"


/******************************************************************************/
/******************************************************************************/

bool CLuaShaders::PushEntries(lua_State* L)
{
	if (!GLEW_ARB_shader_objects) {
		return true;
	}

	if (!configHandler.GetInt("LuaShaders", 1)) {
		return true;
	}

#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(CreateShader);
	REGISTER_LUA_CFUNC(DeleteShader);
	REGISTER_LUA_CFUNC(UseShader);

	REGISTER_LUA_CFUNC(GetActiveUniforms);
	REGISTER_LUA_CFUNC(GetUniformLocation);
	REGISTER_LUA_CFUNC(Uniform);
	REGISTER_LUA_CFUNC(UniformInt);
	REGISTER_LUA_CFUNC(UniformMatrix);

	REGISTER_LUA_CFUNC(GetShaderLog);

	return true;
}


/******************************************************************************/
/******************************************************************************/

CLuaShaders::CLuaShaders()
{
	Program p;
	p.id = 0;
	programs.push_back(p);
}


CLuaShaders::~CLuaShaders()
{
	for (int p = 0; p < (int)programs.size(); p++) {
		DeleteProgram(programs[p]);
	}
	programs.clear();
}


/******************************************************************************/
/******************************************************************************/

inline void CheckDrawingEnabled(lua_State* L, const char* caller)
{
	if (!LuaOpenGL::IsDrawingEnabled()) {
		luaL_error(L, "%s(): OpenGL calls can only be used in Draw() "
		              "call-ins, or while creating display lists", caller);
	}
}


/******************************************************************************/
/******************************************************************************/

GLuint CLuaShaders::GetProgramName(unsigned int progID) const
{
	if ((progID <= 0) || (progID >= (int)programs.size())) {
		return 0;
	}
	return programs[progID].id;
}


GLuint CLuaShaders::GetProgramName(lua_State* L, int index) const
{
	const int progID = (GLuint)luaL_checknumber(L, 1);
	if ((progID <= 0) || (progID >= (int)programs.size())) {
		return 0;
	}
	return programs[progID].id;
}


unsigned int CLuaShaders::AddProgram(const Program& p)
{
	if (!unused.empty()) {
		const unsigned int index = unused[unused.size() - 1];
		programs[index] = p;
		unused.pop_back();
		return index;
	}
	programs.push_back(p);
	return (programs.size() - 1);
}


void CLuaShaders::RemoveProgram(unsigned int progID)
{
	if ((progID <= 0) || (progID >= (int)programs.size())) {
		return;
	}
	Program& p = programs[progID];
	DeleteProgram(p);
	unused.push_back(progID);
	return;
}


void CLuaShaders::DeleteProgram(Program& p)
{
	if (p.id == 0) {
		return;
	}

	for (int o = 0; o < (int)p.objects.size(); o++) {
		Object& obj = p.objects[o];
		glDetachShader(p.id, obj.id);
		glDeleteShader(obj.id);
	}
	p.objects.clear();

	glDeleteProgram(p.id);
	p.id = 0;
}


/******************************************************************************/
/******************************************************************************/

int CLuaShaders::GetShaderLog(lua_State* L)
{
	const CLuaShaders& shaders = CLuaHandle::GetActiveShaders();
	lua_pushstring(L, shaders.errorLog.c_str());
	return 1;
}


/******************************************************************************/
/******************************************************************************/

static GLuint CompileObject(const vector<string>& sources, GLenum type,
                            bool& success)
{
	if (sources.empty()) {
		success = true;
		return 0;
	}

	GLuint obj = glCreateShader(type);

	const int count = (int)sources.size();
	
	const GLchar** texts = SAFE_NEW const GLchar*[count];
	for (int i = 0; i < count; i++) {
		texts[i] = sources[i].c_str();
	}
	
	glShaderSource(obj, count, texts, NULL);

	delete[] texts;

	glCompileShader(obj);

	GLint result;
	glGetShaderiv(obj, GL_COMPILE_STATUS, &result);
	if (result != GL_TRUE) {
		GLchar log[4096];
		GLsizei logSize = sizeof(log);
		glGetShaderInfoLog(obj, logSize, &logSize, log);
		
		CLuaShaders& shaders = CLuaHandle::GetActiveShaders();
		shaders.errorLog = log;
		
		glDeleteShader(obj);

		success = false;
		return 0;
	}

	success = true;
	return obj;
}


static bool ParseSources(lua_State* L, int table,
                         const char* type, vector<string>& srcs)
{
	if (!lua_istable(L, 1)) {
		return false;
	}

	lua_pushstring(L, type);
	lua_gettable(L, table);
	
	if (lua_isstring(L, -1)) {
		const string src = lua_tostring(L, -1);
		if (!src.empty()) {
			srcs.push_back(src);
		}
	}
	else if (lua_istable(L, -1)) {
		const int table2 = lua_gettop(L);
		for (lua_pushnil(L); lua_next(L, table2) != 0; lua_pop(L, 1)) {
			if (!lua_isnumber(L, -2) || !lua_isstring(L, -1)) {
				continue;
			}
			const string src = lua_tostring(L, -1);
			if (!src.empty()) {
				srcs.push_back(src);
			}
		}
	}
	else if (!lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return false;
	}

	lua_pop(L, 1);
	return true;
}


int CLuaShaders::CreateShader(lua_State* L)
{
	const int args = lua_gettop(L);
	if ((args != 1) || !lua_istable(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.CreateShader()");
	}

	vector<string> vertSrcs;
	vector<string> fragSrcs;
	if (!ParseSources(L, 1, "vertex",   vertSrcs) ||
	    !ParseSources(L, 1, "fragment", fragSrcs)) {
		return 0;
	}
	if (vertSrcs.empty() && fragSrcs.empty()) {
		return 0;
	}

	bool success;
	const GLuint vertObj = CompileObject(vertSrcs, GL_VERTEX_SHADER, success);
	if (!success) {
		return 0;
	}
	const GLuint fragObj = CompileObject(fragSrcs, GL_FRAGMENT_SHADER, success);
	if (!success) {
		glDeleteShader(vertObj);
		return 0;
	}

	const GLuint prog = glCreateProgram();

	Program p;
	p.id = prog;

	if (vertObj != 0) {
		glAttachShader(prog, vertObj);
		p.objects.push_back(Object(vertObj, GL_VERTEX_SHADER));
	}
	if (fragObj != 0) {
		glAttachShader(prog, fragObj);
		p.objects.push_back(Object(fragObj, GL_FRAGMENT_SHADER));
	}

	glLinkProgram(prog);

	CLuaShaders& shaders = CLuaHandle::GetActiveShaders();

	GLint result;
	glGetProgramiv(prog, GL_LINK_STATUS, &result);
	if (result != GL_TRUE) {
		GLchar log[4096];
		GLsizei logSize = sizeof(log);
		glGetProgramInfoLog(prog, logSize, &logSize, log);
		shaders.errorLog = log;

		DeleteProgram(p);
		return 0;
	}

	const unsigned int progID = shaders.AddProgram(p);
	lua_pushnumber(L, progID);
	return 1;
}


int CLuaShaders::DeleteShader(lua_State* L)
{
	const int progID = (int)luaL_checknumber(L, 1);
	CLuaShaders& shaders = CLuaHandle::GetActiveShaders();
	shaders.RemoveProgram(progID);
	return 0;
}


int CLuaShaders::UseShader(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int progID = luaL_checknumber(L, 1);
	if (progID == 0) {
		glUseProgram(0);
		lua_pushboolean(L, true);
		return 1;
	}
		
	CLuaShaders& shaders = CLuaHandle::GetActiveShaders();
	const GLuint progName = shaders.GetProgramName(progID);
	if (progName == 0) {
		lua_pushboolean(L, false);
	} else {
		glUseProgram(progName);
		lua_pushboolean(L, true);
	}
	return 1;
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


int CLuaShaders::GetActiveUniforms(lua_State* L)
{
	const CLuaShaders& shaders = CLuaHandle::GetActiveShaders();
	const GLuint progName = shaders.GetProgramName(L, 1);
	if (progName == 0) {
		return 0;
	}
	
	GLint uniformCount;
	glGetProgramiv(progName, GL_ACTIVE_UNIFORMS, &uniformCount);

	lua_newtable(L);

	for (GLint i = 0; i < uniformCount; i++) {
		GLsizei bufSize;
		GLsizei length;
		GLint size;
		GLenum type;
		GLchar name[1024];
		glGetActiveUniform(progName, i, sizeof(name), &length, &size, &type, name);

		lua_pushnumber(L, i + 1);
		lua_newtable(L); {
			HSTR_PUSH_STRING(L, "name",   name);
			HSTR_PUSH_STRING(L, "type",   UniformTypeString(type));
			HSTR_PUSH_NUMBER(L, "length", length);
			HSTR_PUSH_NUMBER(L, "size",   size);
		}
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", uniformCount);
	
	return 1;
}


int CLuaShaders::GetUniformLocation(lua_State* L)
{
	const CLuaShaders& shaders = CLuaHandle::GetActiveShaders();
	const GLuint progName = shaders.GetProgramName(L, 1);
	if (progName == 0) {
		return 0;
	}

	const string name = luaL_checkstring(L, 2);
	const GLint location = glGetUniformLocation(progName, name.c_str());
	lua_pushnumber(L, location);
	return 1;
}


/******************************************************************************/

int CLuaShaders::Uniform(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLuint location = (GLuint)luaL_checknumber(L, 1);

	const int values = lua_gettop(L) - 1;
	switch (values) {
		case 1: {
			glUniform1f(location,
									(float)luaL_checknumber(L, 2));
			break;
		}
		case 2: {
			glUniform2f(location,
									(float)luaL_checknumber(L, 2),
									(float)luaL_checknumber(L, 3));
			break;
		}
		case 3: {
			glUniform3f(location,
									(float)luaL_checknumber(L, 2),
									(float)luaL_checknumber(L, 3),
									(float)luaL_checknumber(L, 4));
			break;
		}
		case 4: {
			glUniform4f(location,
									(float)luaL_checknumber(L, 2),
									(float)luaL_checknumber(L, 3),
									(float)luaL_checknumber(L, 4),
									(float)luaL_checknumber(L, 5));
			break;
		}
		default: {
			luaL_error(L, "Incorrect arguments to gl.Uniform()");
		}
	}
	return 0;
}


int CLuaShaders::UniformInt(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLuint location = (GLuint)luaL_checknumber(L, 1);

	const int values = lua_gettop(L) - 1;
	switch (values) {
		case 1: {
			glUniform1i(location,
									(int)luaL_checknumber(L, 2));
			break;
		}
		case 2: {
			glUniform2i(location,
									(int)luaL_checknumber(L, 2),
									(int)luaL_checknumber(L, 3));
			break;
		}
		case 3: {
			glUniform3i(location,
									(int)luaL_checknumber(L, 2),
									(int)luaL_checknumber(L, 3),
									(int)luaL_checknumber(L, 4));
			break;
		}
		case 4: {
			glUniform4i(location,
									(int)luaL_checknumber(L, 2),
									(int)luaL_checknumber(L, 3),
									(int)luaL_checknumber(L, 4),
									(int)luaL_checknumber(L, 5));
			break;
		}
		default: {
			luaL_error(L, "Incorrect arguments to gl.UniformInt()");
		}
	}
	return 0;
}


int CLuaShaders::UniformMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLuint location = (GLuint)luaL_checknumber(L, 1);

	const int values = lua_gettop(L) - 1;
	switch (values) {
		case (2 * 2): {
			float array[2 * 2];
			for (int i = 0; i < (2 * 2); i++) {
				array[i] = (float)luaL_checknumber(L, i + 2);
			}
			glUniformMatrix2fv(location, 1, GL_FALSE, array);
			break;
		}
		case (3 * 3): {
			float array[3 * 3];
			for (int i = 0; i < (3 * 3); i++) {
				array[i] = (float)luaL_checknumber(L, i + 2);
			}
			glUniformMatrix3fv(location, 1, GL_FALSE, array);
			break;
		}
		case (4 * 4): {
			float array[4 * 4];
			for (int i = 0; i < (4 * 4); i++) {
				array[i] = (float)luaL_checknumber(L, i + 2);
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


/******************************************************************************/
/******************************************************************************/
