/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_SHADER_HDR
#define SPRING_SHADER_HDR

#include <string>
#include <vector>

namespace Shader {
	struct IShaderObject {
	public:
		IShaderObject(int shType, const std::string& shSrc):
			objID(0), type(shType), valid(false), src(shSrc) {
		}
		virtual ~IShaderObject() {
		}

		virtual void Compile() {}
		virtual void Release() {}
		unsigned int GetObjID() const { return objID; }
		int GetType() const { return type; }
		bool IsValid() const { return valid; }
		const std::string& GetLog() const { return log; }

	protected:
		unsigned int objID;
		int type;
		bool valid;

		std::string src;
		std::string log;
	};


	struct ARBShaderObject: public Shader::IShaderObject {
	public:
		ARBShaderObject(int, const std::string&);
		void Compile();
		void Release();
	};

	struct GLSLShaderObject: public Shader::IShaderObject {
	public:
		GLSLShaderObject(int, const std::string&);
		void Compile();
		void Release();
	};




	struct IProgramObject {
	public:
		IProgramObject(): objID(0), valid(false) {}
		virtual ~IProgramObject() {}

		virtual void Enable() const = 0;
		virtual void Disable() const = 0;
		virtual void Link() {}
		virtual void Validate() {}
		virtual void Release() = 0;

		virtual void SetUniformTarget(int) {}
		virtual void SetUniformLocation(const std::string&) {}

		virtual void SetUniform1i(int idx, int   v0) const = 0;
		virtual void SetUniform2i(int idx, int   v0, int   v1) const = 0;
		virtual void SetUniform3i(int idx, int   v0, int   v1, int   v2) const = 0;
		virtual void SetUniform4i(int idx, int   v0, int   v1, int   v2, int   v3) const = 0;
		virtual void SetUniform1f(int idx, float v0) const = 0;
		virtual void SetUniform2f(int idx, float v0, float v1) const = 0;
		virtual void SetUniform3f(int idx, float v0, float v1, float v2) const = 0;
		virtual void SetUniform4f(int idx, float v0, float v1, float v2, float v3) const = 0;

		virtual void SetUniform2iv(int idx, const int*   v) const = 0;
		virtual void SetUniform3iv(int idx, const int*   v) const = 0;
		virtual void SetUniform4iv(int idx, const int*   v) const = 0;
		virtual void SetUniform2fv(int idx, const float* v) const = 0;
		virtual void SetUniform3fv(int idx, const float* v) const = 0;
		virtual void SetUniform4fv(int idx, const float* v) const = 0;

		virtual void SetUniformMatrix2fv(int idx, bool transp, const float* v) const {}
		virtual void SetUniformMatrix3fv(int idx, bool transp, const float* v) const {}
		virtual void SetUniformMatrix4fv(int idx, bool transp, const float* v) const {}
		virtual void SetUniformMatrix2dv(int idx, bool transp, const double* v) const {}
		virtual void SetUniformMatrix3dv(int idx, bool transp, const double* v) const {}
		virtual void SetUniformMatrix4dv(int idx, bool transp, const double* v) const {}

		virtual void SetUniformMatrixArray4fv(int idx, int count, bool transp, const float* v) const {}
		virtual void SetUniformMatrixArray4dv(int idx, int count, bool transp, const double* v) const {}

		typedef std::vector<const IShaderObject*> SOVec;
		typedef std::vector<const IShaderObject*>::const_iterator SOVecIt;

		virtual void AttachShaderObject(const IShaderObject* so) { shaderObjs.push_back(so); }
		const std::vector<const IShaderObject*>& GetAttachedShaderObjs() const { return shaderObjs; }

		unsigned int GetObjID() const { return objID; }
		bool IsValid() const { return valid; }
		const std::string& GetLog() const { return log; }

	protected:
		unsigned int objID;
		bool valid;

		std::string log;
		std::vector<const IShaderObject*> shaderObjs;
	};


	struct ARBProgramObject: public Shader::IProgramObject {
	public:
		ARBProgramObject();
		void Enable() const;
		void Disable() const;
		void Link();
		void Release();

		void SetUniformTarget(int target) { uniformTarget = target; }

		void SetUniform1i(int idx, int   v0) const;
		void SetUniform2i(int idx, int   v0, int   v1) const;
		void SetUniform3i(int idx, int   v0, int   v1, int   v2) const;
		void SetUniform4i(int idx, int   v0, int   v1, int   v2, int   v3) const;
		void SetUniform1f(int idx, float v0) const;
		void SetUniform2f(int idx, float v0, float v1) const;
		void SetUniform3f(int idx, float v0, float v1, float v2) const;
		void SetUniform4f(int idx, float v0, float v1, float v2, float v3) const;

		void SetUniform2iv(int idx, const int*   v) const;
		void SetUniform3iv(int idx, const int*   v) const;
		void SetUniform4iv(int idx, const int*   v) const;
		void SetUniform2fv(int idx, const float* v) const;
		void SetUniform3fv(int idx, const float* v) const;
		void SetUniform4fv(int idx, const float* v) const;

		void AttachShaderObject(const IShaderObject*);

	private:
		int uniformTarget;
	};

	struct GLSLProgramObject: public Shader::IProgramObject {
	public:
		GLSLProgramObject();
		void Enable() const;
		void Disable() const;
		void Link();
		void Validate();
		void Release();

		void SetUniformLocation(const std::string&);

		void SetUniform1i(int idx, int   v0) const;
		void SetUniform2i(int idx, int   v0, int   v1) const;
		void SetUniform3i(int idx, int   v0, int   v1, int   v2) const;
		void SetUniform4i(int idx, int   v0, int   v1, int   v2, int   v3) const;
		void SetUniform1f(int idx, float v0) const;
		void SetUniform2f(int idx, float v0, float v1) const;
		void SetUniform3f(int idx, float v0, float v1, float v2) const;
		void SetUniform4f(int idx, float v0, float v1, float v2, float v3) const;

		void SetUniform2iv(int idx, const int*   v) const;
		void SetUniform3iv(int idx, const int*   v) const;
		void SetUniform4iv(int idx, const int*   v) const;
		void SetUniform2fv(int idx, const float* v) const;
		void SetUniform3fv(int idx, const float* v) const;
		void SetUniform4fv(int idx, const float* v) const;

		void SetUniformMatrix2fv(int idx, bool transp, const float* v) const;
		void SetUniformMatrix3fv(int idx, bool transp, const float* v) const;
		void SetUniformMatrix4fv(int idx, bool transp, const float* v) const;
		void SetUniformMatrix2dv(int idx, bool transp, const double* v) const;
		void SetUniformMatrix3dv(int idx, bool transp, const double* v) const;
		void SetUniformMatrix4dv(int idx, bool transp, const double* v) const;

		void SetUniformMatrixArray4fv(int idx, int count, bool transp, const float* v) const;
		void SetUniformMatrixArray4dv(int idx, int count, bool transp, const double* v) const;

		void AttachShaderObject(const IShaderObject*);

	private:
		std::vector<int> uniformLocs;
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
}

#endif
