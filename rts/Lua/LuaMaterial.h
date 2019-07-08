/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_MATERIAL_H
#define LUA_MATERIAL_H

#include <cstring> // strcmp
#include <string>
#include <vector>

/*
LuaMaterial
	LuaMatShader
	LuaMatUniforms
	LuaMatTex
LuaMatBin : public LuaMaterial
	array of units & features that use the Material
	refCounter

class LuaMatHandler
	LuaMatBin* []

LuaMatRef
	LuaMatBin*

LuaObjectMaterial (set of LODs) //FIXME merge with LuaMatRef?
	LuaMatRef []

LuaObjectMaterialData (helper to choose the current LOD) //FIXME rename
	LuaObjectMaterial []
*/


#include "LuaOpenGLUtils.h"
#include "LuaObjectMaterial.h" // for LuaMatRef
#include "Rendering/GL/myGL.h"
#include "System/StringHash.h"
#include "System/UnorderedMap.hpp"


class CSolidObject;

/******************************************************************************/
/******************************************************************************/

class LuaMatShader {
	public:
		enum Type {
			LUASHADER_3DO    = 0, // engine default; *must* equal MODELTYPE_3DO!
			LUASHADER_S3O    = 1, // engine default; *must* equal MODELTYPE_S3O!
			LUASHADER_ASS    = 2, // engine default; *must* equal MODELTYPE_ASS!
			LUASHADER_GL     = 3, // custom Lua; *must* equal MODELTYPE_OTHER
			LUASHADER_LAST   = 4,
		};
		enum Pass {
			LUASHADER_PASS_FWD = 0, // forward pass
			LUASHADER_PASS_DFR = 1, // deferred pass
			LUASHADER_PASS_CNT = 2,
		};

	public:
		void Finalize() { openglID *= (type == LUASHADER_GL); }
		void Execute(const LuaMatShader& prev, bool deferredPass) const;
		void Print(const std::string& indent, bool isDeferred) const;

		// only accepts custom programs
		void SetCustomTypeFromID(unsigned int id) {
			if ((openglID = id) == 0) {
				type = LuaMatShader::LUASHADER_LAST;
			} else {
				type = LuaMatShader::LUASHADER_GL;
			}
		}
		// only accepts engine programs
		void SetEngineTypeFromKey(const char* key) {
			switch (hashStringLower(key)) {
				case hashStringLower("3DO"): { type = LUASHADER_3DO ; } break;
				case hashStringLower("S3O"): { type = LUASHADER_S3O ; } break;
				case hashStringLower("ASS"): { type = LUASHADER_ASS ; } break;
				default                    : { type = LUASHADER_LAST; } break;
			}
		}

		static int Compare(const LuaMatShader& a, const LuaMatShader& b);

		bool operator <  (const LuaMatShader& mt) const { return (Compare(*this, mt) < 0); }
		bool operator == (const LuaMatShader& mt) const = delete;
		bool operator != (const LuaMatShader& mt) const = delete;

		// both passes require a (custom or engine) shader
		bool ValidForPass(Pass pass) const { return (type != LUASHADER_LAST); }

		bool IsCustomType() const { return (type == LUASHADER_GL); }
		bool IsEngineType() const { return (type >= LUASHADER_3DO && type <= LUASHADER_ASS); }

	public:
		Type type = LUASHADER_LAST;
		GLuint openglID = 0;
};


/******************************************************************************/

struct LuaMatUniform {
	char name[32] = {0};
	union {
		int i[32];
		float f[32];
	} data;

	// GL_INT or GL_FLOAT_*
	int type = 0;
	// number of data elements
	int size = 0;

	// -3 := empty uniform, -2 := loc not set, -1 := loc invalid
	mutable int loc = -3;
};

struct LuaMatUniforms {
public:
	LuaMatUniforms() {
		ClearObjectUniforms(LUAOBJ_UNIT   );
		ClearObjectUniforms(LUAOBJ_FEATURE);
	}

	void Print(const std::string& indent, bool isDeferred) const;
	void Parse(lua_State* L, const int tableIdx);
	void AutoLink(LuaMatShader* s);
	void Validate(LuaMatShader* s);

	void Execute() const;
	void ExecuteInstanceTeamColor(const float4& tc) const { teamColor.Execute(tc); }
	void ExecuteInstanceMatrices(const CMatrix44f& mm, const std::vector<CMatrix44f>& pms) const {
		modelMatrix.Execute(mm);
		pieceMatrices.Execute(pms);
	}


	bool ClearObjectUniforms(int objType) {
		objectUniforms[objType].clear();
		objectUniforms[objType].reserve(128);
		return true;
	}

	bool ClearObjectUniforms(int objId, int objType) { return (objectUniforms[objType].erase(objId)); }
	bool ClearObjectUniform(int objId, int objType, const char* name) {
		const auto objIter = objectUniforms[objType].find(objId);
		const auto strPred = [name](const LuaMatUniform& a) { return (strcmp(name, a.name) == 0); };
		const auto locPred = [](const LuaMatUniform& a) { return (a.loc == -3); };

		if (objIter == objectUniforms[objType].end())
			return false;

		auto& uniformData = objIter->second;
		auto  uniformIter = std::find_if(uniformData.begin(), uniformData.end(), strPred);

		if (uniformIter == uniformData.end())
			return false;

		assert(uniformIter->loc != -3);

		// replace iter with last valid (locIter - 1) uniform
		auto locIter = std::find_if(uniformIter + 1, uniformData.end(), locPred);

		if ((locIter--) == uniformData.end()) {
			*uniformIter = uniformData.back();
			return (uniformData.back() = {}, true);
		}

		*uniformIter = *locIter;
		return (*locIter = {}, true);
	}

	bool AddObjectUniform(int objId, int objType, const LuaMatUniform& u) {
		const auto pred = [](const LuaMatUniform& a) { return (a.loc == -3); };

		auto& uniformData = objectUniforms[objType][objId];
		auto  uniformIter = std::find_if(uniformData.begin(), uniformData.end(), pred);

		if (uniformIter == uniformData.end())
			return false;

		return (*uniformIter = u, true);
	}


	static LuaMatUniform& GetDummyObjectUniform() {
		static LuaMatUniform u;
		return (u = {});
	}

	LuaMatUniform& GetObjectUniform(int objId, int objType, const char* name) {
		const auto pred = [name](const LuaMatUniform& a) { return (strcmp(name, a.name) == 0); };

		auto& data = objectUniforms[objType][objId];
		auto  iter = std::find_if(data.begin(), data.end(), pred);

		if (iter == data.end())
			return (GetDummyObjectUniform());

		return *iter;
	}


	static int Compare(const LuaMatUniforms& a, const LuaMatUniforms& b);

public:
	struct IUniform {
		static_assert(GL_INVALID_INDEX == -1, "glGetUniformLocation is defined to return -1 (GL_INVALID_INDEX) for invalid names");

		virtual bool CanExec() const = 0;
		virtual GLenum GetType() const = 0;

		bool SetLoc(GLint i) { return ((loc = i) != GL_INVALID_INDEX); }
		bool IsValid() const { return (loc != GL_INVALID_INDEX); }

	public:
		GLint loc = GL_INVALID_INDEX;
	};

private:
	template<typename Type> struct UniformMat : public IUniform {
	public:
		bool CanExec() const { return (loc != GL_INVALID_INDEX); }
		bool Execute(const Type& val) const { return (CanExec() && RawExec(val)); }
		bool RawExec(const Type& val) const { glUniformMatrix4fv(loc, 1, GL_FALSE, val); return true; }

		GLenum GetType() const { return GL_FLOAT_MAT4; }
	};

	template<typename Type> struct UniformMatArray : public IUniform {
	public:
		bool CanExec() const { return (loc != GL_INVALID_INDEX); }
		bool Execute(const Type& val) const { return (CanExec() && RawExec(val)); }
		bool RawExec(const Type& val) const { glUniformMatrix4fv(loc, val.size(), GL_FALSE, val[0]); return true; }

		GLenum GetType() const { return GL_FLOAT_MAT4; }
	};

	template<typename Type, size_t Size = sizeof(Type) / sizeof(float)> struct UniformVec : public IUniform {
	public:
		bool CanExec() const { return (loc != GL_INVALID_INDEX); }
		bool Execute(const Type val) const { return (CanExec() && RawExec(val)); }
		bool RawExec(const Type val) const {
			switch (Size) {
				case 3: { glUniform3fv(loc, 1, &val.x); return true; } break;
				case 4: { glUniform4fv(loc, 1, &val.x); return true; } break;
				default: { return false; } break;
			}
		}

		GLenum GetType() const {
			switch (Size) {
				case 3: return GL_FLOAT_VEC3;
				case 4: return GL_FLOAT_VEC4;
				default: return 0;
			}
		}
	};

	template<typename Type> struct UniformInt : public IUniform {
	public:
		bool CanExec() const { return (loc != -1 && cur != prv); } //FIXME a shader might be bound to multiple materials in that case we cannot rely on this!
		bool Execute(const Type val) { cur = val; return (CanExec() && RawExec(val)); }
		bool RawExec(const Type val) { glUniform1i(loc, prv = val); return true; }

		GLenum GetType() const { return GL_INT; }
	public:
		Type cur = Type(0);
		Type prv = Type(0);
	};

private:
	spring::unsynced_map<IUniform*, std::string> GetEngineUniformNamePairs();
	spring::unsynced_map<std::string, IUniform*> GetEngineNameUniformPairs();

public:
	// per-{unit,feature} custom uniforms set via LuaObjectRendering
	spring::unsynced_map<int, std::array<LuaMatUniform, 16>> objectUniforms[2];

	UniformMat<CMatrix44f> viewMatrix;
	UniformMat<CMatrix44f> projMatrix;
	UniformMat<CMatrix44f> viprMatrix;
	UniformMat<CMatrix44f> viewMatrixInv;
	UniformMat<CMatrix44f> projMatrixInv;
	UniformMat<CMatrix44f> viprMatrixInv;

	UniformMat<CMatrix44f> modelMatrix;
	UniformMatArray< std::vector<CMatrix44f> > pieceMatrices;

	UniformMat<CMatrix44f> shadowMatrix;
	UniformVec<float4> shadowParams;

	UniformVec<float4> teamColor;

	UniformVec<float3> camPos;
	UniformVec<float3> camDir;
	UniformVec<float3> sunDir;
	UniformVec<float3> rndVec;

	mutable UniformInt<         int> simFrame;
	mutable UniformInt<unsigned int> visFrame;
};


class LuaMaterial {
public:
	LuaMaterial() = default;
	LuaMaterial(LuaMatType matType): type(matType) {}

	void Parse(
		lua_State* L,
		const int tableIdx,
		void(*ParseShader)(lua_State*, int, LuaMatShader&),
		void(*ParseTexture)(lua_State*, int, LuaMatTexture&),
		GLuint(*ParseDisplayList)(lua_State*, int)
	);

	void Finalize();
	void Execute(const LuaMaterial& prev, bool deferredPass) const;

	void ExecuteInstanceUniforms(int objId, int objType, bool deferredPass) const;
	void ExecuteInstanceTeamColor(const float4& tc, bool deferredPass) const { uniforms[deferredPass].ExecuteInstanceTeamColor(tc); }
	void ExecuteInstanceMatrices(const CMatrix44f& mm, const std::vector<CMatrix44f>& pms, bool deferredPass) const {
		uniforms[deferredPass].ExecuteInstanceMatrices(mm, pms);
	}

	void Print(const std::string& indent) const;

	static int Compare(const LuaMaterial& a, const LuaMaterial& b);

	bool operator <  (const LuaMaterial& m) const { return (Compare(*this, m) <  0); }
	bool operator == (const LuaMaterial& m) const { return (Compare(*this, m) == 0); }

	bool HasDrawCall() const { return (uuid >= 0); }

public:
	static constexpr int MAX_TEX_UNITS = 16;

	// default invalid
	LuaMatType type = LuaMatType(-1);

	int uuid = -1; // user-set unique ID, enables Draw callin
	int order = 0; // for manually adjusting rendering order
	int texCount = 0;

	// [0] := standard, [1] := deferred
	LuaMatShader   shaders[LuaMatShader::LUASHADER_PASS_CNT];
	LuaMatUniforms uniforms[LuaMatShader::LUASHADER_PASS_CNT];
	LuaMatTexture  textures[MAX_TEX_UNITS];

	GLenum cullingMode = 0;

	static const LuaMaterial defMat;
};


/******************************************************************************/
/******************************************************************************/

class LuaMatBin : public LuaMaterial {
friend class LuaMatHandler;
public:
	void ClearUnits() { units.clear(); }
	void ClearFeatures() { features.clear(); }

	const std::vector<CSolidObject*>& GetUnits() const { return units; }
	const std::vector<CSolidObject*>& GetFeatures() const { return features; }
	const std::vector<CSolidObject*>& GetObjects(LuaObjType objType) const {
		static const std::vector<CSolidObject*> dummy;

		switch (objType) {
			case LUAOBJ_UNIT   : { return (GetUnits   ()); } break;
			case LUAOBJ_FEATURE: { return (GetFeatures()); } break;
			default            : {          assert(false); } break;
		}

		return dummy;
	}

	void Ref() { refCount++; }
	void UnRef();

	void AddUnit(CSolidObject* o) { units.push_back(o); }
	void AddFeature(CSolidObject* o) { features.push_back(o); }
	void AddObject(CSolidObject* o, LuaObjType objType) {
		switch (objType) {
			case LUAOBJ_UNIT   : { AddUnit   (o); } break;
			case LUAOBJ_FEATURE: { AddFeature(o); } break;
			default            : { assert(false); } break;
		}
	}

	void Print(const std::string& indent) const;

private:
	LuaMatBin(const LuaMaterial& _mat) : LuaMaterial(_mat), refCount(0) {}
	LuaMatBin(const LuaMatBin&);
	LuaMatBin& operator=(const LuaMatBin&) = delete;

private:
	int refCount;

	std::vector<CSolidObject*> units;
	std::vector<CSolidObject*> features;
};


/******************************************************************************/

// typedef std::set<LuaMatBin*, LuaMatBinPtrLessThan> LuaMatBinSet;
typedef std::vector<LuaMatBin*> LuaMatBinSet;


/******************************************************************************/

class LuaMatHandler {
	public:
		LuaMatRef GetRef(const LuaMaterial& mat);

		const LuaMatBinSet& GetBins(LuaMatType type) const { return binTypes[type]; }

		void ClearBins(LuaObjType objType);
		void ClearBins(LuaObjType objType, LuaMatType matType);

		void FreeBin(LuaMatBin* bin);

		void PrintBins(const std::string& indent, LuaMatType type) const;
		void PrintAllBins(const std::string& indent) const;

	public:
		typedef void (*SetupDrawStateFunc)(unsigned int, bool);
		typedef void (*ResetDrawStateFunc)(unsigned int, bool);

		// note: only the DEF_* entries can be non-NULL
		SetupDrawStateFunc setupDrawStateFuncs[LuaMatShader::LUASHADER_LAST];
		ResetDrawStateFunc resetDrawStateFuncs[LuaMatShader::LUASHADER_LAST];

	private:
		LuaMatHandler();
		LuaMatHandler(const LuaMatHandler&);
		LuaMatHandler operator=(const LuaMatHandler&);
		~LuaMatHandler();

	private:
		LuaMatBinSet binTypes[LUAMAT_TYPE_COUNT];

	public:
		static LuaMatHandler handler;
};


/******************************************************************************/
/******************************************************************************/


extern LuaMatHandler& luaMatHandler;


#endif /* LUA_MATERIAL_H */
