/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_SHADERHANDLER_HDR
#define SPRING_SHADERHANDLER_HDR

#include <string>

#include "Rendering/GL/myGL.h" //GLuint
#include "System/StringHash.h"
#include "System/UnorderedMap.hpp"

namespace Shader {
	struct IProgramObject;
	struct IShaderObject;
};

class CShaderHandler {
public:
	typedef spring::unsynced_map<std::string, Shader::IProgramObject*> ProgramObjMap;
	typedef spring::unsynced_map<std::string, Shader::IProgramObject*>::iterator ProgramObjMapIt;
	typedef spring::unsynced_map<std::string, ProgramObjMap> ProgramTable;
	// indexed by literals
	typedef spring::unsynced_map<unsigned int, std::array<std::string, GL::SHADER_TYPE_CNT>> ExtShaderSourceMap;
	typedef spring::unsynced_map<unsigned int, Shader::IProgramObject*> ExtProgramObjMap;

	static CShaderHandler* GetInstance();

	void ReloadShaders(bool persistent);
	void ReloadAll() {
		ReloadShaders(false);
		ReloadShaders(true);
	}

	void ClearShaders(bool persistent);
	void ClearAll() {
		ClearShaders(false);
		ClearShaders(true);
		shaderCache.Clear();
	}


	void InsertExtProgramObject(const char* name, Shader::IProgramObject* prog);
	void RemoveExtProgramObject(const char* name, Shader::IProgramObject* prog) {
		extProgramObjects.erase(hashString(name));
		extShaderSources.erase(hashString(name));
	}

	bool ReleaseProgramObjects(const std::string& poClass, bool persistent = false);
	void ReleaseProgramObjectsMap(ProgramObjMap& poMap);


	Shader::IProgramObject* CreateProgramObject(const std::string& poClass, const std::string& poName, bool persistent = false);
	/**
	 * @param soName The filepath to the shader.
	 * @param soDefs Additional preprocessor flags passed as header.
	 * @param soType Can be GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, ...
	 */
	Shader::IShaderObject* CreateShaderObject(const std::string& soName, const std::string& soDefs, int soType);

	Shader::IProgramObject* GetExtProgramObject(const char* name) {
		const auto it = extProgramObjects.find(hashString(name));

		if (it == extProgramObjects.end())
			return nullptr;

		return (it->second);
	}

	const std::array<std::string, GL::SHADER_TYPE_CNT>* GetExtShaderSources(const char* name) const {
		const auto it = extShaderSources.find(hashString(name));

		if (it == extShaderSources.end())
			return nullptr;

		return &it->second;
	}


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
		spring::unsynced_map<size_t, GLuint> cache;
	};

	const ShaderCache& GetShaderCache() const { return shaderCache; }
	      ShaderCache& GetShaderCache()       { return shaderCache; }

private:
	// [0] := game-created programs, by name
	// [1] := menu-created (persistent) programs, by name
	ProgramTable programObjects[2];

	// holds external programs not created by CreateProgramObject
	ExtProgramObjMap extProgramObjects;
	ExtShaderSourceMap extShaderSources;

	// all (re)loaded program ID's, by hash
	ShaderCache shaderCache;
};

#define shaderHandler (CShaderHandler::GetInstance())

#endif
