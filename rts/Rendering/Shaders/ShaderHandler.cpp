#include <GL/glew.h>
#include <cassert>

#include "Rendering/Shaders/ShaderHandler.hpp"
#include "System/FileSystem/FileHandler.h"
#include "System/LogOutput.h"

CShaderHandler* CShaderHandler::GetInstance() {
	static CShaderHandler shaHandler;
	return &shaHandler;
}



void CShaderHandler::ReleaseProgramObjects() {
	for (ProgramObjMapIt it = programObjMap.begin(); it != programObjMap.end(); it++) {
		(it->second)->Release();
		delete (it->second);
	}

	programObjMap.clear();
}

Shader::IProgramObject* CShaderHandler::CreateProgramObject(const std::string& poKey, bool arbProgram) {
	Shader::IProgramObject* po = NULL;

	if (programObjMap.find(poKey) != programObjMap.end()) {
		return (programObjMap[poKey]);
	}

	if (arbProgram) {
		po = new Shader::ARBProgramObject();
	} else {
		po = new Shader::GLSLProgramObject();
	}

	programObjMap[poKey] = po;
	return po;
}

Shader::IProgramObject* CShaderHandler::CreateProgramObject(const std::string& poKey, const std::string& vsStr, const std::string& fsStr, bool arbProgram) {
	Shader::IProgramObject* po = CreateProgramObject(poKey, arbProgram);

	if (po->IsValid()) {
		return po;
	}

	Shader::IShaderObject* vso = CreateShaderObject(vsStr, (arbProgram? GL_VERTEX_PROGRAM_ARB: GL_VERTEX_SHADER));
	Shader::IShaderObject* fso = CreateShaderObject(fsStr, (arbProgram? GL_FRAGMENT_PROGRAM_ARB: GL_FRAGMENT_SHADER));

	po->AttachShaderObject(vso);
	po->AttachShaderObject(fso);

	po->Link();

	if (!po->IsValid()) {
		logOutput.Print("[CShaderHandler::CreateProgramObject]\n");
		logOutput.Print("\tprogram-object key: %s, link-log:\n%s\n", poKey.c_str(), po->GetLog().c_str());
	}
	return po;
}



Shader::IShaderObject* CShaderHandler::CreateShaderObject(const std::string& shaderName, int shaderType) {
	assert(!shaderName.empty());

	bool arbShader = false;

	std::string shaderSrc("");
	CFileHandler shaderFile("shaders/" + shaderName);

	if (shaderFile.FileExists()) {
		arbShader =
			shaderName.find(".glsl") == std::string::npos &&
			shaderName.find(".vert") == std::string::npos &&
			shaderName.find(".frag") == std::string::npos;

		std::vector<char> shaderFileBuffer(shaderFile.FileSize() + 1, 0);
		shaderFile.Read(&shaderFileBuffer[0], shaderFile.FileSize());

		shaderSrc = std::string(&shaderFileBuffer[0]);
	} else {
		arbShader =
			(shaderName.find("!!ARBvp") != std::string::npos) ||
			(shaderName.find("!!ARBfp") != std::string::npos);
		shaderSrc = shaderName;
	}

	Shader::IShaderObject* so = NULL;

	switch (shaderType) {
		case GL_VERTEX_PROGRAM_ARB:
		case GL_FRAGMENT_PROGRAM_ARB: {
			assert(arbShader);
			so = new Shader::ARBShaderObject(shaderType, shaderSrc);
		} break;

		case GL_VERTEX_SHADER:
		case GL_FRAGMENT_SHADER: {
			assert(!arbShader);
			so = new Shader::GLSLShaderObject(shaderType, shaderSrc);
		} break;
	}

	assert(so != NULL);
	so->Compile();

	if (!so->IsValid()) {
		logOutput.Print("[CShaderHandler::CreateShaderObject]\n");
		logOutput.Print("\tshader-object name: %s, compile-log:\n%s\n", shaderName.c_str(), (so->GetLog()).c_str());
	}
	return so;
}
