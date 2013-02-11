/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"

#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/GlobalRendering.h"
#include "System/Log/ILog.h"
#include "System/Util.h"

#include <cassert>


CShaderHandler* CShaderHandler::GetInstance() {
	static CShaderHandler shaHandler;
	return &shaHandler;
}


void CShaderHandler::ReloadAll() {
	for (std::map<std::string, ProgramObjMap>::iterator it = programObjects.begin(); it != programObjects.end(); ++it) {
		for (ProgramObjMapIt jt = it->second.begin(); jt != it->second.end(); ++jt) {
			(jt->second)->Reload(true);
		}
	}
}


void CShaderHandler::ReleaseProgramObjects(const std::string& poClass) {
	if (programObjects.find(poClass) == programObjects.end()) {
		return;
	}

	for (ProgramObjMapIt it = programObjects[poClass].begin(); it != programObjects[poClass].end(); ++it) {
		// free the program object and its attachments
		if (it->second != Shader::nullProgramObject) {
			(it->second)->Release(); delete (it->second);
		}
	}

	programObjects[poClass].clear();
	programObjects.erase(poClass);
}


Shader::IProgramObject* CShaderHandler::GetProgramObject(const std::string& poClass, const std::string& poName) {
	if (programObjects.find(poClass) != programObjects.end()) {
		if (programObjects[poClass].find(poName) != programObjects[poClass].end()) {
			return (programObjects[poClass][poName]);
		}
	}
	return NULL;
}


Shader::IProgramObject* CShaderHandler::CreateProgramObject(const std::string& poClass, const std::string& poName, bool arbProgram) {
	Shader::IProgramObject* po = Shader::nullProgramObject;

	if (programObjects.find(poClass) != programObjects.end()) {
		if (programObjects[poClass].find(poName) != programObjects[poClass].end()) {
			LOG_L(L_WARNING, "[%s] There is already a shader program named \"%s\"!",
					__FUNCTION__, poName.c_str());
			return (programObjects[poClass][poName]);
		}
	} else {
		programObjects[poClass] = ProgramObjMap();
	}

	if (arbProgram) {
		if (globalRendering->haveARB) {
			po = new Shader::ARBProgramObject(poName);
		}
	} else {
		if (globalRendering->haveGLSL) {
			po = new Shader::GLSLProgramObject(poName);
		}
	}

	if (po == Shader::nullProgramObject) {
		LOG_L(L_ERROR, "[%s] Tried to create \"%s\" a %s shader program on hardware w/o support for it!",
				__FUNCTION__, poName.c_str(), arbProgram ? "ARB" : "GLSL");
	}

	programObjects[poClass][poName] = po;
	return po;
}



Shader::IShaderObject* CShaderHandler::CreateShaderObject(const std::string& soName, const std::string& soDefs, int soType) {
	assert(!soName.empty());

	const std::string lowerSoName = StringToLower(soName);

	bool arbShader = (lowerSoName.find("arb") != std::string::npos);
/*
	bool arbShader =
		lowerSoName.find(".glsl") == std::string::npos &&
		lowerSoName.find(".vert") == std::string::npos &&
		lowerSoName.find(".frag") == std::string::npos;
*/
	Shader::IShaderObject* so = Shader::nullShaderObject;

	switch (soType) {
		case GL_VERTEX_PROGRAM_ARB:
		case GL_FRAGMENT_PROGRAM_ARB: {
			assert(arbShader);
			if (globalRendering->haveARB) {
				so = new Shader::ARBShaderObject(soType, soName);
			}

		} break;

		default: {
			// assume GLSL shaders by default
			assert(!arbShader);
			if (globalRendering->haveGLSL) {
				so = new Shader::GLSLShaderObject(soType, soName, soDefs);
			}
		} break;
	}

	if (so == Shader::nullShaderObject) {
		LOG_L(L_ERROR, "[%s] Tried to create a %s shader (\"%s\") on a hardware that does not support them!",
				__FUNCTION__, arbShader? "ARB": "GLSL", soName.c_str());
		return so;
	}

	so->Compile(true);
	return so;
}
