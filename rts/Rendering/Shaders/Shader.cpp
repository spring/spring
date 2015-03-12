/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/LuaShaderContainer.h"
#include "Rendering/Shaders/GLSLCopyState.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Lua/LuaMaterial.h"

#include "System/Util.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Sync/HsiehHash.h"
#include "System/Log/ILog.h"

#include <algorithm>
#ifdef DEBUG
	#include <string.h> // strncmp
#endif
#ifndef GL_INVALID_INDEX
	#define GL_INVALID_INDEX -1
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
	int maxLength;

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
		hash = HsiehHash((const void*)curShaderSrc.data(), curShaderSrc.size(), hash);
		hash = HsiehHash((const void*)modDefStrs.data(), modDefStrs.size(), hash);
		hash = HsiehHash((const void*)rawDefStrs.data(), rawDefStrs.size(), hash);
		return hash;
	}


	/*****************************************************************/

	ARBShaderObject::ARBShaderObject(
		unsigned int shType,
		const std::string& shSrc,
		const std::string& shSrcDefs
	): IShaderObject(shType, shSrc)
	{
		assert(globalRendering->haveARB); // non-debug check is done in ShaderHandler
		glGenProgramsARB(1, &objID);
	}

	void ARBShaderObject::Compile(bool reloadFromDisk) {
		glEnable(type);

		if (reloadFromDisk)
			curShaderSrc = GetShaderSource(srcFile);

		glBindProgramARB(type, objID);
		glProgramStringARB(type, GL_PROGRAM_FORMAT_ASCII_ARB, curShaderSrc.size(), curShaderSrc.c_str());

		int errorPos = -1;
		int isNative =  0;

		glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
		glGetProgramivARB(type, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isNative);

		const char* err = (const char*) glGetString(GL_PROGRAM_ERROR_STRING_ARB);

		log = std::string((err != NULL)? err: "[NULL]");
		valid = (errorPos == -1 && isNative != 0);

		glBindProgramARB(type, 0);
		glDisable(type);
	}

	void ARBShaderObject::Release() {
		glDeleteProgramsARB(1, &objID);
	}


	/*****************************************************************/

	GLSLShaderObject::GLSLShaderObject(
		unsigned int shType,
		const std::string& shSrcFile,
		const std::string& shSrcDefs
	): IShaderObject(shType, shSrcFile, shSrcDefs)
	{
		assert(globalRendering->haveGLSL); // non-debug check is done in ShaderHandler
	}

	void GLSLShaderObject::Compile(bool reloadFromDisk) {
		if (reloadFromDisk || curShaderSrc.empty())
			curShaderSrc = GetShaderSource(srcFile);

		assert(!curShaderSrc.empty());
		std::string sourceStr = curShaderSrc;
		std::string defFlags  = rawDefStrs + "\n" + modDefStrs;
		std::string versionStr;

		// extract #version pragma and put it on the first line (only allowed there)
		// version pragma in definitions overrides version pragma in source (if any)
		ExtractGlslVersion(&sourceStr, &versionStr);
		ExtractGlslVersion(&defFlags,  &versionStr);
		if (!versionStr.empty()) EnsureEndsWith(&versionStr, "\n");
		if (!defFlags.empty())   EnsureEndsWith(&defFlags,   "\n");

		// NOTE: many definition flags are not set until after shader is compiled
		std::vector<const GLchar*> sources = {
			"// SHADER VERSION\n",
			versionStr.c_str(),
			"// SHADER FLAGS\n",
			defFlags.c_str(),
			"// SHADER SOURCE\n",
			"#line 1\n",
			sourceStr.c_str()
		};

		if (objID == 0)
			objID = glCreateShader(type);

		glShaderSource(objID, sources.size(), &sources[0], NULL);
		glCompileShader(objID);

		valid = glslIsValid(objID);
		log   = glslGetLog(objID);

		if (!IsValid()) {
			const std::string& name = srcFile.find("void main()") ? "unknown" : srcFile;
			LOG_L(L_WARNING, "[GLSL-SO::%s] shader-object name: %s, compile-log:\n%s\n", __FUNCTION__, name.c_str(), log.c_str());
			LOG_L(L_WARNING, "\n%s%s%s%s%s%s%s", sources[0], sources[1], sources[2], sources[3], sources[4], sources[5], sources[6]);
		}
	}

	void GLSLShaderObject::Release() {
		glDeleteShader(objID);
		objID = 0;
	}


	/*****************************************************************/

	IProgramObject::IProgramObject(const std::string& poName): name(poName), objID(0), curFlagsHash(0), valid(false), bound(false) {
	}

	void IProgramObject::Enable() {
		bound = true;
	}

	void IProgramObject::Disable() {
		bound = false;
	}

	bool IProgramObject::IsBound() const {
		return bound;
	}

	void IProgramObject::Release() {
		for (IShaderObject*& so: shaderObjs) {
			so->Release();
			delete so;
		}

		uniformStates.clear();
		shaderObjs.clear();
		textures.clear();
		valid = false;
		log = "";
	}

	bool IProgramObject::IsShaderAttached(const IShaderObject* so) const {
		return (std::find(shaderObjs.begin(), shaderObjs.end(), so) != shaderObjs.end());
	}

	bool IProgramObject::LoadFromLua(const std::string& filename) {
		return Shader::LoadFromLua(this, filename);
	}

	void IProgramObject::RecompileIfNeeded()
	{
		if (GetHash() == curFlagsHash)
			return;

		// NOTE: this does not preserve the #version pragma
		const std::string definitionFlags = GetString();
		for (IShaderObject*& so: shaderObjs) {
			so->SetDefinitions(definitionFlags);
		}

		Reload(false);
		PrintInfo();
	}

	void IProgramObject::PrintInfo()
	{
		LOG_L(L_DEBUG, "Uniform States for program-object \"%s\":", name.c_str());
		LOG_L(L_DEBUG, "Defs:\n %s", GetString().c_str());
		LOG_L(L_DEBUG, "Uniforms:");
		for (const auto& p : uniformStates) {
			const bool curUsed = GetUniformLocation(p.second.GetName()) >= 0;
			if (p.second.IsUninit()) {
				LOG_L(L_DEBUG, "\t%s: uninitialized used=%i", (p.second.GetName()).c_str(), int(curUsed));
			} else {
				LOG_L(L_DEBUG, "\t%s: x=float:%f;int:%i y=%f z=%f used=%i", (p.second.GetName()).c_str(), p.second.GetFltValues()[0], p.second.GetIntValues()[0], p.second.GetFltValues()[1], p.second.GetFltValues()[2], int(curUsed));
			}
		}
	}

	UniformState* IProgramObject::GetNewUniformState(const std::string name)
	{
		const auto it = uniformStates.insert(std::make_pair(hashString(name.c_str()), UniformState(name)));
		// const auto it = uniformStates.emplace(hashString(name.c_str()), name);

		UniformState* us = &(it.first->second);
		us->SetLocation(GetUniformLoc(name));
	#if DEBUG
		if (us->IsLocationValid())
			us->SetType(GetUniformType(us->GetLocation()));
	#endif
		return us;
	}

	void IProgramObject::AddTextureBinding(const int index, const std::string& luaTexName)
	{
		textures[index] = luaTexName;
	}

	void IProgramObject::BindTextures() const
	{
		LuaMatTexture luaTex;
		for (auto& p: textures) {
			if (LuaOpenGLUtils::ParseTextureImage(nullptr, luaTex, p.second)) {
				glActiveTexture(GL_TEXTURE0 + p.first);
				luaTex.Bind();
			}
		}
		glActiveTexture(GL_TEXTURE0);
	}


	/*****************************************************************/

	ARBProgramObject::ARBProgramObject(const std::string& poName): IProgramObject(poName) {
		objID = -1; // not used for ARBProgramObject instances
		uniformTarget = -1;
	}

	void ARBProgramObject::SetUniformTarget(int target) {
		uniformTarget = target;
	}
	int ARBProgramObject::GetUnitformTarget() {
		return uniformTarget;
	}

	void ARBProgramObject::Enable() {
		RecompileIfNeeded();
		for (const IShaderObject* so: shaderObjs) {
			glEnable(so->GetType());
			glBindProgramARB(so->GetType(), so->GetObjID());
		}
		IProgramObject::Enable();
	}
	void ARBProgramObject::Disable() {
		for (const IShaderObject* so: shaderObjs) {
			glBindProgramARB(so->GetType(), 0);
			glDisable(so->GetType());
		}
		IProgramObject::Disable();
	}

	void ARBProgramObject::Link() {
		RecompileIfNeeded();
		bool shaderObjectsValid = true;

		for (const IShaderObject* so: shaderObjs) {
			shaderObjectsValid = (shaderObjectsValid && so->IsValid());
		}

		valid = shaderObjectsValid;
	}
	void ARBProgramObject::Release() {
		IProgramObject::Release();
	}
	void ARBProgramObject::Reload(bool reloadFromDisk) {
		for (IShaderObject* so: shaderObjs) {
			so->Compile(reloadFromDisk);
		}
	}

	int ARBProgramObject::GetUniformLoc(const std::string& name) {
		return -1; //FIXME
	}

	#define glPEP4f  glProgramEnvParameter4fARB
	#define glPEP4fv glProgramEnvParameter4fvARB
	void ARBProgramObject::SetUniform1i(int idx, int   v0                              ) { glPEP4f(uniformTarget, idx, float(v0), float( 0), float( 0), float( 0)); }
	void ARBProgramObject::SetUniform2i(int idx, int   v0, int   v1                    ) { glPEP4f(uniformTarget, idx, float(v0), float(v1), float( 0), float( 0)); }
	void ARBProgramObject::SetUniform3i(int idx, int   v0, int   v1, int   v2          ) { glPEP4f(uniformTarget, idx, float(v0), float(v1), float(v2), float( 0)); }
	void ARBProgramObject::SetUniform4i(int idx, int   v0, int   v1, int   v2, int   v3) { glPEP4f(uniformTarget, idx, float(v0), float(v1), float(v2), float(v3)); }
	void ARBProgramObject::SetUniform1f(int idx, float v0                              ) { glPEP4f(uniformTarget, idx, v0, 0.0f, 0.0f, 0.0f); }
	void ARBProgramObject::SetUniform2f(int idx, float v0, float v1                    ) { glPEP4f(uniformTarget, idx, v0,   v1, 0.0f, 0.0f); }
	void ARBProgramObject::SetUniform3f(int idx, float v0, float v1, float v2          ) { glPEP4f(uniformTarget, idx, v0,   v1,   v2, 0.0f); }
	void ARBProgramObject::SetUniform4f(int idx, float v0, float v1, float v2, float v3) { glPEP4f(uniformTarget, idx, v0,   v1,   v2,   v3); }

	void ARBProgramObject::SetUniform2iv(int idx, const int*   v) { int   vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] =    0; vv[3] =    0; glPEP4fv(uniformTarget, idx, (float*) vv); }
	void ARBProgramObject::SetUniform3iv(int idx, const int*   v) { int   vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] =    0; glPEP4fv(uniformTarget, idx, (float*) vv); }
	void ARBProgramObject::SetUniform4iv(int idx, const int*   v) { int   vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] = v[3]; glPEP4fv(uniformTarget, idx, (float*) vv); }
	void ARBProgramObject::SetUniform2fv(int idx, const float* v) { float vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = 0.0f; vv[3] = 0.0f; glPEP4fv(uniformTarget, idx,          vv); }
	void ARBProgramObject::SetUniform3fv(int idx, const float* v) { float vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] = 0.0f; glPEP4fv(uniformTarget, idx,          vv); }
	void ARBProgramObject::SetUniform4fv(int idx, const float* v) { float vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] = v[3]; glPEP4fv(uniformTarget, idx,          vv); }
	#undef glPEP4f
	#undef glPEP4fv

	void ARBProgramObject::AttachShaderObject(IShaderObject* so) {
		if (so != NULL) {
			IProgramObject::AttachShaderObject(so);
		}
	}


	/*****************************************************************/

	GLSLProgramObject::GLSLProgramObject(const std::string& poName): IProgramObject(poName), curSrcHash(0) {
		objID = glCreateProgram();
	}

	GLSLProgramObject::~GLSLProgramObject() {
		IProgramObject::Release();
		glDeleteProgram(objID);
		objID = 0;
	}

	void GLSLProgramObject::Enable() { RecompileIfNeeded(); glUseProgram(objID); IProgramObject::Enable(); }
	void GLSLProgramObject::Disable() { glUseProgram(0); IProgramObject::Disable(); }

	void GLSLProgramObject::Link() {
		RecompileIfNeeded();
		assert(glIsProgram(objID));

		if (!glIsProgram(objID))
			return;

		glLinkProgram(objID);

		valid = glslIsValid(objID);
		log += glslGetLog(objID);

		if (!IsValid()) {
			LOG_L(L_WARNING, "[GLSL-PO::%s] program-object name: %s, link-log:\n%s\n", __FUNCTION__, name.c_str(), log.c_str());
		}
	}

	void GLSLProgramObject::Validate() {
		GLint validated = 0;

		glValidateProgram(objID);
		glGetProgramiv(objID, GL_VALIDATE_STATUS, &validated);

		// append the validation-log
		log += glslGetLog(objID);

		valid = valid && bool(validated);

	#ifdef DEBUG
		GLsizei numUniforms, maxUniformNameLength;
		glGetProgramiv(objID, GL_ACTIVE_UNIFORMS, &numUniforms);
		glGetProgramiv(objID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformNameLength);

		if (maxUniformNameLength <= 0)
			return;

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

			const auto hash = hashString(&bufname[0]);
			auto it = uniformStates.find(hash);
			if (it != uniformStates.end())
				continue;

			LOG_L(L_WARNING, "[GLSL-PO::%s] program-object name: %s, unset uniform: %s", __FUNCTION__, name.c_str(), &bufname[0]);
			//assert(false);
		}
	#endif
	}

	void GLSLProgramObject::Release() {
		IProgramObject::Release();
		glDeleteProgram(objID);
		ClearHash();
		curFlagsHash = 0;
		objID = 0;
		objID = glCreateProgram();
		curSrcHash = 0;
	}

	void GLSLProgramObject::Reload(bool reloadFromDisk) {
		CShaderHandler::ShaderCache& shadersCache = shaderHandler->GetShaderCache();

		const auto   oldSrcHash = curSrcHash;
		const bool   oldValid  = IsValid();
		const GLuint oldProgID = objID;

		log = "";
		valid = false;

		// create shader source hash
		curFlagsHash = GetHash();
		curSrcHash = curFlagsHash;
		for (const IShaderObject* so: GetAttachedShaderObjs()) {
			curSrcHash ^= so->GetHash();
		}

		// clear all uniform locations
		for (auto& us_pair: uniformStates) {
			us_pair.second.SetLocation(GL_INVALID_INDEX);
		}

		// early-exit: empty program
		// TODO delete existing program if exists?
		if (GetAttachedShaderObjs().empty())
			return;

		// put old program into cache
		bool deleteOldShader = false;

		if (oldValid) {
			if ((deleteOldShader = !shadersCache.Push(oldSrcHash, oldProgID))) {
				for (IShaderObject*& so: GetAttachedShaderObjs()) {
					glDetachShader(oldProgID, so->GetObjID());
					so->Release();
				}
			}
		}

		// either read new program for cache or recompile if not found in it (id 0)
		objID = (!reloadFromDisk) ? shadersCache.Find(curSrcHash) : 0;
		if (objID == 0) {
			objID = glCreateProgram();
			for (IShaderObject*& so: GetAttachedShaderObjs()) {
				so->Compile(reloadFromDisk); //FIXME check if changed or not (when it did, we can't use shader cache!)

				if (so->IsValid()) {
					glAttachShader(objID, so->GetObjID());
				}
			}
			Link();
		} else {
			valid = true;
		}

		// copy full program state from old to new program (uniforms etc.)
		if (oldValid && IsValid()) {
			GLSLCopyState(objID, oldProgID, &((IProgramObject*)(this))->uniformStates);
		}

		// delete old program when not further used
		if (deleteOldShader)
			glDeleteProgram(oldProgID);
	}

	void GLSLProgramObject::AttachShaderObject(IShaderObject* so) {
		if (so == NULL)
			return;

		assert(!IsShaderAttached(so));
		IProgramObject::AttachShaderObject(so);

		if (so->IsValid()) {
			glAttachShader(objID, so->GetObjID());
		}
	}

	int GLSLProgramObject::GetUniformType(const int loc) {
		GLint size = 0;
		GLenum type = 0;
		glGetActiveUniform(objID, loc, 0, nullptr, &size, &type, nullptr);
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
	void GLSLProgramObject::SetUniformMatrixArray4fv(int idx, int count, bool transp, const float* v) { assert(IsBound()); /*glUniformMatrix4fv(it->second.GetLocation(), count, transp, v);*/ }

	#ifdef glUniformMatrix2dv
	void GLSLProgramObject::SetUniformMatrix2dv(int idx, bool transp, const double* v) { /*glUniformMatrix2dv(it->second.GetLocation(), 1, transp, v);*/ }
	#else
	void GLSLProgramObject::SetUniformMatrix2dv(int idx, bool transp, const double* v) {}
	#endif
	#ifdef glUniformMatrix3dv
	void GLSLProgramObject::SetUniformMatrix3dv(int idx, bool transp, const double* v) { /*glUniformMatrix3dv(it->second.GetLocation(), 1, transp, v);*/ }
	#else
	void GLSLProgramObject::SetUniformMatrix3dv(int idx, bool transp, const double* v) {}
	#endif
	#ifdef glUniformMatrix4dv
	void GLSLProgramObject::SetUniformMatrix4dv(int idx, bool transp, const double* v) { /*glUniformMatrix4dv(it->second.GetLocation(), 1, transp, v);*/ }
	#else
	void GLSLProgramObject::SetUniformMatrix4dv(int idx, bool transp, const double* v) {}
	#endif

	#ifdef glUniformMatrix4dv
	void GLSLProgramObject::SetUniformMatrixArray4dv(int idx, int count, bool transp, const double* v) { /*glUniformMatrix4dv(it->second.GetLocation(), count, transp, v);*/ }
	#else
	void GLSLProgramObject::SetUniformMatrixArray4dv(int idx, int count, bool transp, const double* v) {}
	#endif
}
