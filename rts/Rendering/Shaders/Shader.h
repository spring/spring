/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_SHADER_HDR
#define SPRING_SHADER_HDR

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "ShaderStates.h"
#include "lib/gml/gmlcnf.h"



constexpr size_t hashString(const char* str, size_t hash = 5381)
{
	return (*str) ? hashString(str + 1, hash + (hash << 5) + *str) : hash;
}

struct fast_hash : public std::unary_function<int, size_t>
{
        size_t operator()(const int a) const
        {
                return a;
        }
};

namespace Shader {
	struct IShaderObject {
	public:
		IShaderObject(unsigned int shType, const std::string& shSrcFile, const std::string& shSrcDefs = ""):
			objID(0), type(shType), valid(false), srcFile(shSrcFile), rawDefStrs(shSrcDefs) {
		}

		virtual ~IShaderObject() {}

		virtual void Compile(bool reloadFromDisk) {}
		virtual void Release() {}
		unsigned int GetObjID() const { return objID; }
		unsigned int GetType() const { return type; }
		bool IsValid() const { return valid; }
		const std::string& GetLog() const { return log; }

		void SetDefinitions(const std::string& defs) { modDefStrs = defs; }

	protected:
		unsigned int objID;
		unsigned int type;

		bool valid;

		std::string srcFile;
		std::string curShaderSrc;
		std::string modDefStrs;
		std::string rawDefStrs;
		std::string log;
	};

	struct NullShaderObject: public Shader::IShaderObject {
	public:
		NullShaderObject(unsigned int shType, const std::string& shSrcFile) : IShaderObject(shType, shSrcFile) {}
	};

	struct ARBShaderObject: public Shader::IShaderObject {
	public:
		ARBShaderObject(unsigned int, const std::string&, const std::string& shSrcDefs = "");
		void Compile(bool reloadFromDisk);
		void Release();
	};

	struct GLSLShaderObject: public Shader::IShaderObject {
	public:
		GLSLShaderObject(unsigned int, const std::string&, const std::string& shSrcDefs = "");
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

		void PrintInfo();

	public:
		int GetUniformLocation(const std::string& name) {
			return GetUniformState(name)->GetLocation();
		}
		UniformState* GetUniformState(const std::string& name) {
			return GetUniformState(hashString(name.c_str()), name);
		}
		UniformState* GetUniformState(const size_t hash, const std::string& name) {
			auto it = uniformStates.find(hash);
			if (it != uniformStates.end())
				return &it->second;
			//UniformState* us = &uniformStates.emplace(h, name).first->second;
			UniformState* us = &uniformStates.insert(std::pair<size_t, Shader::UniformState>(hash, Shader::UniformState(name))).first->second;
			us->SetLocation(GetUniformLoc(name));
			return us;
		}

	//private:
		virtual int GetUniformLoc(const std::string& name) = 0;

	//public:
		template<typename TK, typename TV> inline void SetUniform(const TK& name, TV v0) { SetUniform(GetUniformState(hashString(name), name), v0); }
		template<typename TK, typename TV> inline void SetUniform(const TK& name, TV v0, TV v1)  { SetUniform(GetUniformState(hashString(name), name), v0, v1); }
		template<typename TK, typename TV> inline void SetUniform(const TK& name, TV v0, TV v1, TV v2)  { SetUniform(GetUniformState(hashString(name), name), v0, v1, v2); }
		template<typename TK, typename TV> inline void SetUniform(const TK& name, TV v0, TV v1, TV v2, TV v3)  { SetUniform(GetUniformState(hashString(name), name), v0, v1, v2, v3); }

		template<typename TK, typename TV> inline void SetUniform2v(const TK& name, const TV* v) { SetUniform2v(GetUniformState(hashString(name), name), v); }
		template<typename TK, typename TV> inline void SetUniform3v(const TK& name, const TV* v) { SetUniform3v(GetUniformState(hashString(name), name), v); }
		template<typename TK, typename TV> inline void SetUniform4v(const TK& name, const TV* v) { SetUniform4v(GetUniformState(hashString(name), name), v); }

		template<typename TK, typename TV> inline void SetUniformMatrix2x2(const TK& name, bool transp, const TV* v) { SetUniformMatrix2x2(GetUniformState(hashString(name), name), transp, v); }
		template<typename TK, typename TV> inline void SetUniformMatrix3x3(const TK& name, bool transp, const TV* v) { SetUniformMatrix3x3(GetUniformState(hashString(name), name), transp, v); }
		template<typename TK, typename TV> inline void SetUniformMatrix4x4(const TK& name, bool transp, const TV* v) { SetUniformMatrix4x4(GetUniformState(hashString(name), name), transp, v); }


		virtual void SetUniform(UniformState* uState, int   v0) { SetUniform1i(uState->GetLocation(), v0); }
		virtual void SetUniform(UniformState* uState, float v0) { SetUniform1f(uState->GetLocation(), v0); }
		virtual void SetUniform(UniformState* uState, int   v0, int   v1) { SetUniform2i(uState->GetLocation(), v0, v1); }
		virtual void SetUniform(UniformState* uState, float v0, float v1) { SetUniform2f(uState->GetLocation(), v0, v1); }
		virtual void SetUniform(UniformState* uState, int   v0, int   v1, int   v2) { SetUniform3i(uState->GetLocation(), v0, v1, v2); }
		virtual void SetUniform(UniformState* uState, float v0, float v1, float v2) { SetUniform3f(uState->GetLocation(), v0, v1, v2); }
		virtual void SetUniform(UniformState* uState, int   v0, int   v1, int   v2, int   v3) { SetUniform4i(uState->GetLocation(), v0, v1, v2, v3); }
		virtual void SetUniform(UniformState* uState, float v0, float v1, float v2, float v3) { SetUniform4f(uState->GetLocation(), v0, v1, v2, v3); }

		virtual void SetUniform2v(UniformState* uState, const int*   v) { SetUniform2iv(uState->GetLocation(), v); }
		virtual void SetUniform2v(UniformState* uState, const float* v) { SetUniform2fv(uState->GetLocation(), v); }
		virtual void SetUniform3v(UniformState* uState, const int*   v) { SetUniform3iv(uState->GetLocation(), v); }
		virtual void SetUniform3v(UniformState* uState, const float* v) { SetUniform3fv(uState->GetLocation(), v); }
		virtual void SetUniform4v(UniformState* uState, const int*   v) { SetUniform4iv(uState->GetLocation(), v); }
		virtual void SetUniform4v(UniformState* uState, const float* v) { SetUniform4fv(uState->GetLocation(), v); }

		virtual void SetUniformMatrix2x2(UniformState* uState, bool transp, const float*  m) { SetUniformMatrix2fv(uState->GetLocation(), transp, m); }
		virtual void SetUniformMatrix3x3(UniformState* uState, bool transp, const float*  m) { SetUniformMatrix3fv(uState->GetLocation(), transp, m); }
		virtual void SetUniformMatrix4x4(UniformState* uState, bool transp, const float*  m) { SetUniformMatrix4fv(uState->GetLocation(), transp, m); }


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
		//virtual void SetUniformMatrixArray4fv(int idx, int count, bool transp, const float* v) {}

		//virtual void SetUniformMatrix2dv(int idx, bool transp, const double* v) {}
		//virtual void SetUniformMatrix3dv(int idx, bool transp, const double* v) {}
		//virtual void SetUniformMatrix4dv(int idx, bool transp, const double* v) {}
		//virtual void SetUniformMatrixArray4dv(int idx, int count, bool transp, const double* v) {}

		typedef std::vector<IShaderObject*> SOVec;
		typedef SOVec::iterator SOVecIt;
		typedef SOVec::const_iterator SOVecConstIt;

		virtual void AttachShaderObject(IShaderObject* so) { shaderObjs.push_back(so); }
		SOVec& GetAttachedShaderObjs() { return shaderObjs; }

		void RecompileIfNeeded();

		bool IsBound() const;
		bool IsShaderAttached(const IShaderObject* so) const;
		bool IsValid() const { return valid; }

		unsigned int GetObjID() const { return objID; }

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
	public:
		std::unordered_map<std::size_t, UniformState, fast_hash> uniformStates;
	};

	struct NullProgramObject: public Shader::IProgramObject {
	public:
		NullProgramObject(const std::string& poName): IProgramObject(poName) {}
		void Enable() {}
		void Disable() {}
		void Release() {}
		void Reload(bool reloadFromDisk) {}

		int GetUniformLoc(const std::string& name) { return -1; }

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

		int GetUniformLoc(const std::string& name);
		void SetUniformTarget(int target);
		int GetUnitformTarget();

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
#ifdef USE_GML
		int tuniformTargets[GML_MAX_NUM_THREADS];
#endif
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

		int GetUniformLoc(const std::string& name);
		void SetUniformLocation(const std::string&);

		void SetUniform(UniformState* uState, int   v0);
		void SetUniform(UniformState* uState, float v0);
		void SetUniform(UniformState* uState, int   v0, int   v1);
		void SetUniform(UniformState* uState, float v0, float v1);
		void SetUniform(UniformState* uState, int   v0, int   v1, int   v2);
		void SetUniform(UniformState* uState, float v0, float v1, float v2);
		void SetUniform(UniformState* uState, int   v0, int   v1, int   v2, int   v3);
		void SetUniform(UniformState* uState, float v0, float v1, float v2, float v3);

		void SetUniform2v(UniformState* uState, const int*   v);
		void SetUniform2v(UniformState* uState, const float* v);
		void SetUniform3v(UniformState* uState, const int*   v);
		void SetUniform3v(UniformState* uState, const float* v);
		void SetUniform4v(UniformState* uState, const int*   v);
		void SetUniform4v(UniformState* uState, const float* v);

		void SetUniformMatrix2x2(UniformState* uState, bool transp, const float*  v);
		//void SetUniformMatrix2x2(UniformState* uState, bool transp, const double* v);
		void SetUniformMatrix3x3(UniformState* uState, bool transp, const float*  v);
		//void SetUniformMatrix3x3(UniformState* uState, bool transp, const double* v);
		void SetUniformMatrix4x4(UniformState* uState, bool transp, const float*  v);
		//void SetUniformMatrix4x4(UniformState* uState, bool transp, const double* v);

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
		std::vector<size_t> uniformLocs;
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
