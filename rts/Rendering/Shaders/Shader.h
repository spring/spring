/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_SHADER_HDR
#define SPRING_SHADER_HDR

#include <algorithm>
#include <string>
#include <vector>

#include "ShaderStates.h"
#include "Lua/LuaOpenGLUtils.h"
#include "System/UnorderedMap.hpp"



constexpr size_t hashString(const char* str, size_t hash = 5381)
{
	return (*str) ? hashString(str + 1, hash + (hash << 5) + *str) : hash;
}

struct fast_hash : public std::unary_function<int, size_t>
{
	size_t operator()(const int a) const { return a; }
};

namespace Shader {
	struct IShaderObject {
	public:
		IShaderObject(unsigned int shType, const std::string& shSrcData, const std::string& shSrcDefs = ""):
			type(shType), srcData(shSrcData), rawDefStrs(shSrcDefs) {
		}

		virtual ~IShaderObject() {}

		bool ReloadFromTextOrFile();
		bool IsValid() const { return valid; }

		unsigned int GetObjID() const { return glid; }
		unsigned int GetType() const { return type; }
		unsigned int GetHash() const;

		const std::string& GetLog() const { return log; }

		void SetDefinitions(const std::string& defs) { modDefStrs = defs; }

	protected:
		unsigned int glid = 0;
		unsigned int type = 0;

		bool valid = false;

		std::string srcData; // text or file
		std::string srcText; // equal to srcData if text, otherwise read from file
		std::string rawDefStrs; // set via constructor only, constant
		std::string modDefStrs; // set on reload from changed flags
		std::string log;
	};

	struct NullShaderObject: public Shader::IShaderObject {
	public:
		NullShaderObject(unsigned int shType, const std::string& shSrcFile) : IShaderObject(shType, shSrcFile) {}
	};

	struct GLSLShaderObject: public Shader::IShaderObject {
	public:
		GLSLShaderObject(
			unsigned int shType,
			const std::string& shSrcData,
			const std::string& shSrcDefs = ""
		): IShaderObject(shType, shSrcData, shSrcDefs) {}


		struct CompiledShaderObject {
		public:
			CompiledShaderObject(): id(0), valid(false) {}
			CompiledShaderObject(const CompiledShaderObject& cso) = delete;
			CompiledShaderObject(CompiledShaderObject&& cso) { *this = std::move(cso); }
			~CompiledShaderObject();

			CompiledShaderObject& operator = (const CompiledShaderObject& cso) = delete;
			CompiledShaderObject& operator = (CompiledShaderObject&& cso) {
				// destination CSO must not be valid yet
				id = cso.id;
				cso.id = 0;

				valid = cso.valid;
			}

		public:
			unsigned int id;
			bool      valid;
		};

		CompiledShaderObject CreateAndCompileShaderObject(std::string& programLog);
	};




	struct IProgramObject {
	public:
		IProgramObject() { LoadFromID(0); }
		IProgramObject(const std::string& poName): name(poName) {}
		IProgramObject(const IProgramObject& po) = delete;
		IProgramObject(IProgramObject&& po) { *this = std::move(po); }
		virtual ~IProgramObject() {}

		IProgramObject& operator = (const IProgramObject& po) = delete;
		IProgramObject& operator = (IProgramObject&& po) {
			glid = po.glid;
			hash = po.hash;

			valid = po.valid;
			bound = po.bound;

			name = std::move(po.name);
			log = std::move(po.log);

			shaderObjs = std::move(po.shaderObjs);
			shaderFlags = std::move(po.shaderFlags);

			uniformStates = std::move(po.uniformStates);
			luaTextures = std::move(po.luaTextures);

			// invalidate old
			po.LoadFromID(0);
			return *this;
		}

		void LoadFromID(unsigned int _glid) {
			glid = _glid;
			hash = 0;

			valid = (glid != 0 && Validate());
			bound = false;

			// objs are not needed for pre-compiled programs
			ClearAttachedShaderObjects();
		}

		/// create the whole shader from a lua file
		bool LoadFromLua(const std::string& filename);

		virtual void Enable() { bound = true; }
		virtual void Disable() { bound = false; }
		virtual bool CreateAndLink() = 0;
		virtual bool ValidateAndCopyUniforms(unsigned int tgtProgID, unsigned int srcProgID, bool validate) = 0;
		virtual void Link() = 0;
		virtual bool Validate() = 0;
		virtual void Release(bool deleteShaderObjs = true) = 0;
		virtual void Reload(bool force, bool validate) = 0;
		virtual bool ReloadState(bool reloadShaderObjs) = 0;

		/// attach single shader objects (vertex, frag, ...) to the program
		void AttachShaderObject(IShaderObject* so) { shaderObjs.push_back(so); }
		void ClearAttachedShaderObjects() { shaderObjs.clear(); }

		bool IsBound() const { return bound; }
		bool IsValid() const { return valid; }

		unsigned int GetObjID() const { return glid; }

		const std::string& GetName() const { return name; }
		const std::string& GetLog() const { return log; }

		void MaybeReload(bool validate);
		void PrintDebugInfo();

	public:
		/// new interface
		template<typename TK, typename TV> inline void SetUniform(const TK& name, TV v0) { SetUniform(GetUniformState(name), v0); }
		template<typename TK, typename TV> inline void SetUniform(const TK& name, TV v0, TV v1)  { SetUniform(GetUniformState(name), v0, v1); }
		template<typename TK, typename TV> inline void SetUniform(const TK& name, TV v0, TV v1, TV v2)  { SetUniform(GetUniformState(name), v0, v1, v2); }
		template<typename TK, typename TV> inline void SetUniform(const TK& name, TV v0, TV v1, TV v2, TV v3)  { SetUniform(GetUniformState(name), v0, v1, v2, v3); }

		template<typename TK, typename TV> inline void SetUniform2v(const TK& name, const TV* v) { SetUniform2v(GetUniformState(name), v); }
		template<typename TK, typename TV> inline void SetUniform3v(const TK& name, const TV* v) { SetUniform3v(GetUniformState(name), v); }
		template<typename TK, typename TV> inline void SetUniform4v(const TK& name, const TV* v) { SetUniform4v(GetUniformState(name), v); }

		template<typename TK, typename TV> inline void SetUniformMatrix2x2(const TK& name, bool transp, const TV* v) { SetUniformMatrix2x2(GetUniformState(name), transp, v); }
		template<typename TK, typename TV> inline void SetUniformMatrix3x3(const TK& name, bool transp, const TV* v) { SetUniformMatrix3x3(GetUniformState(name), transp, v); }
		template<typename TK, typename TV> inline void SetUniformMatrix4x4(const TK& name, bool transp, const TV* v) { SetUniformMatrix4x4(GetUniformState(name), transp, v); }

		template<typename TV> void SetFlag(const char* key, TV val) { shaderFlags.Set(key, val); }


		/// old interface
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

	public:
		/// interface to auto-bind textures with the shader
		void AddTextureBinding(const int texUnit, const std::string& luaTexName);
		void BindTextures() const;

	protected:
		/// internal
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

		virtual void SetUniformMatrix2x2(UniformState* uState, bool transp, const float* m) { SetUniformMatrix2fv(uState->GetLocation(), transp, m); }
		virtual void SetUniformMatrix3x3(UniformState* uState, bool transp, const float* m) { SetUniformMatrix3fv(uState->GetLocation(), transp, m); }
		virtual void SetUniformMatrix4x4(UniformState* uState, bool transp, const float* m) { SetUniformMatrix4fv(uState->GetLocation(), transp, m); }

	protected:
		int GetUniformLocation(const std::string& name) { return GetUniformState(name)->GetLocation(); }

	private:
		virtual int GetUniformLoc(const std::string& name) = 0;
		virtual int GetUniformType(const int idx) = 0;

		UniformState* GetNewUniformState(const std::string name);
		UniformState* GetUniformState(const std::string& name) {
			const auto hash = hashString(name.c_str()); // never compiletime const (std::string is never a literal)
			const auto iter = uniformStates.find(hash);
			if (iter != uniformStates.end())
				return &iter->second;
			return GetNewUniformState(name);
		}
		UniformState* GetUniformState(const char* name) {
			// (when inlined) hash might be compiletime const cause of constexpr of hashString
			// WARNING: Cause of a bug in gcc, you _must_ assign the constexpr to a var before
			//          passing it to a function. I.e. foo.find(hashString(name)) would always
			//          be runtime evaluated (even when `name` is a literal)!
			const auto hash = hashString(name);
			const auto iter = uniformStates.find(hash);
			if (iter != uniformStates.end())
				return &iter->second;
			return GetNewUniformState(name);
		}

	protected:
		unsigned int glid = 0; // GL identifier
		unsigned int hash = 0; // hash-code

		bool valid = false;
		bool bound = false;

		std::string name;
		std::string log;

		std::vector<IShaderObject*> shaderObjs;

		ShaderFlags shaderFlags;

	public:
		spring::unsynced_map<std::size_t, UniformState, fast_hash> uniformStates;
		spring::unsynced_map<int, LuaMatTexture> luaTextures;
	};



	struct NullProgramObject: public Shader::IProgramObject {
	public:
		NullProgramObject(const std::string& poName): IProgramObject(poName) {}
		NullProgramObject(const NullProgramObject& po) = delete;
		NullProgramObject(NullProgramObject&& po) = delete; // never moved

		NullProgramObject& operator = (const NullProgramObject& po) = delete;
		NullProgramObject& operator = (NullProgramObject&& po) = delete;

		void Enable() {}
		void Disable() {}
		bool CreateAndLink() { return false; }
		bool ValidateAndCopyUniforms(unsigned int tgtProgID, unsigned int srcProgID, bool validate) { return false; }
		void Link() {}
		void Release(bool deleteShaderObjs = true) {}
		void Reload(bool force, bool validate) {}
		bool ReloadState(bool reloadShaderObjs) {}
		bool Validate() { return true; }

		int GetUniformLoc(const std::string& name) { return -1; }
		int GetUniformType(const int idx) { return -1; }

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



	struct GLSLProgramObject: public Shader::IProgramObject {
	public:
		GLSLProgramObject(): IProgramObject() {}
		GLSLProgramObject(const std::string& poName); // NOTE: calls glCreateProgram
		GLSLProgramObject(const GLSLProgramObject&) = delete;
		GLSLProgramObject(GLSLProgramObject&& po) { *this = std::move(po); }
		~GLSLProgramObject() { Release(); }

		GLSLProgramObject& operator = (const GLSLProgramObject& po) = delete;
		GLSLProgramObject& operator = (GLSLProgramObject&& po) {
			uniformLocs = std::move(po.uniformLocs);

			IProgramObject::operator = (std::move(po));
			return *this;
		}

		void Enable();
		void Disable();
		bool CreateAndLink();
		bool ValidateAndCopyUniforms(unsigned int tgtProgID, unsigned int srcProgID, bool validate);
		bool ValidateAndCopyUniforms(unsigned int srcProgID, bool validate) { return (ValidateAndCopyUniforms(glid, srcProgID, validate)); }
		void Link(); // NOTE: calls MaybeReload (i.e. Reload which compiles *and* links)
		bool Validate();
		void Release(bool deleteShaderObjs = true);
		void Reload(bool force, bool validate);
		bool ReloadState(bool reloadShaderObjs);

		void ClearUniformLocations();
		void SetShaderDefinitions(const std::string& defs);
		void ReloadShaderObjects();
		void RecalculateShaderHash();

	public:
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

	private:
		int GetUniformType(const int idx);
		int GetUniformLoc(const std::string& name);

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
		void SetUniformMatrix3x3(UniformState* uState, bool transp, const float*  v);
		void SetUniformMatrix4x4(UniformState* uState, bool transp, const float*  v);

	private:
		std::vector<size_t> uniformLocs;
	};


	extern NullShaderObject* nullShaderObject;
	extern NullProgramObject* nullProgramObject;
}

#endif
