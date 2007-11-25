#include "StdAfx.h"
// LuaShaders.cpp: implementation of the LuaShaders class.
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
#include "Rendering/ShadowHandler.h"


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

LuaShaders::LuaShaders()
{
	Program p;
	p.id = 0;
	programs.push_back(p);
}


LuaShaders::~LuaShaders()
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

GLuint LuaShaders::GetProgramName(unsigned int progID) const
{
	if ((progID <= 0) || (progID >= (int)programs.size())) {
		return 0;
	}
	return programs[progID].id;
}


GLuint LuaShaders::GetProgramName(lua_State* L, int index) const
{
	const int progID = (GLuint)luaL_checknumber(L, 1);
	if ((progID <= 0) || (progID >= (int)programs.size())) {
		return 0;
	}
	return programs[progID].id;
}


unsigned int LuaShaders::AddProgram(const Program& p)
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


void LuaShaders::RemoveProgram(unsigned int progID)
{
	if ((progID <= 0) || (progID >= (int)programs.size())) {
		return;
	}
	Program& p = programs[progID];
	DeleteProgram(p);
	unused.push_back(progID);
	return;
}


void LuaShaders::DeleteProgram(Program& p)
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

int LuaShaders::GetShaderLog(lua_State* L)
{
	const LuaShaders& shaders = CLuaHandle::GetActiveShaders();
	lua_pushstring(L, shaders.errorLog.c_str());
	return 1;
}


/******************************************************************************/
/******************************************************************************/

static int ParseIntArray(lua_State* L, int* array, int size)
{
	if (!lua_istable(L, -1)) {
		return -1;
	}
	const int table = lua_gettop(L);
	for (int i = 0; i < size; i++) {
		lua_rawgeti(L, table, (i + 1));
		if (lua_isnumber(L, -1)) {
			array[i] = (int)lua_tonumber(L, -1);
			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			return i;
		}
	}
	return size;
}


static int ParseFloatArray(lua_State* L, float* array, int size)
{
	if (!lua_istable(L, -1)) {
		return -1;
	}
	const int table = lua_gettop(L);
	for (int i = 0; i < size; i++) {
		lua_rawgeti(L, table, (i + 1));
		if (lua_isnumber(L, -1)) {
			array[i] = (float)lua_tonumber(L, -1);
			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			return i;
		}
	}
	return size;
}


static bool ParseUniformTable(lua_State* L, int index, GLuint progName)
{
	lua_pushstring(L, "uniform");
	lua_gettable(L, index);
	if (lua_istable(L, -1)) {
		const int table = lua_gettop(L);
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2)) {
				const string name = lua_tostring(L, -2);
				const GLint loc = glGetUniformLocation(progName, name.c_str());
				if (loc >= 0) {
					if (lua_israwnumber(L, -1)) {
						const float value = (float)lua_tonumber(L, -1);
						glUniform1f(loc, value);
					}
					else if (lua_istable(L, -1)) {
						float a[4];
						const int count = ParseFloatArray(L, a, 4);
						switch (count) {
							case 1: { glUniform1f(loc, a[0]);                   break; }
							case 2: { glUniform2f(loc, a[0], a[1]);             break; }
							case 3: { glUniform3f(loc, a[0], a[1], a[2]);       break; }
							case 4: { glUniform4f(loc, a[0], a[1], a[2], a[3]); break; }
						}
					}
				}
			}				
		}
	}
	lua_pop(L, 1);
	return true;
}


static bool ParseUniformIntTable(lua_State* L, int index, GLuint progName)
{
	lua_pushstring(L, "uniformInt");
	lua_gettable(L, index);
	if (lua_istable(L, -1)) {
		const int table = lua_gettop(L);
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2)) {
				const string name = lua_tostring(L, -2);
				const GLint loc = glGetUniformLocation(progName, name.c_str());
				if (loc >= 0) {
					if (lua_israwnumber(L, -1)) {
						const int value = (int)lua_tonumber(L, -1);
						glUniform1i(loc, value);
					}
					else if (lua_istable(L, -1)) {
						int a[4];
						const int count = ParseIntArray(L, a, 4);
						switch (count) {
							case 1: { glUniform1i(loc, a[0]);                   break; }
							case 2: { glUniform2i(loc, a[0], a[1]);             break; }
							case 3: { glUniform3i(loc, a[0], a[1], a[2]);       break; }
							case 4: { glUniform4i(loc, a[0], a[1], a[2], a[3]); break; }
						}
					}
				}
			}				
		}
	}
	lua_pop(L, 1);
	return true;
}


static bool ParseUniformMatrixTable(lua_State* L, int index, GLuint progName)
{
	lua_pushstring(L, "uniformMatrix");
	lua_gettable(L, index);
	if (lua_istable(L, -1)) {
		const int table = lua_gettop(L);
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2)) {
				const string name = lua_tostring(L, -2);
				const GLint loc = glGetUniformLocation(progName, name.c_str());
				if (loc >= 0) {
					if (lua_istable(L, -1)) {
						float a[16];
						const int count = ParseFloatArray(L, a, 16);
						switch (count) {
							case (2 * 2): { glUniformMatrix2fv(loc, 1, GL_FALSE, a); break; }
							case (3 * 3): { glUniformMatrix3fv(loc, 1, GL_FALSE, a); break; }
							case (4 * 4): { glUniformMatrix4fv(loc, 1, GL_FALSE, a); break; }
						}
					}
				}
			}				
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

	const bool success = ParseUniformTable(L, index, progName)    &&
	                     ParseUniformIntTable(L, index, progName) &&
	                     ParseUniformMatrixTable(L, index, progName);

	glUseProgram(currentProgram);

	return success;
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
		
		LuaShaders& shaders = CLuaHandle::GetActiveShaders();
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
			if (!lua_israwnumber(L, -2) || !lua_israwstring(L, -1)) {
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


int LuaShaders::CreateShader(lua_State* L)
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

	LuaShaders& shaders = CLuaHandle::GetActiveShaders();

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

	// Allows setting up uniforms when drawing is disabled
	// (much more convenient for sampler uniforms, and static
	//  configuration values)
	ParseUniformSetupTables(L, 1, prog);

	const unsigned int progID = shaders.AddProgram(p);
	lua_pushnumber(L, progID);
	return 1;
}


int LuaShaders::DeleteShader(lua_State* L)
{
	if (lua_isnil(L, 1)) {
		return 0;
	}
	const int progID = (int)luaL_checknumber(L, 1);
	LuaShaders& shaders = CLuaHandle::GetActiveShaders();
	shaders.RemoveProgram(progID);
	return 0;
}


int LuaShaders::UseShader(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int progID = (int)luaL_checknumber(L, 1);
	if (progID == 0) {
		glUseProgram(0);
		lua_pushboolean(L, true);
		return 1;
	}
		
	LuaShaders& shaders = CLuaHandle::GetActiveShaders();
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


int LuaShaders::GetActiveUniforms(lua_State* L)
{
	const LuaShaders& shaders = CLuaHandle::GetActiveShaders();
	const GLuint progName = shaders.GetProgramName(L, 1);
	if (progName == 0) {
		return 0;
	}
	
	GLint uniformCount;
	glGetProgramiv(progName, GL_ACTIVE_UNIFORMS, &uniformCount);

	lua_newtable(L);

	for (GLint i = 0; i < uniformCount; i++) {
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


int LuaShaders::GetUniformLocation(lua_State* L)
{
	const LuaShaders& shaders = CLuaHandle::GetActiveShaders();
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

/******************************************************************************/

int LuaShaders::Uniform(lua_State* L)
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


int LuaShaders::UniformInt(lua_State* L)
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


static void UniformMatrix4dv(GLint location, const double dm[16])
{
	float fm[16];
	for (int i = 0; i < 16; i++) {
		fm[i] = (float)dm[i];
	}
	glUniformMatrix4fv(location, 1, GL_FALSE, fm);
}


int LuaShaders::UniformMatrix(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	const GLuint location = (GLuint)luaL_checknumber(L, 1);

	const int values = lua_gettop(L) - 1;
	switch (values) {
		case 1: {
			if (!lua_isstring(L, 2)) {
				luaL_error(L, "Incorrect arguments to gl.UniformMatrix()");
			}
			const string matName = lua_tostring(L, 2);
			if (matName == "shadow") {
				if (shadowHandler) {
					glUniformMatrix4fv(location, 1, GL_FALSE,
					                   shadowHandler->shadowMatrix.m);
				}
			}
			else if (matName == "camera") {
				UniformMatrix4dv(location, camera->GetModelview());
			}
			else if (matName == "caminv") {
				UniformMatrix4dv(location, camera->modelviewInverse);
			}
			else if (matName == "camprj") {
				UniformMatrix4dv(location, camera->GetProjection());
			}
			else {
				luaL_error(L, "Incorrect arguments to gl.UniformMatrix()");
			}
			break;
		}
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
