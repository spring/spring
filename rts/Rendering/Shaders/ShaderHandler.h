/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_SHADERHANDLER_HDR
#define SPRING_SHADERHANDLER_HDR

#include <string>
#include <map>

#include "Rendering/Shaders/Shader.h"

class CShaderHandler {
public:
	static CShaderHandler* GetInstance();

	void ReloadAll();
	Shader::IProgramObject* GetProgramObject(const std::string& poClass, const std::string& poName);
	void ReleaseProgramObjects(const std::string& poClass);

	Shader::IProgramObject* CreateProgramObject(const std::string& poClass, const std::string& poName, bool arbProgram);
	/**
	 * @param soName The filepath to the shader.
	 * @param soDefs Additional preprocessor flags passed as header.
	 * @param soType In case of an ARB shader it must be either GL_VERTEX_PROGRAM_ARB & GL_FRAGMENT_PROGRAM_ARB.
	 *               In case of an GLSL shader it can be GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, ...
	 */
	Shader::IShaderObject* CreateShaderObject(const std::string& soName, const std::string& soDefs, int soType);

private:
	typedef std::map<std::string, Shader::IProgramObject*> ProgramObjMap;
	typedef std::map<std::string, Shader::IProgramObject*>::iterator ProgramObjMapIt;

	std::map<std::string, ProgramObjMap> programObjects;
};

#define shaderHandler (CShaderHandler::GetInstance())

#endif
