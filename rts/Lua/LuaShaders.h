/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_SHADERS_H
#define LUA_SHADERS_H

#include <string>
#include <vector>
#include <unordered_map>

#include "Rendering/GL/myGL.h"


struct lua_State;

class LuaShaders {
	public:
		struct Program;

		static bool PushEntries(lua_State* L);

		LuaShaders();
		~LuaShaders();

		void Clear() {
			programs.clear();
			unused.clear();
		}

		std::string errorLog;

		GLuint GetProgramName(uint32_t progIdx) const;
		const Program* GetProgram(uint32_t progIdx) const;
		      Program* GetProgram(uint32_t progIdx);
	private:
		struct Object {
			Object(GLuint _id, GLenum _type) : id(_id), type(_type) {}
			GLuint id;
			GLenum type;
		};
	public:
		struct ActiveUniform {
			GLint size = 0;
			GLenum type = 0;
		};
		struct ActiveUniformLocation {
			GLint location = -1;
		};
		struct Program {
			Program(GLuint _id) : id(_id) {}

			GLuint id;
			std::vector<Object> objects;
			std::unordered_map<std::string, ActiveUniform> activeUniforms;
			std::unordered_map<std::string, ActiveUniformLocation> activeUniformLocations;
		};
	private:
		std::vector<Program> programs;
		std::vector<uint32_t> unused; // references slots in programs
	private:
		uint32_t AddProgram(const Program& p);
		bool RemoveProgram(uint32_t progIdx);
		GLuint GetProgramName(lua_State* L, int index) const;
		const Program* GetProgram(lua_State* L, int index) const;
		      Program* GetProgram(lua_State* L, int index);
	private:
		// helper
		static bool DeleteProgram(Program& p);
		static GLint GetUniformLocation(Program* p, const char* name);
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

		static int GetEngineUniformBufferDef(lua_State* L);

		static int SetUnitBufferUniforms(lua_State* L);
		static int SetFeatureBufferUniforms(lua_State* L);

		static int SetGeometryShaderParameter(lua_State* L);
		static int SetTesselationShaderParameter(lua_State* L);

		static int GetShaderLog(lua_State* L);

	private:
		inline static Program* activeProgram = nullptr;
		inline static int activeShaderDepth = 0;
};


#endif /* LUA_SHADERS_H */
