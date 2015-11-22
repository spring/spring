/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_UNIT_MATERIAL_H
#define LUA_UNIT_MATERIAL_H

// NOTE: some implementation is done in LuaMaterial.cpp

typedef            int   GLint;
typedef unsigned   int  GLuint;
typedef          float GLfloat;
typedef unsigned   int  GLenum;

#include <vector>
using std::vector;
#include <stddef.h>

class CUnit;
class LuaMatBin;


/******************************************************************************/
/******************************************************************************/

enum LuaMatType {
	LUAMAT_ALPHA          = 0,
	LUAMAT_OPAQUE         = 1,
	LUAMAT_ALPHA_REFLECT  = 2,
	LUAMAT_OPAQUE_REFLECT = 3,
	LUAMAT_SHADOW         = 4,
	LUAMAT_TYPE_COUNT     = 5
};


/******************************************************************************/

class LuaMatRef {

	friend class LuaMatHandler;

	public:
		LuaMatRef() : bin(NULL) {}
		LuaMatRef(const LuaMatRef&);
		LuaMatRef& operator=(const LuaMatRef&);
		~LuaMatRef();

		void Reset();

		void AddUnit(CUnit*);
		void Execute() const;

		inline bool IsActive() const { return (bin != NULL); }

		inline const LuaMatBin* GetBin() const { return bin; }

	private:
		LuaMatRef(LuaMatBin* _bin);
		
	private:
		LuaMatBin* bin; // can be NULL
};


/******************************************************************************/
/******************************************************************************/

class LuaUnitUniforms {
	public:
		LuaUnitUniforms()
		: haveUniforms(false),
		  speedLoc(-1),
		  healthLoc(-1),
		  unitIDLoc(-1),
		  teamIDLoc(-1),
		  customLoc(-1),
		  customCount(0)
		{}
		LuaUnitUniforms(const LuaUnitUniforms&);
		LuaUnitUniforms& operator=(const LuaUnitUniforms&);

		void Execute(const CUnit* unit) const;

		void SetCustomCount(int count);

	public:
		bool haveUniforms;
		GLint speedLoc;
		GLint healthLoc;
		GLint unitIDLoc;
		GLint teamIDLoc;
		GLint customLoc;
		int customCount;
		vector<GLfloat> customData;


	// FIXME: do this differently
	struct EngineData {
		GLint location;
		GLenum type; // GL_FLOAT, GL_FLOAT_VEC3, etc...
		GLuint size;
		GLuint count;
		GLint*   intData;
		GLfloat* floatData;
		void* data;
	};
	vector<EngineData> uniforms;
};


/******************************************************************************/

class LuaUnitLODMaterial {
	public:
		LuaUnitLODMaterial()
		: preDisplayList(0),
		  postDisplayList(0)
		{}

		inline bool IsActive() const { return matref.IsActive(); }

		inline void AddUnit(CUnit* unit) { matref.AddUnit(unit); }

		inline void ExecuteMaterial() const { matref.Execute(); }

		inline void ExecuteUniforms(const CUnit* unit) const
		{
			uniforms.Execute(unit);
		}

	public:
		GLuint          preDisplayList;
		GLuint          postDisplayList;
		LuaMatRef       matref;
		LuaUnitUniforms uniforms;
};


/******************************************************************************/

class LuaUnitMaterial {
	public:
		LuaUnitMaterial() : lodCount(0), lastLOD(0) {}

		bool SetLODCount(unsigned int count);
		bool SetLastLOD(unsigned int count);

		inline const unsigned int GetLODCount() const { return lodCount; }
		inline const unsigned int GetLastLOD() const { return lastLOD; }

		inline bool IsActive(unsigned int lod) const {
			if (lod >= lodCount) {
				return false;
			}
			return lodMats[lod].IsActive();
		}

		inline LuaUnitLODMaterial* GetMaterial(unsigned int lod) {
			if (lod >= lodCount) {
				return NULL;
			}
			return &lodMats[lod];
		}

		inline const LuaUnitLODMaterial* GetMaterial(unsigned int lod) const {
			if (lod >= lodCount) {
				return NULL;
			}
			return &lodMats[lod];
		}

	private:
		unsigned int lodCount;
		unsigned int lastLOD;
		vector<LuaUnitLODMaterial> lodMats;
};


/******************************************************************************/

struct LuaUnitMaterialData {
public:
	LuaUnitMaterialData() {
		lodCount = 0;
		currentLOD = 0;
	}

	bool Enabled() const { return (lodCount > 0); }

	const LuaUnitMaterial* GetLuaMaterials() const { return &luaMats[0]; }

	const LuaUnitMaterial* GetLuaMaterial(LuaMatType type) const { return &luaMats[type]; }
	      LuaUnitMaterial* GetLuaMaterial(LuaMatType type)       { return &luaMats[type]; }

	const LuaUnitLODMaterial* GetLuaLODMaterial(LuaMatType type) const {
		const LuaUnitMaterial* mat = GetLuaMaterial(type);
		const LuaUnitLODMaterial* lodMat = mat->GetMaterial(currentLOD);
		return lodMat;
	}

	LuaUnitLODMaterial* GetLuaLODMaterial(LuaMatType type) {
		LuaUnitMaterial* mat = GetLuaMaterial(type);
		LuaUnitLODMaterial* lodMat = mat->GetMaterial(currentLOD);
		return lodMat;
	}

	void UpdateCurrentLOD(float lodDist, LuaMatType type) {
		SetCurrentLOD(CalcCurrentLOD(lodDist, luaMats[type].GetLastLOD()));
	}

	unsigned int CalcCurrentLOD(float lodDist, unsigned int lastLOD) const;
	unsigned int SetCurrentLOD(unsigned int lod) { return (currentLOD = lod); }

	unsigned int GetLODCount() const { return lodCount; }
	unsigned int GetCurrentLOD() const { return currentLOD; }

	float GetLODLength(unsigned int lod) const { return lodLengths[lod]; }


	void PushLODCount(unsigned int tmpCount) { lodStack.push_back(lodCount); lodCount = tmpCount; }
	void PopLODCount() { lodCount = lodStack.back(); lodStack.pop_back(); }

	void SetLODCount(unsigned int count) {
		const unsigned int oldCount = lodCount;

		lodLengths.resize(lodCount = count);

		for (unsigned int i = oldCount; i < count; i++) {
			lodLengths[i] = -1.0f;
		}

		for (int m = 0; m < LUAMAT_TYPE_COUNT; m++) {
			luaMats[m].SetLODCount(count);
		}
	}

	void SetLODLength(unsigned int lod, float length) {
		if (lod >= lodCount)
			return;

		lodLengths[lod] = length;
	}

	bool AddUnitForLOD(CUnit* unit, LuaMatType type, float lodDist) {
		if (!Enabled())
			return false;

		LuaUnitMaterial* unitMat = GetLuaMaterial(type);
		LuaUnitLODMaterial* lodMat = unitMat->GetMaterial(SetCurrentLOD(CalcCurrentLOD(lodDist, unitMat->GetLastLOD())));

		if ((lodMat != NULL) && lodMat->IsActive()) {
			lodMat->AddUnit(unit);
			return true;
		}

		return false;
	}

	static void SetGlobalLODFactor(float value) {
		UNIT_GLOBAL_LOD_FACTOR = value;
	}

private:
	static float UNIT_GLOBAL_LOD_FACTOR;

	// equal to lodLengths.size(); if non-zero, then at least
	// one LOD-level has been assigned a custom Lua material
	unsigned int lodCount;
	// which LuaUnitLODMaterial should be used
	unsigned int currentLOD;

	// length-per-pixel; see CalcCurrentLOD
	std::vector<float> lodLengths;
	std::vector<unsigned int> lodStack;

	LuaUnitMaterial luaMats[LUAMAT_TYPE_COUNT];
};


/******************************************************************************/
/******************************************************************************/


#endif /* LUA_UNIT_MATERIAL_H */
