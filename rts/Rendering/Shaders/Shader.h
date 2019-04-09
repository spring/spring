/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_SHADER_HDR
#define SPRING_SHADER_HDR

#include <algorithm>
#include <string>
#include <vector>

#include "ShaderStates.h"
#include "Lua/LuaOpenGLUtils.h"
#include "System/UnorderedMap.hpp"
#include "System/StringHash.h"


struct fast_hash : public std::unary_function<int, size_t>
{
	size_t operator()(const int a) const { return a; }
};

namespace Shader {
	std::string GetShaderSource(const std::string& srcData);
	std::string GetShaderVersionDirective(std::string& srcText);


	// uniform or attribute
	struct ShaderInput {
		uint32_t index;
		uint32_t count;
		uint32_t type;
		uint32_t stride;

		const char* name;
		const void* data;
	};


	struct IShaderObject {
	public:
		IShaderObject(unsigned int shType, const std::string& shSrcData, const std::string& shSrcDefs):
			type(shType), srcData(shSrcData), rawDefStrs(shSrcDefs) {
		}

		virtual ~IShaderObject() {}

		bool ReloadFromTextOrFile();
		bool IsValid() const { return valid; }

		unsigned int GetObjID() const { return glid; }
		unsigned int GetType() const { return type; }
		unsigned int GetHash() const;

		const std::string& GetSrc(bool text) const { return (text? srcText: srcData); }
		const std::string& GetLog() const { return log; }

		void SetSourceString(const std::string& src, bool text) { (text? srcText: srcData) = src; }
		void SetDefineStrings(const std::string& defs) { modDefStrs = defs; }

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
		NullShaderObject(unsigned int shType, const std::string& shSrcFile) : IShaderObject(shType, shSrcFile, "") {}
	};

	struct GLSLShaderObject: public Shader::IShaderObject {
	public:
		GLSLShaderObject(
			unsigned int shType,
			const std::string& shSrcData,
			const std::string& shSrcDefs
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
				return *this;
			}

		public:
			unsigned int id;
			bool      valid;
		};

		CompiledShaderObject CreateAndCompileShaderObject(std::string& programLog);
	};




	struct IProgramObject {
	public:
		IProgramObject() {
			shaderObjs.reserve(4);
			uniformStates.reserve(32);
		}
		IProgramObject(const std::string& poName): name(poName) {
			shaderObjs.reserve(4);
			uniformStates.reserve(32);
		}
		IProgramObject(const IProgramObject& po) = delete;
		IProgramObject(IProgramObject&& po) { *this = std::move(po); }
		virtual ~IProgramObject() {}

		IProgramObject& operator = (const IProgramObject& po) = delete;
		IProgramObject& operator = (IProgramObject&& po) {
			std::swap(glid, po.glid);
			std::swap(hash, po.hash);

			std::swap(valid, po.valid);
			std::swap(bound, po.bound);

			name = std::move(po.name);
			log = std::move(po.log);

			shaderObjs = std::move(po.shaderObjs);
			shaderFlags = std::move(po.shaderFlags);

			uniformStates = std::move(po.uniformStates);
			luaTextures = std::move(po.luaTextures);

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
		virtual void EnableRaw() {}
		virtual void DisableRaw() {}
		virtual bool CreateAndLink() = 0;
		virtual bool CopyUniformsAndValidate(unsigned int tgtProgID, unsigned int srcProgID) = 0;
		virtual void Link() = 0;
		virtual bool Validate() = 0;
		virtual void Release(bool deleteShaderObjs = true) = 0;
		virtual void Reload(bool force, bool linking) = 0;
		virtual bool ReloadState(bool reloadShaderObjs) = 0;

		/// attach single shader objects (vertex, frag, ...) to the program
		void AttachShaderObject(IShaderObject* so) { shaderObjs.push_back(so); }
		void ClearAttachedShaderObjects() { shaderObjs.clear(); }

		bool IsBound() const { return bound; }
		bool IsValid() const { return valid; }

		unsigned int GetObjID() const { return glid; }
		unsigned int GetHash() const { return hash; }

		const std::vector<IShaderObject*>& GetShaderObjs() const { return shaderObjs; }
		const std::string& GetName() const { return name; }
		const std::string& GetLog() const { return log; }

		void MaybeReload(bool linking);
		void PrintDebugInfo();

		/// interface to auto-bind textures with the shader
		void AddTextureBinding(const int texUnit, const std::string& luaTexName);
		void BindTextures() const;

	public:
		virtual int GetUniformLoc(const char* name) const { return -1; }
		virtual int GetUniformType(int idx) const { return -1; }

		virtual void SetUniform(const ShaderInput& u) {}

		/// new (slower) interface
		template<typename TV> void SetUniform(const char* name, TV v0                     ) { SetUniform(GetOrAddUniformState(name), v0            ); }
		template<typename TV> void SetUniform(const char* name, TV v0, TV v1              ) { SetUniform(GetOrAddUniformState(name), v0, v1        ); }
		template<typename TV> void SetUniform(const char* name, TV v0, TV v1, TV v2       ) { SetUniform(GetOrAddUniformState(name), v0, v1, v2    ); }
		template<typename TV> void SetUniform(const char* name, TV v0, TV v1, TV v2, TV v3) { SetUniform(GetOrAddUniformState(name), v0, v1, v2, v3); }

		template<typename TV> void SetUniform2v(const char* name, const TV* v) { SetUniform2v(GetOrAddUniformState(name), v); }
		template<typename TV> void SetUniform3v(const char* name, const TV* v) { SetUniform3v(GetOrAddUniformState(name), v); }
		template<typename TV> void SetUniform4v(const char* name, const TV* v) { SetUniform4v(GetOrAddUniformState(name), v); }

		template<typename TV> void SetUniform2v(const char* name, int cnt, const TV* v) { SetUniform2v(GetOrAddUniformState(name), v, cnt); }
		template<typename TV> void SetUniform3v(const char* name, int cnt, const TV* v) { SetUniform3v(GetOrAddUniformState(name), v, cnt); }
		template<typename TV> void SetUniform4v(const char* name, int cnt, const TV* v) { SetUniform4v(GetOrAddUniformState(name), v, cnt); }

		template<typename TV> void SetUniformMatrix2x2(const char* name, bool transp, const TV* v) { SetUniformMatrix2x2(GetOrAddUniformState(name), v, 1, transp); }
		template<typename TV> void SetUniformMatrix3x3(const char* name, bool transp, const TV* v) { SetUniformMatrix3x3(GetOrAddUniformState(name), v, 1, transp); }
		template<typename TV> void SetUniformMatrix4x4(const char* name, bool transp, const TV* v) { SetUniformMatrix4x4(GetOrAddUniformState(name), v, 1, transp); }

		template<typename TV> void SetFlag(const char* key, TV val) { shaderFlags.Set(key, val); }


		/// old interface
		virtual int SetUniformLocation(const char*) { return -1; }

		virtual void SetUniform1i(int idx,   int v0                              ) {}
		virtual void SetUniform2i(int idx,   int v0,   int v1                    ) {}
		virtual void SetUniform3i(int idx,   int v0,   int v1,   int v2          ) {}
		virtual void SetUniform4i(int idx,   int v0,   int v1,   int v2,   int v3) {}
		virtual void SetUniform1f(int idx, float v0                              ) {}
		virtual void SetUniform2f(int idx, float v0, float v1                    ) {}
		virtual void SetUniform3f(int idx, float v0, float v1, float v2          ) {}
		virtual void SetUniform4f(int idx, float v0, float v1, float v2, float v3) {}

		virtual void SetUniform2iv(int idx,          const   int* v) {}
		virtual void SetUniform2iv(int idx, int cnt, const   int* v) {}
		virtual void SetUniform3iv(int idx,          const   int* v) {}
		virtual void SetUniform3iv(int idx, int cnt, const   int* v) {}
		virtual void SetUniform4iv(int idx,          const   int* v) {}
		virtual void SetUniform4iv(int idx, int cnt, const   int* v) {}
		virtual void SetUniform2fv(int idx,          const float* v) {}
		virtual void SetUniform2fv(int idx, int cnt, const float* v) {}
		virtual void SetUniform3fv(int idx,          const float* v) {}
		virtual void SetUniform3fv(int idx, int cnt, const float* v) {}
		virtual void SetUniform4fv(int idx,          const float* v) {}
		virtual void SetUniform4fv(int idx, int cnt, const float* v) {}

		virtual void SetUniformMatrix2fv(int idx,          bool transp, const float* v) {}
		virtual void SetUniformMatrix2fv(int idx, int cnt, bool transp, const float* v) {}
		virtual void SetUniformMatrix3fv(int idx,          bool transp, const float* v) {}
		virtual void SetUniformMatrix3fv(int idx, int cnt, bool transp, const float* v) {}
		virtual void SetUniformMatrix4fv(int idx,          bool transp, const float* v) {}
		virtual void SetUniformMatrix4fv(int idx, int cnt, bool transp, const float* v) {}

	protected:
		/// internal; used by the templates above
		virtual void SetUniform(UniformState* us,   int v0                              ) { SetUniform1i(us->GetLocation(), v0            ); }
		virtual void SetUniform(UniformState* us,   int v0,   int v1                    ) { SetUniform2i(us->GetLocation(), v0, v1        ); }
		virtual void SetUniform(UniformState* us,   int v0,   int v1,   int v2          ) { SetUniform3i(us->GetLocation(), v0, v1, v2    ); }
		virtual void SetUniform(UniformState* us,   int v0,   int v1,   int v2,   int v3) { SetUniform4i(us->GetLocation(), v0, v1, v2, v3); }
		virtual void SetUniform(UniformState* us, float v0                              ) { SetUniform1f(us->GetLocation(), v0            ); }
		virtual void SetUniform(UniformState* us, float v0, float v1                    ) { SetUniform2f(us->GetLocation(), v0, v1        ); }
		virtual void SetUniform(UniformState* us, float v0, float v1, float v2          ) { SetUniform3f(us->GetLocation(), v0, v1, v2    ); }
		virtual void SetUniform(UniformState* us, float v0, float v1, float v2, float v3) { SetUniform4f(us->GetLocation(), v0, v1, v2, v3); }

		virtual void SetUniform2v(UniformState* us, const   int* v, int cnt = 1) { SetUniform2iv(us->GetLocation(), cnt, v); }
		virtual void SetUniform3v(UniformState* us, const   int* v, int cnt = 1) { SetUniform3iv(us->GetLocation(), cnt, v); }
		virtual void SetUniform4v(UniformState* us, const   int* v, int cnt = 1) { SetUniform4iv(us->GetLocation(), cnt, v); }
		virtual void SetUniform2v(UniformState* us, const float* v, int cnt = 1) { SetUniform2fv(us->GetLocation(), cnt, v); }
		virtual void SetUniform3v(UniformState* us, const float* v, int cnt = 1) { SetUniform3fv(us->GetLocation(), cnt, v); }
		virtual void SetUniform4v(UniformState* us, const float* v, int cnt = 1) { SetUniform4fv(us->GetLocation(), cnt, v); }

		virtual void SetUniformMatrix2x2(UniformState* us, const float* m, int cnt, bool transp) { SetUniformMatrix2fv(us->GetLocation(), cnt, transp, m); }
		virtual void SetUniformMatrix3x3(UniformState* us, const float* m, int cnt, bool transp) { SetUniformMatrix3fv(us->GetLocation(), cnt, transp, m); }
		virtual void SetUniformMatrix4x4(UniformState* us, const float* m, int cnt, bool transp) { SetUniformMatrix4fv(us->GetLocation(), cnt, transp, m); }

	protected:
		int GetUniformLocation(const char* name, size_t hash) { return (GetOrAddUniformState(name, hash)->GetLocation()); }

	private:
		UniformState* GetOrAddUniformState(const char* name) { return (GetOrAddUniformState(name, hashString(name))); }
		UniformState* GetOrAddUniformState(const char* name, size_t hash) {
			// (when inlined) hash might be compiletime const cause of constexpr of hashString
			// WARNING: Cause of a bug in gcc, you _must_ assign the constexpr to a var before
			//          passing it to a function. I.e. foo.find(hashString(name)) would always
			//          be runtime evaluated (even when `name` is a literal)!
			const auto iter = uniformStates.find(hash);

			if (iter != uniformStates.end())
				return &iter->second;

			return (AddUniformState(name, hash));
		}

		UniformState* AddUniformState(const char* name, size_t hash) {
			const auto pair = uniformStates.emplace(hash, name);
			const auto iter = pair.first;

			UniformState* us = &(iter->second);
			us->SetLocation(GetUniformLoc(name));
			return us;
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

		void Enable() override {}
		void Disable() override {}
		bool CreateAndLink() override { return false; }
		bool CopyUniformsAndValidate(unsigned int tgtProgID, unsigned int srcProgID) override { return false; }
		void Link() override {}
		void Release(bool deleteShaderObjs = true) override {}
		void Reload(bool force, bool linking) override {}
		bool ReloadState(bool reloadShaderObjs) override { return true; }
		bool Validate() override { return true; }
	};



	struct GLSLProgramObject: public Shader::IProgramObject {
	public:
		GLSLProgramObject(): IProgramObject() {}
		GLSLProgramObject(const std::string& poName); // NOTE: calls glCreateProgram
		GLSLProgramObject(const GLSLProgramObject&) = delete;
		GLSLProgramObject(GLSLProgramObject&& po) { *this = std::move(po); }
		~GLSLProgramObject() { assert(glid == 0 && shaderObjs.empty()); }

		GLSLProgramObject& operator = (const GLSLProgramObject& po) = delete;
		GLSLProgramObject& operator = (GLSLProgramObject&& po) {
			uniformHashes = std::move(po.uniformHashes);

			IProgramObject::operator = (std::move(po));
			return *this;
		}

		void Enable() override;
		void Disable() override { DisableRaw(); }
		void EnableRaw() override;
		void DisableRaw() override;
		bool CreateAndLink() override;
		bool CopyUniformsAndValidate(unsigned int tgtProgID, unsigned int srcProgID) override;
		bool CopyUniformsAndValidate(unsigned int srcProgID) { return (CopyUniformsAndValidate(glid, srcProgID)); }
		void Link() override; // NOTE: calls MaybeReload (i.e. Reload which compiles *and* links)
		bool Validate() override;
		void Release(bool deleteShaderObjs = true) override;
		void Reload(bool force, bool linking) override;
		bool ReloadState(bool reloadShaderObjs) override;

		void ClearUniformLocations();
		void SetShaderDefinitions(const std::string& defs);
		void ReloadShaderObjects();
		void RecalculateShaderHash();

	public:
		int GetUniformLoc(const char* name) const override;
		int GetUniformType(int idx) const override;

		void SetUniform(const ShaderInput& u) override { SetUniformLocation(u.name); } // TODO: values (u.type, u.data)
		int SetUniformLocation(const char* name) override {
			uniformHashes.push_back(hashString(name));
			// create a new state if one is not yet associated with <name>
			return (GetUniformLocation(name, uniformHashes.back()));
		}

		void SetUniform1i(int idx,   int v0                              ) override { SetUniform(GetUniformState(idx), v0            ); }
		void SetUniform2i(int idx,   int v0,   int v1                    ) override { SetUniform(GetUniformState(idx), v0, v1        ); }
		void SetUniform3i(int idx,   int v0,   int v1,   int v2          ) override { SetUniform(GetUniformState(idx), v0, v1, v2    ); }
		void SetUniform4i(int idx,   int v0,   int v1,   int v2,   int v3) override { SetUniform(GetUniformState(idx), v0, v1, v2, v3); }
		void SetUniform1f(int idx, float v0                              ) override { SetUniform(GetUniformState(idx), v0            ); }
		void SetUniform2f(int idx, float v0, float v1                    ) override { SetUniform(GetUniformState(idx), v0, v1        ); }
		void SetUniform3f(int idx, float v0, float v1, float v2          ) override { SetUniform(GetUniformState(idx), v0, v1, v2    ); }
		void SetUniform4f(int idx, float v0, float v1, float v2, float v3) override { SetUniform(GetUniformState(idx), v0, v1, v2, v3); }

		void SetUniform2iv(int idx,          const   int* v) override { SetUniform2v(GetUniformState(idx), v,   1); }
		void SetUniform2iv(int idx, int cnt, const   int* v) override { SetUniform2v(GetUniformState(idx), v, cnt); }
		void SetUniform3iv(int idx,          const   int* v) override { SetUniform3v(GetUniformState(idx), v,   1); }
		void SetUniform3iv(int idx, int cnt, const   int* v) override { SetUniform3v(GetUniformState(idx), v, cnt); }
		void SetUniform4iv(int idx,          const   int* v) override { SetUniform4v(GetUniformState(idx), v,   1); }
		void SetUniform4iv(int idx, int cnt, const   int* v) override { SetUniform4v(GetUniformState(idx), v, cnt); }
		void SetUniform2fv(int idx,          const float* v) override { SetUniform2v(GetUniformState(idx), v,   1); }
		void SetUniform2fv(int idx, int cnt, const float* v) override { SetUniform2v(GetUniformState(idx), v, cnt); }
		void SetUniform3fv(int idx,          const float* v) override { SetUniform3v(GetUniformState(idx), v,   1); }
		void SetUniform3fv(int idx, int cnt, const float* v) override { SetUniform3v(GetUniformState(idx), v, cnt); }
		void SetUniform4fv(int idx,          const float* v) override { SetUniform4v(GetUniformState(idx), v,   1); }
		void SetUniform4fv(int idx, int cnt, const float* v) override { SetUniform4v(GetUniformState(idx), v, cnt); }

		void SetUniformMatrix2fv(int idx,          bool transp, const float* v) override { SetUniformMatrix2x2(GetUniformState(idx), v,   1, transp); }
		void SetUniformMatrix2fv(int idx, int cnt, bool transp, const float* v) override { SetUniformMatrix2x2(GetUniformState(idx), v, cnt, transp); }
		void SetUniformMatrix3fv(int idx,          bool transp, const float* v) override { SetUniformMatrix3x3(GetUniformState(idx), v,   1, transp); }
		void SetUniformMatrix3fv(int idx, int cnt, bool transp, const float* v) override { SetUniformMatrix3x3(GetUniformState(idx), v, cnt, transp); }
		void SetUniformMatrix4fv(int idx,          bool transp, const float* v) override { SetUniformMatrix4x4(GetUniformState(idx), v,   1, transp); }
		void SetUniformMatrix4fv(int idx, int cnt, bool transp, const float* v) override { SetUniformMatrix4x4(GetUniformState(idx), v, cnt, transp); }

	private:
		UniformState* GetUniformState(size_t idx) {
			assert(IsBound());
			assert(idx < uniformHashes.size());
			const auto it = uniformStates.find(uniformHashes[idx]);
			assert(it != uniformStates.end());
			return &(it->second);
		}

		void SetUniform(UniformState* us,   int v0                              ) override;
		void SetUniform(UniformState* us,   int v0,   int v1                    ) override;
		void SetUniform(UniformState* us,   int v0,   int v1,   int v2          ) override;
		void SetUniform(UniformState* us,   int v0,   int v1,   int v2,   int v3) override;
		void SetUniform(UniformState* us, float v0                              ) override;
		void SetUniform(UniformState* us, float v0, float v1                    ) override;
		void SetUniform(UniformState* us, float v0, float v1, float v2          ) override;
		void SetUniform(UniformState* us, float v0, float v1, float v2, float v3) override;

		void SetUniform2v(UniformState* us, const   int* v, int cnt = 1) override;
		void SetUniform3v(UniformState* us, const   int* v, int cnt = 1) override;
		void SetUniform4v(UniformState* us, const   int* v, int cnt = 1) override;
		void SetUniform2v(UniformState* us, const float* v, int cnt = 1) override;
		void SetUniform3v(UniformState* us, const float* v, int cnt = 1) override;
		void SetUniform4v(UniformState* us, const float* v, int cnt = 1) override;

		void SetUniformMatrix2x2(UniformState* us, const float* v, int cnt, bool transp) override;
		void SetUniformMatrix3x3(UniformState* us, const float* v, int cnt, bool transp) override;
		void SetUniformMatrix4x4(UniformState* us, const float* v, int cnt, bool transp) override;

	private:
		std::vector<size_t> uniformHashes;
	};


	extern NullShaderObject* nullShaderObject;
	extern NullProgramObject* nullProgramObject;
}

#endif
