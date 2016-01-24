/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_SHADERHANDLER_HDR
#define SPRING_SHADERHANDLER_HDR

#include <string>
#include <unordered_map>
#include "Rendering/GL/myGL.h" //GLuint

namespace Shader {
	struct IProgramObject;
	struct IShaderObject;
};

class CShaderHandler {
public:
	typedef std::unordered_map<std::string, Shader::IProgramObject*> ProgramObjMap;
	typedef std::unordered_map<std::string, Shader::IProgramObject*>::iterator ProgramObjMapIt;
	typedef std::unordered_map<std::string, ProgramObjMap> ProgramTable;

	~CShaderHandler();

	static CShaderHandler* GetInstance(unsigned int instanceValue);
	static void FreeInstance(CShaderHandler*);

	void ReloadAll();
	bool ReleaseProgramObjects(const std::string& poClass);
	void ReleaseProgramObjectsMap(ProgramObjMap& poMap);

	Shader::IProgramObject* GetProgramObject(const std::string& poClass, const std::string& poName);
	Shader::IProgramObject* CreateProgramObject(const std::string& poClass, const std::string& poName, bool arbProgram);
	/**
	 * @param soName The filepath to the shader.
	 * @param soDefs Additional preprocessor flags passed as header.
	 * @param soType In case of an ARB shader it must be either GL_VERTEX_PROGRAM_ARB & GL_FRAGMENT_PROGRAM_ARB.
	 *               In case of an GLSL shader it can be GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, ...
	 */
	Shader::IShaderObject* CreateShaderObject(const std::string& soName, const std::string& soDefs, int soType);


	struct ShaderCache {
	public:
		void Clear() { cache.clear(); }

		unsigned int Find(unsigned int hash) {
			const auto it = cache.find(hash);

			GLuint id = 0;

			if (it != cache.end()) {
				id = it->second;
				cache.erase(it);
			}

			return id;
		}

		bool Push(unsigned int hash, unsigned int objID) {
			const auto it = cache.find(hash);

			if (it == cache.end()) {
				cache[hash] = objID;
				return true;
			}

			return false;
		}

	private:
		std::unordered_map<size_t, GLuint> cache;
	};

	const ShaderCache& GetShaderCache() const { return shaderCache; }
	      ShaderCache& GetShaderCache()       { return shaderCache; }

private:
	// all created programs, by name
	ProgramTable programObjects;
	// all (re)loaded program ID's, by hash
	ShaderCache shaderCache;
};

#define shaderHandler (CShaderHandler::GetInstance(1))

#endif
