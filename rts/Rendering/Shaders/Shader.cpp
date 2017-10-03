/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/LuaShaderContainer.h"
#include "Rendering/Shaders/GLSLCopyState.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"

#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Sync/HsiehHash.h"
#include "System/Log/ILog.h"

#include "System/Config/ConfigHandler.h"

#include <algorithm>
#ifdef DEBUG
	#include <string.h> // strncmp
#endif


/*****************************************************************/

#define LOG_SECTION_SHADER "Shader"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_SHADER)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_SHADER

/*****************************************************************/

CONFIG(bool, UseShaderCache).defaultValue(true).description("If already compiled shaders should be shared via a cache, reducing compiles of already compiled shaders.");


/*****************************************************************/

static bool glslIsValid(GLuint obj)
{
	const bool isShader = glIsShader(obj);
	assert(glIsShader(obj) || glIsProgram(obj));

	GLint compiled;
	if (isShader)
		glGetShaderiv(obj, GL_COMPILE_STATUS, &compiled);
	else
		glGetProgramiv(obj, GL_LINK_STATUS, &compiled);

	return compiled;
}


static std::string glslGetLog(GLuint obj)
{
	const bool isShader = glIsShader(obj);
	assert(glIsShader(obj) || glIsProgram(obj));

	int infologLength = 0;
	int maxLength = 0;

	if (isShader)
		glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &maxLength);
	else
		glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &maxLength);

	std::string infoLog;
	infoLog.resize(maxLength);

	if (isShader)
		glGetShaderInfoLog(obj, maxLength, &infologLength, &infoLog[0]);
	else
		glGetProgramInfoLog(obj, maxLength, &infologLength, &infoLog[0]);

	infoLog.resize(infologLength);
	return infoLog;
}


static std::string GetShaderSource(const std::string& fileName)
{
	if (fileName.find("void main()") != std::string::npos)
		return fileName;

	std::string soPath = "shaders/" + fileName;
	std::string soSource = "";

	CFileHandler soFile(soPath);

	if (soFile.FileExists()) {
		soSource.resize(soFile.FileSize());
		soFile.Read(&soSource[0], soFile.FileSize());
	} else {
		LOG_L(L_ERROR, "[%s] file not found \"%s\"", __FUNCTION__, soPath.c_str());
	}

	return soSource;
}

static bool ExtractGlslVersion(std::string* src, std::string* version)
{
	const auto pos = src->find("#version ");

	if (pos != std::string::npos) {
		const auto eol = src->find('\n', pos) + 1;
		*version = src->substr(pos, eol - pos);
		src->erase(pos, eol - pos);
		return true;
	}
	return false;
}

/*****************************************************************/


namespace Shader {
	static NullShaderObject nullShaderObject_(0, "");
	static NullProgramObject nullProgramObject_("NullProgram");

	NullShaderObject* nullShaderObject = &nullShaderObject_;
	NullProgramObject* nullProgramObject = &nullProgramObject_;


	/*****************************************************************/

	unsigned int IShaderObject::GetHash() const {
		unsigned int hash = 127;
		hash = HsiehHash((const void*)   srcText.data(),    srcText.size(), hash); // srcTextHash is not worth it, only called on reload
		hash = HsiehHash((const void*)modDefStrs.data(), modDefStrs.size(), hash);
		hash = HsiehHash((const void*)rawDefStrs.data(), rawDefStrs.size(), hash); // rawDefStrsHash is not worth it, only called on reload
		return hash;
	}


	bool IShaderObject::ReloadFromDisk()
	{
		std::string newText = GetShaderSource(srcFile);

		if (newText != srcText) {
			srcText = std::move(newText);
			return true;
		}

		return false;
	}




	/*****************************************************************/

	GLSLShaderObject::GLSLShaderObject(
		unsigned int shType,
		const std::string& shSrcFile,
		const std::string& shSrcDefs
	): IShaderObject(shType, shSrcFile, shSrcDefs)
	{
	}

	GLSLShaderObject::CompiledShaderObjectUniquePtr GLSLShaderObject::CompileShaderObject()
	{
		CompiledShaderObjectUniquePtr res(new CompiledShaderObject(), [](CompiledShaderObject* so) {
			glDeleteShader(so->id);
			so->id = 0;
			spring::SafeDelete(so);
		});

		assert(!srcText.empty());

		std::string sourceStr = srcText;
		std::string defFlags  = rawDefStrs + "\n" + modDefStrs;
		std::string versionStr;

		// extract #version pragma and put it on the first line (only allowed there)
		// version pragma in definitions overrides version pragma in source (if any)
		ExtractGlslVersion(&sourceStr, &versionStr);
		ExtractGlslVersion(&defFlags,  &versionStr);

		if (!versionStr.empty()) EnsureEndsWith(&versionStr, "\n");
		if (!defFlags.empty())   EnsureEndsWith(&defFlags,   "\n");

		std::vector<const GLchar*> sources = {
			"// SHADER VERSION\n",
			versionStr.c_str(),
			"// SHADER FLAGS\n",
			defFlags.c_str(),
			"// SHADER SOURCE\n",
			"#line 1\n",
			sourceStr.c_str()
		};

		res->id = glCreateShader(type);

		glShaderSource(res->id, sources.size(), &sources[0], NULL);
		glCompileShader(res->id);

		res->valid = glslIsValid(res->id);
		res->log   = glslGetLog(res->id);

		if (!res->valid) {
			const std::string& name = srcFile.find("void main()") != std::string::npos ? "unknown" : srcFile;
			LOG_L(L_WARNING, "[GLSL-SO::%s] shader-object name: %s, compile-log:\n%s\n", __FUNCTION__, name.c_str(), res->log.c_str());
			LOG_L(L_WARNING, "\n%s%s%s%s%s%s%s", sources[0], sources[1], sources[2], sources[3], sources[4], sources[5], sources[6]);
		}

		return res;
	}




	/*****************************************************************/

	IProgramObject::IProgramObject(const std::string& poName): name(poName), objID(0), valid(false), bound(false) {
	}

	void IProgramObject::Release() {
		for (IShaderObject*& so: shaderObjs) {
			so->Release();
			delete so;
		}

		uniformStates.clear();
		shaderObjs.clear();
		luaTextures.clear();
		log.clear();

		valid = false;
	}


	bool IProgramObject::LoadFromLua(const std::string& filename) {
		return Shader::LoadFromLua(this, filename);
	}

	void IProgramObject::RecompileIfNeeded(bool validate)
	{
		if (shaderFlags.HashSet() && !shaderFlags.Updated())
			return;

		Reload(!shaderFlags.HashSet(), validate);
		PrintDebugInfo();
	}

	void IProgramObject::PrintDebugInfo()
	{
	#if DEBUG
		LOG_L(L_DEBUG, "Uniform States for program-object \"%s\":", name.c_str());
		LOG_L(L_DEBUG, "Defs:\n %s", (shaderFlags.GetString()).c_str());
		LOG_L(L_DEBUG, "Uniforms:");

		for (const auto& p : uniformStates) {
			const bool curUsed = GetUniformLocation(p.second.GetName()) >= 0;
			if (p.second.IsUninit()) {
				LOG_L(L_DEBUG, "\t%s: uninitialized used=%i", (p.second.GetName()).c_str(), int(curUsed));
			} else {
				LOG_L(L_DEBUG, "\t%s: x=float:%f;int:%i y=%f z=%f used=%i", (p.second.GetName()).c_str(), p.second.GetFltValues()[0], p.second.GetIntValues()[0], p.second.GetFltValues()[1], p.second.GetFltValues()[2], int(curUsed));
			}
		}
	#endif
	}

	UniformState* IProgramObject::GetNewUniformState(const std::string name)
	{
		const size_t hash = hashString(name.c_str());
		const auto it = uniformStates.emplace(hash, name);

		UniformState* us = &(it.first->second);
		us->SetLocation(GetUniformLoc(name));

		return us;
	}


	void IProgramObject::AddTextureBinding(const int texUnit, const std::string& luaTexName)
	{
		LuaMatTexture luaTex;

		if (!LuaOpenGLUtils::ParseTextureImage(nullptr, luaTex, luaTexName))
			return;

		luaTextures[texUnit] = luaTex;
	}

	void IProgramObject::BindTextures() const
	{
		for (const auto& p: luaTextures) {
			glActiveTexture(GL_TEXTURE0 + p.first);
			(p.second).Bind();
		}
		glActiveTexture(GL_TEXTURE0);
	}




	/*****************************************************************/

	GLSLProgramObject::GLSLProgramObject(const std::string& poName): IProgramObject(poName), curSrcHash(0) {
		objID = glCreateProgram();
	}

	void GLSLProgramObject::Enable() {
		RecompileIfNeeded(true);
		glUseProgram(objID);
		IProgramObject::Enable();
	}

	void GLSLProgramObject::Disable() {
		glUseProgram(0);
		IProgramObject::Disable();
	}

	void GLSLProgramObject::Link() {
		RecompileIfNeeded(false);
		assert(glIsProgram(objID));
	}

	bool GLSLProgramObject::Validate() {
		GLint validated = 0;

		glValidateProgram(objID);
		glGetProgramiv(objID, GL_VALIDATE_STATUS, &validated);
		valid = bool(validated);

		// append the validation-log
		log += glslGetLog(objID);

	#ifdef DEBUG
		// check if there are unset uniforms left
		GLsizei numUniforms, maxUniformNameLength;
		glGetProgramiv(objID, GL_ACTIVE_UNIFORMS, &numUniforms);
		glGetProgramiv(objID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformNameLength);

		if (maxUniformNameLength <= 0)
			return valid;

		std::string bufname(maxUniformNameLength, 0);
		for (int i = 0; i < numUniforms; ++i) {
			GLsizei nameLength = 0;
			GLint size = 0;
			GLenum type = 0;
			glGetActiveUniform(objID, i, maxUniformNameLength, &nameLength, &size, &type, &bufname[0]);
			bufname[nameLength] = 0;

			if (nameLength == 0)
				continue;

			if (strncmp(&bufname[0], "gl_", 3) == 0)
				continue;

			if (uniformStates.find(hashString(&bufname[0])) != uniformStates.end())
				continue;

			LOG_L(L_WARNING, "[GLSL-PO::%s] program-object name: %s, unset uniform: %s", __FUNCTION__, name.c_str(), &bufname[0]);
			//assert(false);
		}
	#endif

		return valid;
	}

	void GLSLProgramObject::Release() {
		IProgramObject::Release();
		glDeleteProgram(objID);
		shaderFlags.Clear();

		objID = 0;
		curSrcHash = 0;
	}

	void GLSLProgramObject::Reload(bool reloadFromDisk, bool validate) {
		const unsigned int oldProgID = objID;
		const unsigned int oldSrcHash = curSrcHash;

		const bool oldValid = IsValid();
		valid = false;

		{
			// NOTE: this does not preserve the #version pragma
			for (IShaderObject*& so: shaderObjs) {
				so->SetDefinitions(shaderFlags.GetString());
			}

			log.clear();
		}

		// reload shader from disk if requested or necessary
		if ((reloadFromDisk = reloadFromDisk || !oldValid || (oldProgID == 0))) {
			for (IShaderObject*& so: shaderObjs) {
				so->ReloadFromDisk();
			}
		}

		{
			// create shader source hash
			curSrcHash = shaderFlags.UpdateHash();

			for (const IShaderObject* so: shaderObjs) {
				curSrcHash ^= so->GetHash();
			}
		}

		// clear all uniform locations
		for (auto& us_pair: uniformStates) {
			us_pair.second.SetLocation(GL_INVALID_INDEX);
		}

		// early-exit: empty program (TODO: delete it?)
		if (shaderObjs.empty())
			return;

		bool deleteOldShader = true;

		{
			objID = 0;

			// push old program to cache and pop new if available
			if (oldValid && configHandler->GetBool("UseShaderCache")) {
				CShaderHandler::ShaderCache& shadersCache = shaderHandler->GetShaderCache();
				deleteOldShader = !shadersCache.Push(oldSrcHash, oldProgID);
				objID = shadersCache.Find(curSrcHash);
			}
		}

		// recompile if not found in cache (id 0)
		if (objID == 0) {
			objID = glCreateProgram();

			bool shadersValid = true;
			for (IShaderObject*& so: shaderObjs) {
				assert(dynamic_cast<GLSLShaderObject*>(so));

				auto gso = static_cast<GLSLShaderObject*>(so);
				auto obj = gso->CompileShaderObject();

				if (obj->valid) {
					glAttachShader(objID, obj->id);
				} else {
					shadersValid = false;
				}
			}

			if (!shadersValid)
				return;

			glLinkProgram(objID);

			valid = glslIsValid(objID);
			log += glslGetLog(objID);

			if (!IsValid()) {
				LOG_L(L_WARNING, "[GLSL-PO::%s] program-object name: %s, link-log:\n%s\n", __FUNCTION__, name.c_str(), log.c_str());
			}
		} else {
			valid = true;
		}

		// FIXME: fails on ATI, see https://springrts.com/mantis/view.php?id=4715
		if (validate && !globalRendering->haveATI)
			Validate();

		// copy full program state from old to new program (uniforms etc.)
		if (IsValid())
			GLSLCopyState(objID, oldValid ? oldProgID : 0, &uniformStates);

		// delete old program when not further used
		if (deleteOldShader)
			glDeleteProgram(oldProgID);
	}

	int GLSLProgramObject::GetUniformType(const int idx) {
		GLint size = 0;
		GLenum type = 0;
		// NB: idx can not be a *location* returned by glGetUniformLoc except on Nvidia
		glGetActiveUniform(objID, idx, 0, nullptr, &size, &type, nullptr);
		assert(size == 1); // arrays aren't handled yet
		return type;
	}

	int GLSLProgramObject::GetUniformLoc(const std::string& name) {
		return glGetUniformLocation(objID, name.c_str());
	}

	void GLSLProgramObject::SetUniformLocation(const std::string& name) {
		uniformLocs.push_back(hashString(name.c_str()));
		GetUniformLocation(name);
	}

	void GLSLProgramObject::SetUniform(UniformState* uState, int   v0)                               { assert(IsBound()); if (uState->Set(v0            )) glUniform1i(uState->GetLocation(), v0             ); }
	void GLSLProgramObject::SetUniform(UniformState* uState, float v0)                               { assert(IsBound()); if (uState->Set(v0            )) glUniform1f(uState->GetLocation(), v0             ); }
	void GLSLProgramObject::SetUniform(UniformState* uState, int   v0, int   v1)                     { assert(IsBound()); if (uState->Set(v0, v1        )) glUniform2i(uState->GetLocation(), v0, v1         ); }
	void GLSLProgramObject::SetUniform(UniformState* uState, float v0, float v1)                     { assert(IsBound()); if (uState->Set(v0, v1        )) glUniform2f(uState->GetLocation(), v0, v1         ); }
	void GLSLProgramObject::SetUniform(UniformState* uState, int   v0, int   v1, int   v2)           { assert(IsBound()); if (uState->Set(v0, v1, v2    )) glUniform3i(uState->GetLocation(), v0, v1, v2     ); }
	void GLSLProgramObject::SetUniform(UniformState* uState, float v0, float v1, float v2)           { assert(IsBound()); if (uState->Set(v0, v1, v2    )) glUniform3f(uState->GetLocation(), v0, v1, v2     ); }
	void GLSLProgramObject::SetUniform(UniformState* uState, int   v0, int   v1, int   v2, int   v3) { assert(IsBound()); if (uState->Set(v0, v1, v2, v3)) glUniform4i(uState->GetLocation(), v0, v1, v2, v3 ); }
	void GLSLProgramObject::SetUniform(UniformState* uState, float v0, float v1, float v2, float v3) { assert(IsBound()); if (uState->Set(v0, v1, v2, v3)) glUniform4f(uState->GetLocation(), v0, v1, v2, v3 ); }

	void GLSLProgramObject::SetUniform2v(UniformState* uState, const int*   v) { assert(IsBound()); if (uState->Set2v(v)) glUniform2iv(uState->GetLocation(), 1, v); }
	void GLSLProgramObject::SetUniform2v(UniformState* uState, const float* v) { assert(IsBound()); if (uState->Set2v(v)) glUniform2fv(uState->GetLocation(), 1, v); }
	void GLSLProgramObject::SetUniform3v(UniformState* uState, const int*   v) { assert(IsBound()); if (uState->Set3v(v)) glUniform3iv(uState->GetLocation(), 1, v); }
	void GLSLProgramObject::SetUniform3v(UniformState* uState, const float* v) { assert(IsBound()); if (uState->Set3v(v)) glUniform3fv(uState->GetLocation(), 1, v); }
	void GLSLProgramObject::SetUniform4v(UniformState* uState, const int*   v) { assert(IsBound()); if (uState->Set4v(v)) glUniform4iv(uState->GetLocation(), 1, v); }
	void GLSLProgramObject::SetUniform4v(UniformState* uState, const float* v) { assert(IsBound()); if (uState->Set4v(v)) glUniform4fv(uState->GetLocation(), 1, v); }

	void GLSLProgramObject::SetUniformMatrix2x2(UniformState* uState, bool transp, const float* v) { assert(IsBound()); if (uState->Set2x2(v, transp)) glUniformMatrix2fv(uState->GetLocation(), 1, transp, v); }
	void GLSLProgramObject::SetUniformMatrix3x3(UniformState* uState, bool transp, const float* v) { assert(IsBound()); if (uState->Set3x3(v, transp)) glUniformMatrix3fv(uState->GetLocation(), 1, transp, v); }
	void GLSLProgramObject::SetUniformMatrix4x4(UniformState* uState, bool transp, const float* v) { assert(IsBound()); if (uState->Set4x4(v, transp)) glUniformMatrix4fv(uState->GetLocation(), 1, transp, v); }

	void GLSLProgramObject::SetUniform1i(int idx, int   v0                              ) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0            )) glUniform1i(it->second.GetLocation(), v0            ); }
	void GLSLProgramObject::SetUniform2i(int idx, int   v0, int   v1                    ) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0, v1        )) glUniform2i(it->second.GetLocation(), v0, v1        ); }
	void GLSLProgramObject::SetUniform3i(int idx, int   v0, int   v1, int   v2          ) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0, v1, v2    )) glUniform3i(it->second.GetLocation(), v0, v1, v2    ); }
	void GLSLProgramObject::SetUniform4i(int idx, int   v0, int   v1, int   v2, int   v3) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0, v1, v2, v3)) glUniform4i(it->second.GetLocation(), v0, v1, v2, v3); }
	void GLSLProgramObject::SetUniform1f(int idx, float v0                              ) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0            )) glUniform1f(it->second.GetLocation(), v0            ); }
	void GLSLProgramObject::SetUniform2f(int idx, float v0, float v1                    ) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0, v1        )) glUniform2f(it->second.GetLocation(), v0, v1        ); }
	void GLSLProgramObject::SetUniform3f(int idx, float v0, float v1, float v2          ) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0, v1, v2    )) glUniform3f(it->second.GetLocation(), v0, v1, v2    ); }
	void GLSLProgramObject::SetUniform4f(int idx, float v0, float v1, float v2, float v3) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set(v0, v1, v2, v3)) glUniform4f(it->second.GetLocation(), v0, v1, v2, v3); }

	void GLSLProgramObject::SetUniform2iv(int idx, const int*   v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set2v(v)) glUniform2iv(it->second.GetLocation(), 1, v); }
	void GLSLProgramObject::SetUniform3iv(int idx, const int*   v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set3v(v)) glUniform3iv(it->second.GetLocation(), 1, v); }
	void GLSLProgramObject::SetUniform4iv(int idx, const int*   v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set4v(v)) glUniform4iv(it->second.GetLocation(), 1, v); }
	void GLSLProgramObject::SetUniform2fv(int idx, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set2v(v)) glUniform2fv(it->second.GetLocation(), 1, v); }
	void GLSLProgramObject::SetUniform3fv(int idx, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set3v(v)) glUniform3fv(it->second.GetLocation(), 1, v); }
	void GLSLProgramObject::SetUniform4fv(int idx, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set4v(v)) glUniform4fv(it->second.GetLocation(), 1, v); }

	void GLSLProgramObject::SetUniformMatrix2fv(int idx, bool transp, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set2x2(v, transp)) glUniformMatrix2fv(it->second.GetLocation(), 1, transp, v); }
	void GLSLProgramObject::SetUniformMatrix3fv(int idx, bool transp, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set3x3(v, transp)) glUniformMatrix3fv(it->second.GetLocation(), 1, transp, v); }
	void GLSLProgramObject::SetUniformMatrix4fv(int idx, bool transp, const float* v) { assert(IsBound()); auto it = uniformStates.find(uniformLocs[idx]); if (it != uniformStates.end() && it->second.Set4x4(v, transp)) glUniformMatrix4fv(it->second.GetLocation(), 1, transp, v); }
}
