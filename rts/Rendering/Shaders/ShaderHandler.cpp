/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"

#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/GlobalRendering.h"
#include "System/Log/ILog.h"
#include "System/Util.h"

#include <cassert>


// not extern'ed, so static
static CShaderHandler* gShaderHandler = nullptr;
static unsigned int gNumInstances = 0;

CShaderHandler* CShaderHandler::GetInstance(unsigned int instanceValue) {
	assert(instanceValue <= 1);

	if (gShaderHandler == nullptr) {
		gShaderHandler = new CShaderHandler();

		gNumInstances *= instanceValue;
		gNumInstances += 1;
	}

	// nobody should bring us back to life after FreeInstance
	// (unless n==0, which indicates we have just [re]loaded)
	assert(gNumInstances <= 1);
	return gShaderHandler;
}

void CShaderHandler::FreeInstance(CShaderHandler* sh) {
	assert(sh == gShaderHandler);
	delete sh;
	gShaderHandler = nullptr;
}



CShaderHandler::~CShaderHandler() {
	for (auto it = programObjects.begin(); it != programObjects.end(); ++it) {
		// release by poMap (not poClass) to avoid erase-while-iterating pattern
		ReleaseProgramObjectsMap(it->second);
	}

	programObjects.clear();
	shaderCache.Clear();
}


void CShaderHandler::ReloadAll() {
	for (auto it = programObjects.cbegin(); it != programObjects.cend(); ++it) {
		for (auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt) {
			(jt->second)->Reload(true, true);
		}
	}
}

bool CShaderHandler::ReleaseProgramObjects(const std::string& poClass) {
	if (programObjects.find(poClass) == programObjects.end())
		return false;

	ReleaseProgramObjectsMap(programObjects[poClass]);

	programObjects.erase(poClass);
	return true;
}

void CShaderHandler::ReleaseProgramObjectsMap(ProgramObjMap& poMap) {
	for (auto it = poMap.cbegin(); it != poMap.cend(); ++it) {
		Shader::IProgramObject* po = it->second;

		// free the program object and its attachments
		if (po != Shader::nullProgramObject) {
			po->Release(); delete po;
		}
	}

	poMap.clear();
}


Shader::IProgramObject* CShaderHandler::GetProgramObject(const std::string& poClass, const std::string& poName) {
	if (programObjects.find(poClass) == programObjects.end())
		return nullptr;

	if (programObjects[poClass].find(poName) == programObjects[poClass].end())
		return nullptr;

	return (programObjects[poClass][poName]);
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
	Shader::IShaderObject* so = Shader::nullShaderObject;

	switch (soType) {
		case GL_VERTEX_PROGRAM_ARB:
		case GL_FRAGMENT_PROGRAM_ARB: {
			// assert(StringToLower(soName).find("arb") != std::string::npos);

			if (globalRendering->haveARB) {
				so = new Shader::ARBShaderObject(soType, soName);
			}
		} break;

		default: {
			// assume GLSL shaders by default
			// assert(StringToLower(soName).find("arb") == std::string::npos);

			if (globalRendering->haveGLSL) {
				so = new Shader::GLSLShaderObject(soType, soName, soDefs);
			}
		} break;
	}

	if (so == Shader::nullShaderObject) {
		LOG_L(L_ERROR, "[%s] Tried to create a shader (\"%s\") on hardware that does not support them!",
			__FUNCTION__, soName.c_str());
		return so;
	}

	return so;
}
