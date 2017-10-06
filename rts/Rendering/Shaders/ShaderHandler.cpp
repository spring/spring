/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"

#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/GlobalRendering.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"

#include <cassert>


CShaderHandler* CShaderHandler::GetInstance()
{
	static CShaderHandler instance;

	return &instance;
}

void CShaderHandler::FreeInstance()
{
	shaderHandler->ClearGameShaders();
	shaderHandler->ClearPersistentShaders();
	shaderHandler->shaderCache.Clear();
}

void CShaderHandler::ClearGameShaders()
{
	for (auto &p: gameProgramObjects) {
		// release by poMap (not poClass) to avoid erase-while-iterating pattern
		ReleaseProgramObjectsMap(p.second);
	}

	gameProgramObjects.clear();
}

void CShaderHandler::ClearPersistentShaders()
{
	for (auto &p: persistentProgramObjects) {
		// release by poMap (not poClass) to avoid erase-while-iterating pattern
		ReleaseProgramObjectsMap(p.second);
	}

	persistentProgramObjects.clear();
}


void CShaderHandler::ReloadAll()
{
	for (const auto& p1: gameProgramObjects) {
		for (const auto& p2: p1.second) {
			p2.second->Reload(true, true);
		}
	}
	for (const auto& p1: persistentProgramObjects) {
		for (const auto& p2: p1.second) {
			p2.second->Reload(true, true);
		}
	}
}

bool CShaderHandler::ReleaseProgramObjects(const std::string& poClass, bool persistent)
{
	ProgramTable& pTable = persistent ? persistentProgramObjects : gameProgramObjects;
	if (pTable.find(poClass) == pTable.end())
		return false;

	ReleaseProgramObjectsMap(pTable[poClass]);

	pTable.erase(poClass);
	return true;
}

void CShaderHandler::ReleaseProgramObjectsMap(ProgramObjMap& poMap)
{
	for (auto it = poMap.cbegin(); it != poMap.cend(); ++it) {
		Shader::IProgramObject* po = it->second;

		// free the program object and its attachments
		if (po != Shader::nullProgramObject) {
			// erases
			shaderCache.Find(po->GetHash());
			po->Release();
			delete po;
		}
	}

	poMap.clear();
}


Shader::IProgramObject* CShaderHandler::CreateProgramObject(const std::string& poClass, const std::string& poName, bool persistent)
{
	ProgramTable& pTable = persistent ? persistentProgramObjects : gameProgramObjects;

	Shader::IProgramObject* po = Shader::nullProgramObject;

	if (pTable.find(poClass) != pTable.end()) {
		if (pTable[poClass].find(poName) != pTable[poClass].end()) {
			LOG_L(L_WARNING, "[SH::%s] program-object \"%s\" already exists", __func__, poName.c_str());
			return (pTable[poClass][poName]);
		}
	} else {
		pTable[poClass] = ProgramObjMap();
	}

	if ((po = new Shader::GLSLProgramObject(poName)) == Shader::nullProgramObject)
		LOG_L(L_ERROR, "[SH::%s] hardware does not support creating program-object \"%s\"", __func__, poName.c_str());

	pTable[poClass][poName] = po;
	return po;
}



Shader::IShaderObject* CShaderHandler::CreateShaderObject(const std::string& soName, const std::string& soDefs, int soType)
{
	assert(!soName.empty());
	Shader::IShaderObject* so = Shader::nullShaderObject;

	if ((so = new Shader::GLSLShaderObject(soType, soName, soDefs)) == Shader::nullShaderObject) {
		LOG_L(L_ERROR, "[SH::%s] hardware does not support creating shader-object \"%s\"", __func__, soName.c_str());
		return so;
	}

	return so;
}

