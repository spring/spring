/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"

#include "Rendering/Shaders/Shader.hpp"

namespace Shader {
	ARBShaderObject::ARBShaderObject(int shType, const std::string& shSrc): IShaderObject(shType, shSrc) {
		if (glGenProgramsARB != NULL) {
			glGenProgramsARB(1, &objID);
		}
	}

	void ARBShaderObject::Compile() {
		glEnable(type);
		glBindProgramARB(type, objID);
		glProgramStringARB(type, GL_PROGRAM_FORMAT_ASCII_ARB, src.size(), src.c_str());

		int errorPos = -1;
		int isNative =  0;

		glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
		glGetProgramivARB(type, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isNative);

		log = std::string((const char*) glGetString(GL_PROGRAM_ERROR_STRING_ARB));
		valid = (errorPos == -1 && isNative != 0);

		glBindProgramARB(type, 0);
		glDisable(type);
	}
	void ARBShaderObject::Release() {
		glDeleteProgramsARB(1, &objID);
	}



	GLSLShaderObject::GLSLShaderObject(int shType, const std::string& shSrc): IShaderObject(shType, shSrc) {
		if (glCreateShader != NULL) {
			objID = glCreateShader(type);
		}
	}
	void GLSLShaderObject::Compile() {
		char logStr[65536] = {0};
		int  logStrLen     = 0;

		const GLchar* srcStr = src.c_str();
		GLint compiled = 0;

		glShaderSource(objID, 1, &srcStr, NULL);
		glCompileShader(objID);

		glGetShaderiv(objID, GL_COMPILE_STATUS, &compiled);
		glGetShaderInfoLog(objID, 65536, &logStrLen, logStr);

		valid = bool(compiled);
		log   = std::string(logStr);
	}

	void GLSLShaderObject::Release() {
		glDeleteShader(objID);
	}






	void IProgramObject::Release() {
		for (SOVecIt it = shaderObjs.begin(); it != shaderObjs.end(); it++) {
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
	void ARBProgramObject::SetUniform1i(int idx, int   v0                              ) { glPEP4f(uniformTarget, idx, float(v0), float( 0), float( 0), float( 0)); }
	void ARBProgramObject::SetUniform2i(int idx, int   v0, int   v1                    ) { glPEP4f(uniformTarget, idx, float(v0), float(v1), float( 0), float( 0)); }
	void ARBProgramObject::SetUniform3i(int idx, int   v0, int   v1, int   v2          ) { glPEP4f(uniformTarget, idx, float(v0), float(v1), float(v2), float( 0)); }
	void ARBProgramObject::SetUniform4i(int idx, int   v0, int   v1, int   v2, int   v3) { glPEP4f(uniformTarget, idx, float(v0), float(v1), float(v2), float(v3)); }
	void ARBProgramObject::SetUniform1f(int idx, float v0                              ) { glPEP4f(uniformTarget, idx, v0, 0.0f, 0.0f, 0.0f); }
	void ARBProgramObject::SetUniform2f(int idx, float v0, float v1                    ) { glPEP4f(uniformTarget, idx, v0,   v1, 0.0f, 0.0f); }
	void ARBProgramObject::SetUniform3f(int idx, float v0, float v1, float v2          ) { glPEP4f(uniformTarget, idx, v0,   v1,   v2, 0.0f); }
	void ARBProgramObject::SetUniform4f(int idx, float v0, float v1, float v2, float v3) { glPEP4f(uniformTarget, idx, v0,   v1,   v2,   v3); }

	void ARBProgramObject::SetUniform2iv(int idx, int*   v) { int   vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] =    0; vv[3] =    0; glPEP4fv(uniformTarget, idx, (float*) vv); }
	void ARBProgramObject::SetUniform3iv(int idx, int*   v) { int   vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] =    0; glPEP4fv(uniformTarget, idx, (float*) vv); }
	void ARBProgramObject::SetUniform4iv(int idx, int*   v) { int   vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] = v[3]; glPEP4fv(uniformTarget, idx, (float*) vv); }
	void ARBProgramObject::SetUniform2fv(int idx, float* v) { float vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = 0.0f; vv[3] = 0.0f; glPEP4fv(uniformTarget, idx,          vv); }
	void ARBProgramObject::SetUniform3fv(int idx, float* v) { float vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] = 0.0f; glPEP4fv(uniformTarget, idx,          vv); }
	void ARBProgramObject::SetUniform4fv(int idx, float* v) { float vv[4]; vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] = v[3]; glPEP4fv(uniformTarget, idx,          vv); }
	#undef glPEP4f
	#undef glPEP4fv

	void ARBProgramObject::AttachShaderObject(const IShaderObject* so) {
		if (so != NULL) {
			IProgramObject::AttachShaderObject(so);
		}
	}



	GLSLProgramObject::GLSLProgramObject(): IProgramObject() {
		if (glCreateProgram != NULL) {
			objID = glCreateProgram();
		}
	}

	void GLSLProgramObject::Enable() const { glUseProgram(objID); }
	void GLSLProgramObject::Disable() const { glUseProgram(0); }

	void GLSLProgramObject::Link() {
		char logStr[65536] = {0};
		int  logStrLen     = 0;

		GLint linked    = 0;
		GLint validated = 0;

		glLinkProgram(objID);
		glGetProgramInfoLog(objID, 65536, &logStrLen, logStr);
		// append the link-log
		log += std::string(logStr);

		glValidateProgram(objID);
		glGetProgramInfoLog(objID, 65536, &logStrLen, logStr);
		// append the validation-log
		log += std::string(logStr);

		glGetProgramiv(objID, GL_LINK_STATUS,     &linked);
		glGetProgramiv(objID, GL_VALIDATE_STATUS, &validated);

		valid = bool(linked) && bool(validated);
	}

	void GLSLProgramObject::Release() {
		IProgramObject::Release();
		glDeleteProgram(objID);
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

	void GLSLProgramObject::SetUniform1i(int idx, int   v0                              ) { glUniform1i(uniformLocs[idx], v0            ); }
	void GLSLProgramObject::SetUniform2i(int idx, int   v0, int   v1                    ) { glUniform2i(uniformLocs[idx], v0, v1        ); }
	void GLSLProgramObject::SetUniform3i(int idx, int   v0, int   v1, int   v2          ) { glUniform3i(uniformLocs[idx], v0, v1, v2    ); }
	void GLSLProgramObject::SetUniform4i(int idx, int   v0, int   v1, int   v2, int   v3) { glUniform4i(uniformLocs[idx], v0, v1, v2, v3); }
	void GLSLProgramObject::SetUniform1f(int idx, float v0                              ) { glUniform1f(uniformLocs[idx], v0            ); }
	void GLSLProgramObject::SetUniform2f(int idx, float v0, float v1                    ) { glUniform2f(uniformLocs[idx], v0, v1        ); }
	void GLSLProgramObject::SetUniform3f(int idx, float v0, float v1, float v2          ) { glUniform3f(uniformLocs[idx], v0, v1, v2    ); }
	void GLSLProgramObject::SetUniform4f(int idx, float v0, float v1, float v2, float v3) { glUniform4f(uniformLocs[idx], v0, v1, v2, v3); }

	void GLSLProgramObject::SetUniform2iv(int idx, int*   v) { glUniform2iv(uniformLocs[idx], 1, v); }
	void GLSLProgramObject::SetUniform3iv(int idx, int*   v) { glUniform3iv(uniformLocs[idx], 1, v); }
	void GLSLProgramObject::SetUniform4iv(int idx, int*   v) { glUniform4iv(uniformLocs[idx], 1, v); }
	void GLSLProgramObject::SetUniform2fv(int idx, float* v) { glUniform2fv(uniformLocs[idx], 1, v); }
	void GLSLProgramObject::SetUniform3fv(int idx, float* v) { glUniform3fv(uniformLocs[idx], 1, v); }
	void GLSLProgramObject::SetUniform4fv(int idx, float* v) { glUniform4fv(uniformLocs[idx], 1, v); }

	void GLSLProgramObject::SetUniformMatrix2fv(int idx, bool transp, float* v) { glUniformMatrix2fv(uniformLocs[idx], 1, transp, v); }
	void GLSLProgramObject::SetUniformMatrix3fv(int idx, bool transp, float* v) { glUniformMatrix3fv(uniformLocs[idx], 1, transp, v); }
	void GLSLProgramObject::SetUniformMatrix4fv(int idx, bool transp, float* v) { glUniformMatrix4fv(uniformLocs[idx], 1, transp, v); }

	#define M22(m, v)                           \
		m[0] = float(v[0]); m[1] = float(v[1]); \
		m[2] = float(v[2]); m[3] = float(v[3]);
	#define M33(m, v)                                               \
		m[0] = float(v[0]); m[1] = float(v[1]); m[2] = float(v[2]); \
		m[3] = float(v[3]); m[4] = float(v[4]); m[5] = float(v[5]); \
		m[6] = float(v[6]); m[7] = float(v[7]); m[8] = float(v[8]);
	#define M44(m, v)                                                                           \
		m[ 0] = float(v[ 0]); m[ 1] = float(v[ 1]); m[ 2] = float(v[ 2]); m[ 3] = float(v[ 3]); \
		m[ 4] = float(v[ 4]); m[ 5] = float(v[ 5]); m[ 6] = float(v[ 6]); m[ 7] = float(v[ 7]); \
		m[ 8] = float(v[ 8]); m[ 9] = float(v[ 9]); m[10] = float(v[10]); m[11] = float(v[11]); \
		m[12] = float(v[12]); m[13] = float(v[13]); m[14] = float(v[14]); m[15] = float(v[15]);
	void GLSLProgramObject::SetUniformMatrix2dv(int idx, bool transp, double* v) { float m[2 * 2]; M22(m, v); SetUniformMatrix2fv(idx, transp, m); }
	void GLSLProgramObject::SetUniformMatrix3dv(int idx, bool transp, double* v) { float m[3 * 3]; M33(m, v); SetUniformMatrix3fv(idx, transp, m); }
	void GLSLProgramObject::SetUniformMatrix4dv(int idx, bool transp, double* v) { float m[4 * 4]; M44(m, v); SetUniformMatrix4fv(idx, transp, m); }
	#undef M22
	#undef M33
	#undef M44
}
