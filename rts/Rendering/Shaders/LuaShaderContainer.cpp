/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaShaderContainer.h"
#include "Lua/LuaParser.h"
#include "Lua/LuaUtils.h"
#include "Lua/LuaUnsyncedCtrl.h"
#include "Lua/LuaUnsyncedRead.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include <sstream>


enum {
	UNIFORM_TYPE_INT          = 0, // includes arrays
	UNIFORM_TYPE_FLOAT        = 1, // includes arrays
	UNIFORM_TYPE_FLOAT_MATRIX = 2,
};


static void ParseUniformsTable(Shader::IProgramObject* program, const LuaTable* root, const std::string& fieldName, int type)
{
	const LuaTable subTable = root->SubTable(fieldName);
	std::vector<std::string> keys;
	subTable.GetKeys(keys);

	for (const std::string& key: keys) {
		switch (type) {
			case UNIFORM_TYPE_INT: {
				if (subTable.GetType(key) == LuaTable::TABLE) {
					const LuaTable values = subTable.SubTable(key);
					int vi[4];
					for (int i = 0; i<4; ++i) {
						vi[i] = values.GetInt(i+1, 0);
					}
					program->SetUniform4v(key.c_str(), vi);
				} else {
					program->SetUniform(key.c_str(), subTable.GetInt(key, 0));
				}
			} break;
			case UNIFORM_TYPE_FLOAT: {
				if (subTable.GetType(key) == LuaTable::TABLE) {
					const LuaTable values = subTable.SubTable(key);
					float vf[4];
					for (int i = 0; i<4; ++i) {
						vf[i] = values.GetFloat(i+1, 0.0f);
					}
					program->SetUniform4v(key.c_str(), vf);
				} else {
					program->SetUniform(key.c_str(), subTable.GetFloat(key, 0.0f));
				}
			} break;
			case UNIFORM_TYPE_FLOAT_MATRIX: {
				if (subTable.GetType(key) == LuaTable::TABLE) {
					const LuaTable values = subTable.SubTable(key);
					float vf[16];
					switch (values.GetLength()) {
						case 4: {
							for (int i = 0; i<4; ++i) {
								vf[i] = values.GetFloat(i+1, 0.0f);
							}
							program->SetUniformMatrix2x2(key.c_str(), false, vf);
						} break;
						case 9: {
							for (int i = 0; i<9; ++i) {
								vf[i] = values.GetFloat(i+1, 0.0f);
							}
							program->SetUniformMatrix3x3(key.c_str(), false, vf);
						} break;
						case 16: {
							for (int i = 0; i<16; ++i) {
								vf[i] = values.GetFloat(i+1, 0.0f);
							}
							program->SetUniformMatrix4x4(key.c_str(), false, vf);
						} break;
						default:
							LOG_L(L_ERROR, "%s-shader uniformMatrix error: only square matrices supported right now: %s", program->GetName().c_str(), program->GetLog().c_str());
					}
				}
			} break;
		};
	}
}


static void ParseUniformSetupTables(Shader::IProgramObject* program, const LuaTable* root)
{
	ParseUniformsTable(program, root, "uniform",       UNIFORM_TYPE_FLOAT       );
	ParseUniformsTable(program, root, "uniformFloat",  UNIFORM_TYPE_FLOAT       );
	ParseUniformsTable(program, root, "uniformInt",    UNIFORM_TYPE_INT         );
	ParseUniformsTable(program, root, "uniformMatrix", UNIFORM_TYPE_FLOAT_MATRIX);
}


static void CreateShaderObject(
	Shader::IProgramObject* program,
	const std::string& definitions,
	const std::string& sources,
	const GLenum type
) {
	if (sources.empty())
		return;

	program->AttachShaderObject(shaderHandler->CreateShaderObject(sources, definitions, type));
}


static void ParseShaderTable(
	const LuaTable* root,
	const std::string key,
	std::stringstream& data
) {
	const auto keyType = root->GetType(key);

	switch (keyType) {
		case LuaTable::STRING: {
			data << root->GetString(key, "");
		} break;
		case LuaTable::TABLE: {
			const LuaTable shaderCode = root->SubTable(key);
			for (int i = 1; shaderCode.KeyExists(i); ++i) {
				data << shaderCode.GetString(i, "");
			}
		} break;
		default:
			if (root->KeyExists(key)) {
				//FIXME shaders.errorLog = "\"" + string(key) + "\" must be a string or a table value!";
			}
	};
}


static void LoadTextures(Shader::IProgramObject* program, const LuaTable* root)
{
	const LuaTable& textures = root->SubTable("textures");
	std::vector<std::pair<int, std::string>> data;

	data.reserve(8);
	textures.GetPairs(data);

	for (const auto& p: data) {
		program->AddTextureBinding(p.first, p.second);
	}
}


namespace Shader {
bool LoadFromLua(Shader::IProgramObject* program, const std::string& filename)
{
	// lua only supports glsl shaders
	assert(dynamic_cast<Shader::GLSLProgramObject*>(program) != nullptr);

	LuaParser p(filename, SPRING_VFS_RAW_FIRST, SPRING_VFS_MOD_BASE);
	p.SetLowerKeys(false);
	p.SetLowerCppKeys(false);

	p.GetTable("Spring");
	p.AddFunc("GetConfigInt",     LuaUnsyncedRead::GetConfigInt);
	p.AddFunc("GetConfigFloat",   LuaUnsyncedRead::GetConfigFloat);
	p.AddFunc("GetConfigString",  LuaUnsyncedRead::GetConfigString);
	p.AddFunc("GetLosViewColors", LuaUnsyncedRead::GetLosViewColors);
	p.EndTable();

	if (!p.Execute()) {
		LOG_L(L_ERROR, "Failed to parse lua shader \"%s\": %s", filename.c_str(), p.GetErrorLog().c_str());
		return false;
	}
	const LuaTable root = p.GetRoot();

	LoadTextures(program, &root);

	std::stringstream shdrDefs;
	std::stringstream vertSrcs;
	std::stringstream geomSrcs;
	std::stringstream fragSrcs;

	ParseShaderTable(&root, "definitions", shdrDefs);
	ParseShaderTable(&root, "vertex",   vertSrcs);
	ParseShaderTable(&root, "geometry", geomSrcs);
	ParseShaderTable(&root, "fragment", fragSrcs);

	if (vertSrcs.str().empty() && fragSrcs.str().empty() && geomSrcs.str().empty())
		return false;

	CreateShaderObject(program, shdrDefs.str(), vertSrcs.str(), GL_VERTEX_SHADER);
	CreateShaderObject(program, shdrDefs.str(), geomSrcs.str(), GL_GEOMETRY_SHADER);
	CreateShaderObject(program, shdrDefs.str(), fragSrcs.str(), GL_FRAGMENT_SHADER);

	//FIXME ApplyGeometryParameters(L, 1, prog); // done before linking

	program->Link();
	if (!program->IsValid()) {
		const char* fmt = "%s-shader compilation error: %s";
		LOG_L(L_ERROR, fmt, program->GetName().c_str(), program->GetLog().c_str());
		return false;
	} else {
		program->Enable();
		ParseUniformSetupTables(program, &root);
		program->Disable();
		program->Validate();
		if (!program->IsValid()) {
			const char* fmt = "%s-shader validation error: %s";
			LOG_L(L_ERROR, fmt, program->GetName().c_str(), program->GetLog().c_str());
			return false;
		}
	}

	return true;
}
};
