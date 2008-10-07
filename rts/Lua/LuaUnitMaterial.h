#ifndef LUA_UNIT_MATERIAL_H
#define LUA_UNIT_MATERIAL_H
// LuaUnitMaterial.h: interface for the LuaUnitMaterial class.
//
// NOTE: some implementation is done in LuaMaterial.cpp
//
//////////////////////////////////////////////////////////////////////

#include "Rendering/GL/myGL.h"

#include <vector>
using std::vector;

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
		  customCount(0),
		  customData(NULL)
		{}
		LuaUnitUniforms(const LuaUnitUniforms&);
		LuaUnitUniforms& operator=(const LuaUnitUniforms&);
		~LuaUnitUniforms() { delete[] customData; }

		void Execute(CUnit* unit) const;

		void SetCustomCount(int count);

	public:
		bool haveUniforms;
		GLint speedLoc;	
		GLint healthLoc;
		GLint unitIDLoc;
		GLint teamIDLoc;
		GLint customLoc;
		int customCount;
		GLfloat* customData;


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

		inline void ExecuteUniforms(CUnit* unit) const
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
/******************************************************************************/


#endif /* LUA_UNIT_MATERIAL_H */
