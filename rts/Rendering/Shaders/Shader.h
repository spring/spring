/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_SHADER_HDR
#define SPRING_SHADER_HDR

#include <string>
#include <vector>
#include "ShaderStates.h"
#include "lib/gml/gmlcnf.h"


namespace Shader {
	struct IShaderObject {
	public:
		IShaderObject(int shType, const std::string& shSrcFile, const std::string& shDefinitions = ""):
			objID(0), type(shType), valid(false), srcFile(shSrcFile), definitions2(shDefinitions) {
		}
		virtual ~IShaderObject() {
		}

		virtual void Compile(bool reloadFromDisk) {}
		virtual void Release() {}
		unsigned int GetObjID() const { return objID; }
		int GetType() const { return type; }
		bool IsValid() const { return valid; }
		const std::string& GetLog() const { return log; }
		void SetDefinitions(const std::string& defs) { definitions = defs; }

	protected:
		unsigned int objID;
		int type;
		bool valid;

		std::string srcFile;
		std::string curShaderSrc;
		std::string definitions;
		std::string definitions2;
		std::string log;
	};

	struct NullShaderObject: public Shader::IShaderObject {
	public:
		NullShaderObject(int shType, const std::string& shSrcFile) : IShaderObject(shType, shSrcFile) {}
	};
	
	struct ARBShaderObject: public Shader::IShaderObject {
	public:
		ARBShaderObject(int, const std::string&, const std::string& shDefinitions = "");
		void Compile(bool reloadFromDisk);
		void Release();
	};

	struct GLSLShaderObject: public Shader::IShaderObject {
	public:
		GLSLShaderObject(int, const std::string&, const std::string& shDefinitions = "");
		void Compile(bool reloadFromDisk);
		void Release();
	};




	struct IProgramObject : public SShaderFlagState {
	public:
		IProgramObject(const std::string& poName);
		virtual ~IProgramObject() {}

		virtual void Enable();
		virtual void Disable();
		virtual void Link() {}
		virtual void Validate() {}
		virtual void Release() = 0;
		virtual void Reload(bool reloadFromDisk) = 0;
		void RecompileIfNeeded();
		bool IsBound() const;

		virtual void SetUniformTarget(int) {}
		virtual void SetUniformLocation(const std::string&) {}

		virtual void SetUniform1i(int idx, int   v0) = 0;
		virtual void SetUniform2i(int idx, int   v0, int   v1) = 0;
		virtual void SetUniform3i(int idx, int   v0, int   v1, int   v2) = 0;
		virtual void SetUniform4i(int idx, int   v0, int   v1, int   v2, int   v3) = 0;
		virtual void SetUniform1f(int idx, float v0) = 0;
		virtual void SetUniform2f(int idx, float v0, float v1) = 0;
		virtual void SetUniform3f(int idx, float v0, float v1, float v2) = 0;
		virtual void SetUniform4f(int idx, float v0, float v1, float v2, float v3) = 0;

		virtual void SetUniform2iv(int idx, const int*   v) = 0;
		virtual void SetUniform3iv(int idx, const int*   v) = 0;
		virtual void SetUniform4iv(int idx, const int*   v) = 0;
		virtual void SetUniform2fv(int idx, const float* v) = 0;
		virtual void SetUniform3fv(int idx, const float* v) = 0;
		virtual void SetUniform4fv(int idx, const float* v) = 0;

		virtual void SetUniformMatrix2fv(int idx, bool transp, const float* v) {}
		virtual void SetUniformMatrix3fv(int idx, bool transp, const float* v) {}
		virtual void SetUniformMatrix4fv(int idx, bool transp, const float* v) {}
		virtual void SetUniformMatrixArray4fv(int idx, int count, bool transp, const float* v) {}

		virtual void SetUniformMatrix2dv(int idx, bool transp, const double* v) {}
		virtual void SetUniformMatrix3dv(int idx, bool transp, const double* v) {}
		virtual void SetUniformMatrix4dv(int idx, bool transp, const double* v) {}
		virtual void SetUniformMatrixArray4dv(int idx, int count, bool transp, const double* v) {}

		typedef std::vector<IShaderObject*> SOVec;
		typedef SOVec::iterator SOVecIt;
		typedef SOVec::const_iterator SOVecConstIt;

		virtual void AttachShaderObject(IShaderObject* so) { shaderObjs.push_back(so); }
		SOVec& GetAttachedShaderObjs() { return shaderObjs; }
		bool IsShaderAttached(const IShaderObject* so) const;

		unsigned int GetObjID() const { return objID; }
		bool IsValid() const { return valid; }
		const std::string& GetLog() const { return log; }

	protected:
		std::string name;
		std::string log;

		unsigned int objID;
		unsigned int curHash;

		bool valid;
		bool bound;
#ifdef USE_GML
		char tbound[GML_MAX_NUM_THREADS];
#endif

		SOVec shaderObjs;
	};

	struct NullProgramObject: public Shader::IProgramObject {
	public:
		NullProgramObject(const std::string& poName): IProgramObject(poName) {}
		void Enable() {}
		void Disable() {}
		void Release() {}
		void Reload(bool reloadFromDisk) {}

		void SetUniform1i(int idx, int   v0) {}
		void SetUniform2i(int idx, int   v0, int   v1) {}
		void SetUniform3i(int idx, int   v0, int   v1, int   v2) {}
		void SetUniform4i(int idx, int   v0, int   v1, int   v2, int   v3) {}
		void SetUniform1f(int idx, float v0) {}
		void SetUniform2f(int idx, float v0, float v1) {}
		void SetUniform3f(int idx, float v0, float v1, float v2) {}
		void SetUniform4f(int idx, float v0, float v1, float v2, float v3) {}

		void SetUniform2iv(int idx, const int*   v) {}
		void SetUniform3iv(int idx, const int*   v) {}
		void SetUniform4iv(int idx, const int*   v) {}
		void SetUniform2fv(int idx, const float* v) {}
		void SetUniform3fv(int idx, const float* v) {}
		void SetUniform4fv(int idx, const float* v) {}
	};

	struct ARBProgramObject: public Shader::IProgramObject {
	public:
		ARBProgramObject(const std::string& poName);
		void Enable();
		void Disable();
		void Link();
		void Release();
		void Reload(bool reloadFromDisk);

		void SetUniformTarget(int target) { uniformTarget = target; }

		void SetUniform1i(int idx, int   v0);
		void SetUniform2i(int idx, int   v0, int   v1);
		void SetUniform3i(int idx, int   v0, int   v1, int   v2);
		void SetUniform4i(int idx, int   v0, int   v1, int   v2, int   v3);
		void SetUniform1f(int idx, float v0);
		void SetUniform2f(int idx, float v0, float v1);
		void SetUniform3f(int idx, float v0, float v1, float v2);
		void SetUniform4f(int idx, float v0, float v1, float v2, float v3);

		void SetUniform2iv(int idx, const int*   v);
		void SetUniform3iv(int idx, const int*   v);
		void SetUniform4iv(int idx, const int*   v);
		void SetUniform2fv(int idx, const float* v);
		void SetUniform3fv(int idx, const float* v);
		void SetUniform4fv(int idx, const float* v);

		void AttachShaderObject(IShaderObject*);

	private:
		int uniformTarget;
	};

	struct GLSLProgramObject: public Shader::IProgramObject {
	public:
		GLSLProgramObject(const std::string& poName);
		void Enable();
		void Disable();
		void Link();
		void Validate();
		void Release();
		void Reload(bool reloadFromDisk);

		void SetUniformLocation(const std::string&);

		void SetUniform1i(int idx, int   v0);
		void SetUniform2i(int idx, int   v0, int   v1);
		void SetUniform3i(int idx, int   v0, int   v1, int   v2);
		void SetUniform4i(int idx, int   v0, int   v1, int   v2, int   v3);
		void SetUniform1f(int idx, float v0);
		void SetUniform2f(int idx, float v0, float v1);
		void SetUniform3f(int idx, float v0, float v1, float v2);
		void SetUniform4f(int idx, float v0, float v1, float v2, float v3);

		void SetUniform2iv(int idx, const int*   v);
		void SetUniform3iv(int idx, const int*   v);
		void SetUniform4iv(int idx, const int*   v);
		void SetUniform2fv(int idx, const float* v);
		void SetUniform3fv(int idx, const float* v);
		void SetUniform4fv(int idx, const float* v);

		void SetUniformMatrix2fv(int idx, bool transp, const float* v);
		void SetUniformMatrix3fv(int idx, bool transp, const float* v);
		void SetUniformMatrix4fv(int idx, bool transp, const float* v);
		void SetUniformMatrixArray4fv(int idx, int count, bool transp, const float* v);

		void SetUniformMatrix2dv(int idx, bool transp, const double* v);
		void SetUniformMatrix3dv(int idx, bool transp, const double* v);
		void SetUniformMatrix4dv(int idx, bool transp, const double* v);
		void SetUniformMatrixArray4dv(int idx, int count, bool transp, const double* v);

		void AttachShaderObject(IShaderObject*);

	private:
		std::vector<int> uniformLocs;
		std::vector<UniformState> uniformStates;
	};

	/*
	struct GLSLARBProgramObject: public Shader::IProgramObject {
		glCreateProgramObjectARB <==> glCreateProgram
		glCreateShaderObjectARB  <==> glCreateShader
		glDeleteObjectARB        <==> glDelete{Shader,Program}
		glCompileShaderARB       <==> glCompileShader
		glShaderSourceARB        <==> glShaderSource
		glAttachObjectARB        <==> glAttachShader
		glDetachObjectARB        <==> glDetachShader
		glLinkProgramARB         <==> glLinkProgram
		glUseProgramObjectARB    <==> glUseProgram
		glUniform*ARB            <==> glUniform*
		glGetUniformLocationARB  <==> glGetUniformLocation
	};
	*/
	
	extern NullShaderObject* nullShaderObject;
	extern NullProgramObject* nullProgramObject;
}

#endif
