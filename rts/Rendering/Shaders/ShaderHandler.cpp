/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"

#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/GlobalRendering.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/SafeUtil.h"

#include <cassert>


CShaderHandler* CShaderHandler::GetInstance() {
	if (gShaderHandler == nullptr) {
		gShaderHandler = new CShaderHandler();
	}

	return gShaderHandler;
}

void CShaderHandler::FreeInstance() {
	spring::SafeDelete(gShaderHandler);
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

bool CShaderHandler::ReleaseProgramObject(const std::string& poClass, const std::string& poName)
{
	auto classIter = programObjects.find(poClass);
	if (classIter == programObjects.end())
		return false;

	auto nameIter = classIter->second.find(poName);
	if (nameIter == classIter->second.end())
		return false;

	// free the program object and its attachments
	if (nameIter->second != Shader::nullProgramObject) {
		nameIter->second->Release(); delete nameIter->second;
	}
	classIter->second.erase(nameIter);

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
#ifndef HEADLESS
	const char* poType = arbProgram? "ARB": "GLSL";

	if (programObjects.find(poClass) != programObjects.end()) {
		if (programObjects[poClass].find(poName) != programObjects[poClass].end()) {
			LOG_L(L_WARNING, "[SH::%s] program-object \"%s\" already exists", __func__, poName.c_str());
			assert(false);
			return (programObjects[poClass][poName]);
		}
	} else {
		programObjects[poClass] = ProgramObjMap();
	}

	if (arbProgram) {
		if (globalRendering->haveARB)
			po = new Shader::ARBProgramObject(poName);

	} else {
		if (globalRendering->haveGLSL)
			po = new Shader::GLSLProgramObject(poName);

	}

	if (po == Shader::nullProgramObject)
		LOG_L(L_ERROR, "[SH::%s] hardware does not support creating (%s) program-object \"%s\"", __func__, poType, poName.c_str());
#endif
	programObjects[poClass][poName] = po;
	return po;
}



Shader::IShaderObject* CShaderHandler::CreateShaderObject(const std::string& soName, const std::string& soDefs, int soType) {
	assert(!soName.empty());
	Shader::IShaderObject* so = Shader::nullShaderObject;
#ifndef HEADLESS
	switch (soType) {
		case GL_VERTEX_PROGRAM_ARB:
		case GL_FRAGMENT_PROGRAM_ARB: {
			// assert(StringToLower(soName).find("arb") != std::string::npos);

			if (globalRendering->haveARB)
				so = new Shader::ARBShaderObject(soType, soName);

		} break;

		default: {
			// assume GLSL shaders by default
			// assert(StringToLower(soName).find("arb") == std::string::npos);

			if (globalRendering->haveGLSL)
				so = new Shader::GLSLShaderObject(soType, soName, soDefs);

		} break;
	}

	if (so == Shader::nullShaderObject) {
		LOG_L(L_ERROR, "[SH::%s] hardware does not support creating shader-object \"%s\"", __func__, soName.c_str());
		return so;
	}
#endif
	return so;
}
