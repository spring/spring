/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_SHADERHANDLER_HDR
#define SPRING_SHADERHANDLER_HDR

#include <string>
#include <map>
#include <unordered_map>

namespace Shader {
	struct IProgramObject;
	struct IShaderObject;
};

class CShaderHandler {
public:
	~CShaderHandler();

	static CShaderHandler* GetInstance();
	static void FreeInstance(CShaderHandler*);

	void ReloadAll();
	void ReleaseProgramObjects(const std::string& poClass);

	Shader::IProgramObject* GetProgramObject(const std::string& poClass, const std::string& poName);
	Shader::IProgramObject* CreateProgramObject(const std::string& poClass, const std::string& poName, bool arbProgram);
	/**
	 * @param soName The filepath to the shader.
	 * @param soDefs Additional preprocessor flags passed as header.
	 * @param soType In case of an ARB shader it must be either GL_VERTEX_PROGRAM_ARB & GL_FRAGMENT_PROGRAM_ARB.
	 *               In case of an GLSL shader it can be GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, ...
	 */
	Shader::IShaderObject* CreateShaderObject(const std::string& soName, const std::string& soDefs, int soType);

	const std::unordered_map<size_t, GLuint>& GetShaderProgramCache() const { return shaderProgramCache; }
	      std::unordered_map<size_t, GLuint>& GetShaderProgramCache()       { return shaderProgramCache; }

private:
	typedef std::map<std::string, Shader::IProgramObject*> ProgramObjMap;
	typedef std::map<std::string, Shader::IProgramObject*>::iterator ProgramObjMapIt;

	std::map<std::string, ProgramObjMap> programObjects;
	std::unordered_map<size_t, GLuint> shaderProgramCache;
};

#define shaderHandler (CShaderHandler::GetInstance())

#endif
