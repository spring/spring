/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"

#include "Rendering/Shaders/ShaderHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Log/ILog.h"

#include <cassert>


CShaderHandler* CShaderHandler::GetInstance() {
	static CShaderHandler shaHandler;
	return &shaHandler;
}



void CShaderHandler::ReleaseProgramObjects(const std::string& poClass) {
	if (programObjects.find(poClass) == programObjects.end()) {
		return;
	}

	for (ProgramObjMapIt it = programObjects[poClass].begin(); it != programObjects[poClass].end(); ++it) {
		// free the program object and its attachments
		(it->second)->Release(); delete (it->second);
	}

	programObjects[poClass].clear();
	programObjects.erase(poClass);
}

Shader::IProgramObject* CShaderHandler::CreateProgramObject(const std::string& poClass, const std::string& poName, bool arbProgram) {
	Shader::IProgramObject* po = NULL;

	if (programObjects.find(poClass) != programObjects.end()) {
		if (programObjects[poClass].find(poName) != programObjects[poClass].end()) {
			return (programObjects[poClass][poName]);
		}
	} else {
		programObjects[poClass] = ProgramObjMap();
	}

	if (arbProgram) {
		po = new Shader::ARBProgramObject();
	} else {
		po = new Shader::GLSLProgramObject();
	}

	programObjects[poClass][poName] = po;
	return po;
}

Shader::IProgramObject* CShaderHandler::CreateProgramObject(
	const std::string& poClass,
	const std::string& poName,
	const std::string& vsStr,
	const std::string& vsDefs,
	const std::string& fsStr,
	const std::string& fsDefs,
	bool arbProgram
) {
	Shader::IProgramObject* po = CreateProgramObject(poClass, poName, arbProgram);

	if (po->IsValid()) {
		return po;
	}

	Shader::IShaderObject* vso = CreateShaderObject(vsStr, vsDefs, (arbProgram? GL_VERTEX_PROGRAM_ARB: GL_VERTEX_SHADER));
	Shader::IShaderObject* fso = CreateShaderObject(fsStr, fsDefs, (arbProgram? GL_FRAGMENT_PROGRAM_ARB: GL_FRAGMENT_SHADER));

	po->AttachShaderObject(vso);
	po->AttachShaderObject(fso);
	po->Link();
	po->Validate();

	if (!po->IsValid()) {
		const char* fmt = "[%s]\n\tprogram-object name: %s, link-log:\n%s";
		const char* log = po->GetLog().c_str();
		LOG_L(L_WARNING, fmt, __FUNCTION__, poName.c_str(), log);
	}
	return po;
}



Shader::IShaderObject* CShaderHandler::CreateShaderObject(const std::string& soName, const std::string& soDefs, int soType) {
	assert(!soName.empty());

	bool arbShader = false;

	std::string soPath = "shaders/" + soName;
	std::string soSource = "";

	CFileHandler soFile(soPath);

	if (soFile.FileExists()) {
		std::vector<char> soFileBuffer(soFile.FileSize() + 1, 0);
		soFile.Read(&soFileBuffer[0], soFile.FileSize());

		arbShader =
			soName.find(".glsl") == std::string::npos &&
			soName.find(".vert") == std::string::npos &&
			soName.find(".frag") == std::string::npos;
		soSource = std::string(&soFileBuffer[0]);
	} else {
		LOG_L(L_WARNING, "[%s]\n\tfile \"%s\" does not exist, interpreting"
				" \"%s\" as literal shader source-string",
				__FUNCTION__, soPath.c_str(), soName.c_str());

		arbShader =
			(soName.find("!!ARBvp") != std::string::npos) ||
			(soName.find("!!ARBfp") != std::string::npos);
		soSource = soName;
	}

	if (!arbShader) {
		soSource = soDefs + soSource;
	}

	Shader::IShaderObject* so = NULL;

	switch (soType) {
		case GL_VERTEX_PROGRAM_ARB:
		case GL_FRAGMENT_PROGRAM_ARB: {
			assert(arbShader);
			so = new Shader::ARBShaderObject(soType, soSource);
		} break;

		case GL_VERTEX_SHADER:
		case GL_FRAGMENT_SHADER: {
			assert(!arbShader);
			so = new Shader::GLSLShaderObject(soType, soSource);
		} break;
	}

	assert(so != NULL);
	so->Compile();

	if (!so->IsValid()) {
		LOG_L(L_WARNING, "[%s]\n\tshader-object name: %s, compile-log:\n%s",
				__FUNCTION__, soName.c_str(), (so->GetLog()).c_str());
	}
	return so;
}
