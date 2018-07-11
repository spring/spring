/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_SHADERS_H
#define LUA_SHADERS_H

#include <string>
#include <vector>

#include "Rendering/GL/myGL.h"


struct lua_State;


class LuaShaders {
	public:
		static bool PushEntries(lua_State* L);

		LuaShaders();
		~LuaShaders();

		void Clear() {
			programs.clear();
			unused.clear();
		}

		std::string errorLog;

		GLuint GetProgramName(unsigned int progIdx) const;

	private:
		struct Object {
			Object(GLuint _id, GLenum _type) : id(_id), type(_type) {}
			GLuint id;
			GLenum type;
		};

		struct Program {
			Program(GLuint _id): id(_id) {}

			GLuint id;
			std::vector<Object> objects;
		};

		std::vector<Program> programs;
		std::vector<unsigned int> unused; // references slots in programs

	private:
		unsigned int AddProgram(const Program& p);
		bool RemoveProgram(unsigned int progIdx);
		GLuint GetProgramName(lua_State* L, int index) const;

	private:
		static bool DeleteProgram(Program& p);

	private:
		// the call-outs
		static int CreateShader(lua_State* L);
		static int DeleteShader(lua_State* L);
		static int UseShader(lua_State* L);
		static int ActiveShader(lua_State* L);

		static int GetActiveUniforms(lua_State* L);
		static int GetUniformLocation(lua_State* L);
		static int GetSubroutineIndex(lua_State* L);
		static int Uniform(lua_State* L);
		static int UniformInt(lua_State* L);
		static int UniformArray(lua_State* L);
		static int UniformMatrix(lua_State* L);
		static int UniformSubroutine(lua_State* L);

		static int SetGeometryShaderParameter(lua_State* );
		static int SetTesselationShaderParameter(lua_State* );

		static int GetShaderLog(lua_State* L);

	private:
		static int activeShaderDepth;
};


#endif /* LUA_SHADERS_H */
