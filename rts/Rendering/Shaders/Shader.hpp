/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_SHADER_HDR
#define SPRING_SHADER_HDR

#include <string>
#include <vector>

namespace Shader {
	struct IShaderObject {
	public:
		IShaderObject(int shType = -1, const std::string& shSrc = ""):
			objID(0), type(shType), valid(false), src(shSrc), log("") {
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
		IProgramObject(): objID(0), valid(false), log("") {}
		virtual ~IProgramObject() {}

		virtual void Enable() const {}
		virtual void Disable() const {}
		virtual void Link() {}
		virtual void Release();

		virtual void SetUniformTarget(int) {}
		virtual void SetUniformLocation(const std::string&) {}

		virtual void SetUniform1i(int, int) {}
		virtual void SetUniform2i(int, int, int) {}
		virtual void SetUniform3i(int, int, int, int) {}
		virtual void SetUniform4i(int, int, int, int, int) {}
		virtual void SetUniform1f(int, float) {}
		virtual void SetUniform2f(int, float, float) {}
		virtual void SetUniform3f(int, float, float, float) {}
		virtual void SetUniform4f(int, float, float, float, float) {}

		virtual void SetUniform2iv(int, int*) {}
		virtual void SetUniform3iv(int, int*) {}
		virtual void SetUniform4iv(int, int*) {}
		virtual void SetUniform2fv(int, float*) {}
		virtual void SetUniform3fv(int, float*) {}
		virtual void SetUniform4fv(int, float*) {}

		virtual void SetUniformMatrix2fv(int, bool, float*) {}
		virtual void SetUniformMatrix3fv(int, bool, float*) {}
		virtual void SetUniformMatrix4fv(int, bool, float*) {}
		virtual void SetUniformMatrix2dv(int, bool, double*) {}
		virtual void SetUniformMatrix3dv(int, bool, double*) {}
		virtual void SetUniformMatrix4dv(int, bool, double*) {}

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

		void SetUniform1i(int, int);
		void SetUniform2i(int, int, int);
		void SetUniform3i(int, int, int, int);
		void SetUniform4i(int, int, int, int, int);
		void SetUniform1f(int, float);
		void SetUniform2f(int, float, float);
		void SetUniform3f(int, float, float, float);
		void SetUniform4f(int, float, float, float, float);

		void SetUniform2iv(int, int*);
		void SetUniform3iv(int, int*);
		void SetUniform4iv(int, int*);
		void SetUniform2fv(int, float*);
		void SetUniform3fv(int, float*);
		void SetUniform4fv(int, float*);

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
		void Release();

		void SetUniformLocation(const std::string&);

		void SetUniform1i(int, int);
		void SetUniform2i(int, int, int);
		void SetUniform3i(int, int, int, int);
		void SetUniform4i(int, int, int, int, int);
		void SetUniform1f(int, float);
		void SetUniform2f(int, float, float);
		void SetUniform3f(int, float, float, float);
		void SetUniform4f(int, float, float, float, float);

		void SetUniform2iv(int, int*);
		void SetUniform3iv(int, int*);
		void SetUniform4iv(int, int*);
		void SetUniform2fv(int, float*);
		void SetUniform3fv(int, float*);
		void SetUniform4fv(int, float*);

		void SetUniformMatrix2fv(int, bool, float*);
		void SetUniformMatrix3fv(int, bool, float*);
		void SetUniformMatrix4fv(int, bool, float*);
		void SetUniformMatrix2dv(int, bool, double*);
		void SetUniformMatrix3dv(int, bool, double*);
		void SetUniformMatrix4dv(int, bool, double*);

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
