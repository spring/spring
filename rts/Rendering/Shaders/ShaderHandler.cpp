/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "System/Log/ILog.h"

#include <cassert>


CShaderHandler* CShaderHandler::GetInstance()
{
	static CShaderHandler instance;
	return &instance;
}


void CShaderHandler::ClearShaders(bool persistent)
{
	for (auto& p: programObjects[persistent]) {
		// release by poMap (not poClass) to avoid erase-while-iterating pattern
		ReleaseProgramObjectsMap(p.second);
	}

	programObjects[persistent].clear();
}


void CShaderHandler::ReloadShaders(bool persistent)
{
	for (const auto& p1: programObjects[persistent]) {
		for (const auto& p2: p1.second) {
			p2.second->Reload(true, false);
		}
	}
}


void CShaderHandler::InsertExtProgramObject(const char* name, Shader::IProgramObject* prog)
{
	assert(name[0] != 0);

	if (GetExtProgramObject(name) != nullptr)
		return;

	assert(GetExtShaderSources(name) == nullptr);

	extProgramObjects.insert(name, prog);
	extShaderSources.insert(name, {});

	// the attached shader objects may go away later, so make
	// copies of their source strings while those still exist
	for (const Shader::IShaderObject* shader: prog->GetShaderObjs()) {
		extShaderSources[name][ GL::ShaderTypeToEnum(shader->GetType()) ] = shader->GetSrc(false);
	}
}


bool CShaderHandler::ReleaseProgramObjects(const std::string& poClass, bool persistent)
{
	ProgramTable& pTable = programObjects[persistent];

	if (pTable.find(poClass) == pTable.end())
		return false;

	ReleaseProgramObjectsMap(pTable[poClass]);

	pTable.erase(poClass);
	return true;
}

void CShaderHandler::ReleaseProgramObjectsMap(ProgramObjMap& poMap)
{
	for (const auto& item: poMap) {
		Shader::IProgramObject* po = item.second;

		// free the program object and its attachments
		if (po == Shader::nullProgramObject)
			continue;

		// erases
		shaderCache.Find(po->GetHash());
		po->Release();
		delete po;
	}

	poMap.clear();
}


Shader::IProgramObject* CShaderHandler::CreateProgramObject(const std::string& poClass, const std::string& poName, bool persistent)
{
	assert(!poClass.empty());
	assert(!poName.empty());

	ProgramTable& pTable = programObjects[persistent];

	if (pTable.find(poClass) == pTable.end())
		pTable[poClass] = ProgramObjMap();

	if (pTable[poClass].find(poName) == pTable[poClass].end())
		return (pTable[poClass][poName] = new Shader::GLSLProgramObject(poName));

	// do not allow multiple references to the same shader
	LOG_L(L_WARNING, "[SH::%s] program-object \"%s\" in class \"%s\" already exists", __func__, poName.c_str(), poClass.c_str());
	return Shader::nullProgramObject;
}


Shader::IShaderObject* CShaderHandler::CreateShaderObject(const std::string& soName, const std::string& soDefs, int soType)
{
	assert(!soName.empty());
	return (new Shader::GLSLShaderObject(soType, soName, soDefs));
}

