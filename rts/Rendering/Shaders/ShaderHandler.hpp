#ifndef SPRING_SHADERHANDLER_HDR
#define SPRING_SHADERHANDLER_HDR

#include <string>
#include <map>

#include "Rendering/Shaders/Shader.hpp"

class CShaderHandler {
public:
	static CShaderHandler* GetInstance();

	void ReleaseProgramObjects();
	Shader::IProgramObject* CreateProgramObject(const std::string&, bool);
	Shader::IProgramObject* CreateProgramObject(const std::string&, const std::string&, const std::string&, bool);
	Shader::IShaderObject* CreateShaderObject(const std::string&, int);

private:
	std::string GetShaderSourceText(const std::string&);
	std::map<std::string, Shader::IProgramObject*> programObjMap;

	typedef std::map<std::string, Shader::IProgramObject*> ProgramObjMap;
	typedef std::map<std::string, Shader::IProgramObject*>::iterator ProgramObjMapIt;
};

#define shaderHandler (CShaderHandler::GetInstance())

#endif
