/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_SHADER_HDR
#define SPRING_SHADER_HDR

#include <algorithm>
#include <functional>
#include <string>
#include <memory>
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

		virtual void Compile() {}
		virtual void Release() {}

		bool ReloadFromDisk();
		bool IsValid() const { return valid; }

		unsigned int GetObjID() const { return objID; }
		unsigned int GetType() const { return type; }
		unsigned int GetHash() const;

		const std::string& GetLog() const { return log; }

		void SetDefinitions(const std::string& defs) { modDefStrs = defs; }

	protected:
		unsigned int objID;
		unsigned int type;

		bool valid;

		std::string srcFile;
		std::string srcText;
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
		GLSLShaderObject(unsigned int, const std::string&, const std::string& shSrcDefs = "");

		struct CompiledShaderObject {
			CompiledShaderObject() : id(0), valid(false) {}

			unsigned int id;
			bool      valid;
			std::string log;
		};

		/// @brief Returns a GLSL shader object in an unqiue pointer that auto deletes that instance.
		///        Quote of GL docs: If a shader object is deleted while it is attached to a program object,
		///        it will be flagged for deletion, and deletion will not occur until glDetachShader is called
		///        to detach it from all program objects to which it is attached.
		typedef std::unique_ptr<CompiledShaderObject, std::function<void(CompiledShaderObject* so)>> CompiledShaderObjectUniquePtr;
		CompiledShaderObjectUniquePtr CompileShaderObject();
	};




	struct IProgramObject {
	public:
		IProgramObject(const std::string& poName);
		virtual ~IProgramObject() {}

		void LoadFromID(unsigned int id) {
			objID = id;
			valid = (id != 0 && Validate());
			bound = false;

			// not needed for pre-compiled programs
			shaderObjs.clear();
		}

		/// create the whole shader from a lua file
		bool LoadFromLua(const std::string& filename);

		virtual void Enable() { bound = true; }
		virtual void Disable() { bound = false; }
		virtual void Link() = 0;
		virtual bool Validate() = 0;
		virtual void Release() = 0;
		virtual void Reload(bool reloadFromDisk, bool validate) = 0;
		/// attach single shader objects (vertex, frag, ...) to the program
		virtual void AttachShaderObject(IShaderObject* so) { shaderObjs.push_back(so); }

		bool IsBound() const { return bound; }
		bool IsValid() const { return valid; }
		bool IsShaderAttached(const IShaderObject* so) const {
			return (std::find(shaderObjs.begin(), shaderObjs.end(), so) != shaderObjs.end());
		}

		unsigned int GetObjID() const { return objID; }

		const std::string& GetName() const { return name; }
		const std::string& GetLog() const { return log; }

		const std::vector<IShaderObject*>& GetAttachedShaderObjs() const { return shaderObjs; }
		      std::vector<IShaderObject*>& GetAttachedShaderObjs()       { return shaderObjs; }

		void RecompileIfNeeded(bool validate);
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
		std::string name;
		std::string log;

		unsigned int objID;

		bool valid;
		bool bound;

		std::vector<IShaderObject*> shaderObjs;

		ShaderFlags shaderFlags;

	public:
		spring::unsynced_map<std::size_t, UniformState, fast_hash> uniformStates;
		spring::unsynced_map<int, LuaMatTexture> luaTextures;
	};


	struct NullProgramObject: public Shader::IProgramObject {
	public:
		NullProgramObject(const std::string& poName): IProgramObject(poName) {}

		void Enable() {}
		void Disable() {}
		void Release() {}
		void Reload(bool reloadFromDisk, bool validate) {}
		bool Validate() { return true; }
		void Link() {}

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
		GLSLProgramObject(const std::string& poName);
		~GLSLProgramObject() { Release(); }

		void Enable();
		void Disable();
		void Link();
		bool Validate();
		void Release();
		void Reload(bool reloadFromDisk, bool validate);

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
		unsigned int curSrcHash;
	};


	extern NullShaderObject* nullShaderObject;
	extern NullProgramObject* nullProgramObject;
}

#endif
