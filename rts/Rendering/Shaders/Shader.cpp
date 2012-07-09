/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/Shader.h"

namespace Shader {
	ARBShaderObject::ARBShaderObject(int shType, const std::string& shSrc): IShaderObject(shType, shSrc) {
		if (IS_GL_FUNCTION_AVAILABLE(glGenProgramsARB)) {
			glGenProgramsARB(1, &objID);
		}
	}

	void ARBShaderObject::Compile() {
		glEnable(type);

		if (IS_GL_FUNCTION_AVAILABLE(glBindProgramARB)) {
			glBindProgramARB(type, objID);
			glProgramStringARB(type, GL_PROGRAM_FORMAT_ASCII_ARB, src.size(), src.c_str());

			int errorPos = -1;
			int isNative =  0;

			glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
			glGetProgramivARB(type, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isNative);

			const char* err = (const char*) glGetString(GL_PROGRAM_ERROR_STRING_ARB);

			log = std::string((err != NULL)? err: "[NULL]");
			valid = (errorPos == -1 && isNative != 0);

			glBindProgramARB(type, 0);
		}

		glDisable(type);
	}
	void ARBShaderObject::Release() {
		if (IS_GL_FUNCTION_AVAILABLE(glDeleteProgramsARB)) {
			glDeleteProgramsARB(1, &objID);
		}
	}



	GLSLShaderObject::GLSLShaderObject(int shType, const std::string& shSrc): IShaderObject(shType, shSrc) {
		if (IS_GL_FUNCTION_AVAILABLE(glCreateShader)) {
			objID = glCreateShader(type);
		}
	}
	void GLSLShaderObject::Compile() {
		char logStr[65536] = {0};
		int  logStrLen     = 0;

		const GLchar* srcStr = src.c_str();

		if (IS_GL_FUNCTION_AVAILABLE(glShaderSource)) {
			glShaderSource(objID, 1, &srcStr, NULL);
			glCompileShader(objID);

			GLint compiled = 0;

			glGetShaderiv(objID, GL_COMPILE_STATUS, &compiled);
			glGetShaderInfoLog(objID, 65536, &logStrLen, logStr);

			valid = bool(compiled);
			log   = std::string(logStr);
		}
	}

	void GLSLShaderObject::Release() {
		if (IS_GL_FUNCTION_AVAILABLE(glDeleteShader)) {
			glDeleteShader(objID);
		}
	}






	void IProgramObject::Release() {
		for (SOVecIt it = shaderObjs.begin(); it != shaderObjs.end(); ++it) {
			(const_cast<IShaderObject*>(*it))->Release();
			delete *it;
		}

		shaderObjs.clear();
	}



	ARBProgramObject::ARBProgramObject(): IProgramObject() {
		objID = -1; // not used for ARBProgramObject instances
		uniformTarget = -1;
	}

	void ARBProgramObject::Enable() const {
		for (SOVecIt it = shaderObjs.begin(); it != shaderObjs.end(); it++) {
			glEnable((*it)->GetType());
			glBindProgramARB((*it)->GetType(), (*it)->GetObjID());
		}
	}
	void ARBProgramObject::Disable() const {
		for (SOVecIt it = shaderObjs.begin(); it != shaderObjs.end(); it++) {
			glBindProgramARB((*it)->GetType(), 0);
			glDisable((*it)->GetType());
		}
	}

	void ARBProgramObject::Link() {
		bool shaderObjectsValid = true;

		for (SOVecIt it = shaderObjs.begin(); it != shaderObjs.end(); it++) {
			shaderObjectsValid = (shaderObjectsValid && (*it)->IsValid());
		}

		valid = shaderObjectsValid;
	}
	void ARBProgramObject::Release() {
		IProgramObject::Release();
	}

	#define glPEP4f  glProgramEnvParameter4fARB
	#define glPEP4fv glProgramEnvParameter4fvARB
	void ARBProgramObject::SetUniform1i(int idx, int   v0                              ) const { glPEP4f(uniformTarget, idx, float(v0), float( 0), float( 0), float( 0)); }
	void ARBProgramObject::SetUniform2i(int idx, int   v0, int   v1                    ) const { glPEP4f(uniformTarget, idx, float(v0), float(v1), float( 0), float( 0)); }
	void ARBProgramObject::SetUniform3i(int idx, int   v0, int   v1, int   v2          ) const { glPEP4f(uniformTarget, idx, float(v0), float(v1), float(v2), float( 0)); }
	void ARBProgramObject::SetUniform4i(int idx, int   v0, int   v1, int   v2, int   v3) const { glPEP4f(uniformTarget, idx, float(v0), float(v1), float(v2), float(v3)); }
	void ARBProgramObject::SetUniform1f(int idx, float v0                              ) const { glPEP4f(uniformTarget, idx, v0, 0.0f, 0.0f, 0.0f); }
	void ARBProgramObject::SetUniform2f(int idx, float v0, float v1                    ) const { glPEP4f(uniformTarget, idx, v0,   v1, 0.0f, 0.0f); }
	void ARBProgramObject::SetUniform3f(int idx, float v0, float v1, float v2          ) const { glPEP4f(uniformTarget, idx, v0,   v1,   v2, 0.0f); }
	void ARBProgramObject::SetUniform4f(int idx, float v0, float v1, float v2, float v3) const { glPEP4f(uniformTarget, idx, v0,   v1,   v2,   v3); }

	void ARBProgramObject::SetUniform2iv(int idx, const int*   v) const { int   vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] =    0; vv[3] =    0; glPEP4fv(uniformTarget, idx, (float*) vv); }
	void ARBProgramObject::SetUniform3iv(int idx, const int*   v) const { int   vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] =    0; glPEP4fv(uniformTarget, idx, (float*) vv); }
	void ARBProgramObject::SetUniform4iv(int idx, const int*   v) const { int   vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] = v[3]; glPEP4fv(uniformTarget, idx, (float*) vv); }
	void ARBProgramObject::SetUniform2fv(int idx, const float* v) const { float vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = 0.0f; vv[3] = 0.0f; glPEP4fv(uniformTarget, idx,          vv); }
	void ARBProgramObject::SetUniform3fv(int idx, const float* v) const { float vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] = 0.0f; glPEP4fv(uniformTarget, idx,          vv); }
	void ARBProgramObject::SetUniform4fv(int idx, const float* v) const { float vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] = v[3]; glPEP4fv(uniformTarget, idx,          vv); }
	#undef glPEP4f
	#undef glPEP4fv

	void ARBProgramObject::AttachShaderObject(const IShaderObject* so) {
		if (so != NULL) {
			IProgramObject::AttachShaderObject(so);
		}
	}



	GLSLProgramObject::GLSLProgramObject(): IProgramObject() {
		if (IS_GL_FUNCTION_AVAILABLE(glCreateProgram)) {
			objID = glCreateProgram();
		}
	}

	void GLSLProgramObject::Enable() const { glUseProgram(objID); }
	void GLSLProgramObject::Disable() const { glUseProgram(0); }

	void GLSLProgramObject::Link() {
		char logStr[65536] = {0};
		int  logStrLen     = 0;

		GLint linked = 0;

		if (IS_GL_FUNCTION_AVAILABLE(glLinkProgram)) {
			glLinkProgram(objID);
			glGetProgramInfoLog(objID, 65536, &logStrLen, logStr);
			glGetProgramiv(objID, GL_LINK_STATUS, &linked);

			// append the link-log
			log += std::string(logStr);
		}

		valid = bool(linked);
	}

	void GLSLProgramObject::Validate() {
		char logStr[65536] = {0};
		int  logStrLen     = 0;

		GLint validated = 0;

		if (IS_GL_FUNCTION_AVAILABLE(glValidateProgram)) {
			glValidateProgram(objID);
			glGetProgramInfoLog(objID, 65536, &logStrLen, logStr);
			glGetProgramiv(objID, GL_VALIDATE_STATUS, &validated);

			// append the validation-log
			log += std::string(logStr);
		}

		valid = valid && bool(validated);
	}

	void GLSLProgramObject::Release() {
		IProgramObject::Release();

		if (IS_GL_FUNCTION_AVAILABLE(glDeleteProgram)) {
			glDeleteProgram(objID);
		}
	}

	void GLSLProgramObject::AttachShaderObject(const IShaderObject* so) {
		if (so != NULL) {
			IProgramObject::AttachShaderObject(so);

			if (so->IsValid()) {
				glAttachShader(objID, so->GetObjID());
			}
		}
	}

	void GLSLProgramObject::SetUniformLocation(const std::string& name) {
		uniformLocs.push_back(glGetUniformLocation(objID, name.c_str()));
	}

	void GLSLProgramObject::SetUniform1i(int idx, int   v0                              ) const { glUniform1i(uniformLocs[idx], v0            ); }
	void GLSLProgramObject::SetUniform2i(int idx, int   v0, int   v1                    ) const { glUniform2i(uniformLocs[idx], v0, v1        ); }
	void GLSLProgramObject::SetUniform3i(int idx, int   v0, int   v1, int   v2          ) const { glUniform3i(uniformLocs[idx], v0, v1, v2    ); }
	void GLSLProgramObject::SetUniform4i(int idx, int   v0, int   v1, int   v2, int   v3) const { glUniform4i(uniformLocs[idx], v0, v1, v2, v3); }
	void GLSLProgramObject::SetUniform1f(int idx, float v0                              ) const { glUniform1f(uniformLocs[idx], v0            ); }
	void GLSLProgramObject::SetUniform2f(int idx, float v0, float v1                    ) const { glUniform2f(uniformLocs[idx], v0, v1        ); }
	void GLSLProgramObject::SetUniform3f(int idx, float v0, float v1, float v2          ) const { glUniform3f(uniformLocs[idx], v0, v1, v2    ); }
	void GLSLProgramObject::SetUniform4f(int idx, float v0, float v1, float v2, float v3) const { glUniform4f(uniformLocs[idx], v0, v1, v2, v3); }

	void GLSLProgramObject::SetUniform2iv(int idx, const int*   v) const { glUniform2iv(uniformLocs[idx], 1, v); }
	void GLSLProgramObject::SetUniform3iv(int idx, const int*   v) const { glUniform3iv(uniformLocs[idx], 1, v); }
	void GLSLProgramObject::SetUniform4iv(int idx, const int*   v) const { glUniform4iv(uniformLocs[idx], 1, v); }
	void GLSLProgramObject::SetUniform2fv(int idx, const float* v) const { glUniform2fv(uniformLocs[idx], 1, v); }
	void GLSLProgramObject::SetUniform3fv(int idx, const float* v) const { glUniform3fv(uniformLocs[idx], 1, v); }
	void GLSLProgramObject::SetUniform4fv(int idx, const float* v) const { glUniform4fv(uniformLocs[idx], 1, v); }

	void GLSLProgramObject::SetUniformMatrix2fv(int idx, bool transp, const float* v) const { glUniformMatrix2fv(uniformLocs[idx], 1, transp, v); }
	void GLSLProgramObject::SetUniformMatrix3fv(int idx, bool transp, const float* v) const { glUniformMatrix3fv(uniformLocs[idx], 1, transp, v); }
	void GLSLProgramObject::SetUniformMatrix4fv(int idx, bool transp, const float* v) const { glUniformMatrix4fv(uniformLocs[idx], 1, transp, v); }
	#ifdef glUniformMatrix2dv
	void GLSLProgramObject::SetUniformMatrix2dv(int idx, bool transp, const double* v) const { glUniformMatrix2dv(uniformLocs[idx], 1, transp, v); }
	#else
	void GLSLProgramObject::SetUniformMatrix2dv(int idx, bool transp, const double* v) const {}
	#endif
	#ifdef glUniformMatrix3dv
	void GLSLProgramObject::SetUniformMatrix3dv(int idx, bool transp, const double* v) const { glUniformMatrix3dv(uniformLocs[idx], 1, transp, v); }
	#else
	void GLSLProgramObject::SetUniformMatrix3dv(int idx, bool transp, const double* v) const {}
	#endif
	#ifdef glUniformMatrix4dv
	void GLSLProgramObject::SetUniformMatrix4dv(int idx, bool transp, const double* v) const { glUniformMatrix4dv(uniformLocs[idx], 1, transp, v); }
	#else
	void GLSLProgramObject::SetUniformMatrix4dv(int idx, bool transp, const double* v) const {}
	#endif

	void GLSLProgramObject::SetUniformMatrixArray4fv(int idx, int count, bool transp, const float* v) const { glUniformMatrix4fv(uniformLocs[idx], count, transp, v); }
	#ifdef glUniformMatrix4dv
	void GLSLProgramObject::SetUniformMatrixArray4dv(int idx, int count, bool transp, const double* v) const { glUniformMatrix4dv(uniformLocs[idx], count, transp, v); }
	#else
	void GLSLProgramObject::SetUniformMatrixArray4dv(int idx, int count, bool transp, const double* v) const {}
	#endif
}
