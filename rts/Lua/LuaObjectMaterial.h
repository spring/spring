/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_OBJECT_MATERIAL_H
#define LUA_OBJECT_MATERIAL_H

// NOTE: some implementation is done in LuaMaterial.cpp

typedef            int   GLint;
typedef unsigned   int  GLuint;
typedef          float GLfloat;
typedef unsigned   int  GLenum;

#include <algorithm>
#include <cassert>
#include <vector>

class CSolidObject;
class LuaMatBin;


/******************************************************************************/
/******************************************************************************/

enum LuaObjType {
	LUAOBJ_UNIT    = 0,
	LUAOBJ_FEATURE = 1,
	LUAOBJ_LAST    = 2,
};

enum LuaMatType { //FIXME move deferred here
	LUAMAT_ALPHA          = 0,
	LUAMAT_OPAQUE         = 1,
	LUAMAT_ALPHA_REFLECT  = 2,
	LUAMAT_OPAQUE_REFLECT = 3,
	LUAMAT_SHADOW         = 4,
	LUAMAT_TYPE_COUNT     = 5,
};


/******************************************************************************/

class LuaMatRef {

	friend class LuaMatHandler;

	public:
		LuaMatRef() : bin(nullptr) {}
		LuaMatRef(const LuaMatRef&);
		LuaMatRef& operator=(const LuaMatRef&);
		~LuaMatRef();

		void Reset();

		void AddUnit(CSolidObject*);
		void AddFeature(CSolidObject*);

		inline bool IsActive() const { return (bin != nullptr); }

		inline const LuaMatBin* GetBin() const { return bin; }

	private:
		LuaMatRef(LuaMatBin* _bin);
		
	private:
		LuaMatBin* bin; // can be NULL
};


/******************************************************************************/

class LuaObjectLODMaterial {
	public:
		inline bool IsActive() const { return matref.IsActive(); }

		inline void AddUnit(CSolidObject* o) { matref.AddUnit(o); }
		inline void AddFeature(CSolidObject* o) { matref.AddFeature(o); }

	public:
		LuaMatRef matref;
};


/******************************************************************************/

class LuaObjectMaterial {
	public:
		bool SetLODCount(unsigned int count);
		bool SetLastLOD(unsigned int count);

		inline const unsigned int GetLODCount() const { return lodCount; }
		inline const unsigned int GetLastLOD() const { return lastLOD; }

		inline bool IsActive(unsigned int lod) const {
			if (lod >= lodCount)
				return false;

			return lodMats[lod].IsActive();
		}

		inline LuaObjectLODMaterial* GetMaterial(unsigned int lod) {
			if (lod >= lodCount)
				return nullptr;

			return &lodMats[lod];
		}

		inline const LuaObjectLODMaterial* GetMaterial(unsigned int lod) const {
			if (lod >= lodCount)
				return nullptr;

			return &lodMats[lod];
		}

	private:
		unsigned int lodCount = 0;
		unsigned int lastLOD = 0;
		std::vector<LuaObjectLODMaterial> lodMats;
};


/******************************************************************************/

struct LuaObjectMaterialData {
public:
	bool Enabled() const { return (lodCount > 0); }
	bool ValidLOD(unsigned int lod) const { return (lod < lodCount); }

	const LuaObjectMaterial* GetLuaMaterials() const { return &luaMats[0]; }

	const LuaObjectMaterial* GetLuaMaterial(LuaMatType type) const { return &luaMats[type]; }
	      LuaObjectMaterial* GetLuaMaterial(LuaMatType type)       { return &luaMats[type]; }

	const LuaObjectLODMaterial* GetLuaLODMaterial(LuaMatType type) const {
		const LuaObjectMaterial* mat = GetLuaMaterial(type);
		const LuaObjectLODMaterial* lodMat = mat->GetMaterial(currentLOD);
		return lodMat;
	}

	LuaObjectLODMaterial* GetLuaLODMaterial(LuaMatType type) {
		LuaObjectMaterial* mat = GetLuaMaterial(type);
		LuaObjectLODMaterial* lodMat = mat->GetMaterial(currentLOD);
		return lodMat;
	}

	void UpdateCurrentLOD(LuaObjType objType, float lodDist, LuaMatType matType) {
		SetCurrentLOD(CalcCurrentLOD(objType, lodDist, luaMats[matType].GetLastLOD()));
	}

	unsigned int CalcCurrentLOD(LuaObjType objType, float lodDist, unsigned int lastLOD) const {
		if (lastLOD >= lodCount)
			return 0;

		// positive values only!
		const float lpp = std::max(0.0f, lodDist * GLOBAL_LOD_FACTORS[objType]);

		for (/* no-op */; (lastLOD != 0 && lpp <= lodLengths[lastLOD]); lastLOD--) {
		}

		return lastLOD;
	}

	unsigned int SetCurrentLOD(unsigned int lod) { return (currentLOD = lod); }

	unsigned int GetLODCount() const { return lodCount; }
	unsigned int GetCurrentLOD() const { return currentLOD; }

	float GetLODLength(unsigned int lod) const { return lodLengths[lod]; }


	void PushLODCount(unsigned int tmpCount) { lodStack.push_back(lodCount); lodCount = tmpCount; }
	void PopLODCount() { lodCount = lodStack.back(); lodStack.pop_back(); }

	void SetLODCount(unsigned int count) {
		const unsigned int oldCount = lodCount;

		lodLengths.resize(lodCount = count);

		for (unsigned int i = oldCount; i < lodCount; i++) {
			lodLengths[i] = -1.0f;
		}

		for (int m = 0; m < LUAMAT_TYPE_COUNT; m++) {
			luaMats[m].SetLODCount(lodCount);
		}
	}

	void SetLODLength(unsigned int lod, float length) {
		if (lod >= lodCount)
			return;

		lodLengths[lod] = length;
	}


	bool AddObjectForLOD(CSolidObject* o, LuaObjType objType, LuaMatType matType, float lodDist) {
		if (!Enabled())
			return false;

		LuaObjectMaterial* objMat = GetLuaMaterial(matType);
		LuaObjectLODMaterial* lodMat = objMat->GetMaterial(SetCurrentLOD(CalcCurrentLOD(objType, lodDist, objMat->GetLastLOD())));

		if ((lodMat != nullptr) && lodMat->IsActive()) {
			switch (objType) {
				case LUAOBJ_UNIT   : { lodMat->AddUnit   (o); } break;
				case LUAOBJ_FEATURE: { lodMat->AddFeature(o); } break;
				default            : {         assert(false); } break; // gcc needs always a default case, else it's not able to optimize the switch
			}

			return true;
		}

		return false;
	}

	static void SetGlobalLODFactor(LuaObjType objType, float lodFactor) {
		GLOBAL_LOD_FACTORS[objType] = lodFactor;
	}

private:
	static float GLOBAL_LOD_FACTORS[LUAOBJ_LAST];

	// equal to lodLengths.size(); if non-zero, then at least
	// one LOD-level has been assigned a custom Lua material
	unsigned int lodCount = 0;
	// which LuaObjectLODMaterial should be used
	unsigned int currentLOD = 0;

	// length-per-pixel; see CalcCurrentLOD
	std::vector<float> lodLengths;
	std::vector<unsigned int> lodStack;

	LuaObjectMaterial luaMats[LUAMAT_TYPE_COUNT];
};


/******************************************************************************/
/******************************************************************************/


#endif /* LUA_OBJECT_MATERIAL_H */
