/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_MATERIAL_H
#define LUA_MATERIAL_H

#include <string>
#include <vector>
#include <set>

using std::string;
using std::set;

#include "LuaOpenGLUtils.h"
#include "LuaObjectMaterial.h" // for LuaMatRef

#include "Rendering/GL/myGL.h"

class CSolidObject;


class LuaMatTexSetRef;
class LuaMatUnifSetRef;


/******************************************************************************/
/******************************************************************************/

class LuaMatShader {
	public:
		enum Type {
			LUASHADER_DEF_3DO = 0, // engine default; *must* equal MODELTYPE_3DO!
			LUASHADER_DEF_S3O = 1, // engine default; *must* equal MODELTYPE_S3O!
			LUASHADER_DEF_OBJ = 2, // engine default; *must* equal MODELTYPE_OBJ!
			LUASHADER_DEF_ASS = 3, // engine default; *must* equal MODELTYPE_ASS!
			LUASHADER_GL      = 4, // custom Lua
			LUASHADER_NONE    = 5,
			LUASHADER_LAST    = 6,
		};
		enum Pass {
			LUASHADER_PASS_FWD = 0, // forward pass
			LUASHADER_PASS_DFR = 1, // deferred pass
			LUASHADER_PASS_CNT = 2,
		};

	public:
		LuaMatShader() : type(LUASHADER_NONE), openglID(0) {}

		void Finalize();
		void Execute(const LuaMatShader& prev, bool deferredPass) const;
		void Print(const string& indent) const;

		void SetTypeFromID(unsigned int id) {
			if ((openglID = id) == 0) {
				type = LuaMatShader::LUASHADER_NONE;
			} else {
				type = LuaMatShader::LUASHADER_GL;
			}
		}
		void SetTypeFromKey(const string& key) {
			if (key == "3do") { type = LUASHADER_DEF_3DO; return; }
			if (key == "s3o") { type = LUASHADER_DEF_S3O; return; }
			if (key == "obj") { type = LUASHADER_DEF_OBJ; return; }
			if (key == "ass") { type = LUASHADER_DEF_ASS; return; }
			type = LUASHADER_NONE;
		}

		static int Compare(const LuaMatShader& a, const LuaMatShader& b);

		bool operator <(const LuaMatShader& mt) const { return (Compare(*this, mt)  < 0); }
		bool operator==(const LuaMatShader& mt) const { return (Compare(*this, mt) == 0); }
		bool operator!=(const LuaMatShader& mt) const { return (Compare(*this, mt) != 0); }

		bool ValidForPass(Pass pass) const { return (pass != LUASHADER_PASS_DFR || type != LUASHADER_NONE); }

		bool IsCustomType() const { return (type == LUASHADER_GL); }
		bool IsEngineType() const { return (type >= LUASHADER_DEF_3DO && type <= LUASHADER_DEF_ASS); }

	public:
		Type type;
		GLuint openglID;
};

/******************************************************************************/

class LuaMatTexSet {
	public:
		static const int maxTexUnits = 16;

	public:
		LuaMatTexSet() : texCount(0) {}

	public:
		int texCount;
		LuaMatTexture textures[LuaMatTexture::maxTexUnits];
};


/******************************************************************************/

class LuaMaterial {
	public:
		LuaMaterial(LuaMatType matType = LuaMatType(-1)):
		  type(matType), // default invalid
		  order(0),
		  texCount(0),
		  cullingMode(0),

		  preList(0),
		  postList(0),

		  viewMatrixLoc(-1),
		  projMatrixLoc(-1),
		  viprMatrixLoc(-1),
		  viewMatrixInvLoc(-1),
		  projMatrixInvLoc(-1),
		  viprMatrixInvLoc(-1),
		  cameraPosLoc(-1),
		  cameraDirLoc(-1),
		  sunDirLoc(-1),
		  shadowMatrixLoc(-1),
		  shadowParamsLoc(-1),

		  useCamera(true)
		{}

		void Parse(
			lua_State* L,
			const int tableIdx,
			std::function<void(lua_State*, int, LuaMatShader&)> ParseShader,
			std::function<void(lua_State*, int, LuaMatTexture&)> ParseTexture,
			std::function<GLuint(lua_State*, int)> ParseDisplayList
		);
		void Finalize();
		void Execute(const LuaMaterial& prev, bool deferredPass) const;
		void Print(const string& indent) const;

		static int Compare(const LuaMaterial& a, const LuaMaterial& b);

		bool operator <(const LuaMaterial& m) const { return (Compare(*this, m)  < 0); }
		bool operator==(const LuaMaterial& m) const { return (Compare(*this, m) == 0); }
		bool operator!=(const LuaMaterial& m) const { return (Compare(*this, m) != 0); }

	public:
		LuaMatType type;

		int order; // for manually adjusting rendering order
		int texCount;

		// [0] := standard
		// [1] := deferred
		LuaMatShader shaders[LuaMatShader::LUASHADER_PASS_CNT];
		LuaMatTexture textures[LuaMatTexture::maxTexUnits];

		GLenum cullingMode;

		GLuint preList;
		GLuint postList;

		GLint viewMatrixLoc;
		GLint projMatrixLoc;
		GLint viprMatrixLoc;
		GLint viewMatrixInvLoc;
		GLint projMatrixInvLoc;
		GLint viprMatrixInvLoc;
		GLint cameraPosLoc;
		GLint cameraDirLoc;
		GLint sunDirLoc;
		GLint shadowMatrixLoc;
		GLint shadowParamsLoc;

		bool useCamera;

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

		void Ref();
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

		void Print(const string& indent) const;

	private:
		LuaMatBin(const LuaMaterial& _mat) : LuaMaterial(_mat), refCount(0) {}
		LuaMatBin(const LuaMatBin&);
		LuaMatBin& operator=(const LuaMatBin&);

	private:
		int refCount;

		std::vector<CSolidObject*> units;
		std::vector<CSolidObject*> features;
};


/******************************************************************************/

struct LuaMatBinPtrLessThan {
	bool operator()(const LuaMatBin* a, const LuaMatBin* b) const {
		const LuaMaterial* ma = static_cast<const LuaMaterial*>(a);
		const LuaMaterial* mb = static_cast<const LuaMaterial*>(b);
		return (*ma < *mb);
	}
};

typedef set<LuaMatBin*, LuaMatBinPtrLessThan> LuaMatBinSet;


/******************************************************************************/

class LuaMatHandler {
	public:
		LuaMatRef GetRef(const LuaMaterial& mat);

		inline const LuaMatBinSet& GetBins(LuaMatType type) const {
			return binTypes[type];
		}

		void ClearBins(LuaObjType objType);
		void ClearBins(LuaObjType objType, LuaMatType matType);

		void FreeBin(LuaMatBin* bin);

		void PrintBins(const string& indent, LuaMatType type) const;
		void PrintAllBins(const string& indent) const;

	public:
		typedef void (*SetupDrawStateFunc)(unsigned int, bool); // std::function<void(unsigned int, bool)>
		typedef void (*ResetDrawStateFunc)(unsigned int, bool); // std::function<void(unsigned int, bool)>

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
		LuaMaterial* prevMat;

	public:
		static LuaMatHandler handler;
};


/******************************************************************************/
/******************************************************************************/


extern LuaMatHandler& luaMatHandler;


#endif /* LUA_MATERIAL_H */
