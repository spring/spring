/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_MATERIAL_H
#define LUA_MATERIAL_H

#include <string>
#include <vector>
#include <set>
using std::string;
using std::vector;
using std::set;

#include "LuaOpenGLUtils.h"
#include "LuaUnitMaterial.h" // for LuaMatRef

#include "Rendering/GL/myGL.h"
#include "Rendering/ShadowHandler.h"

class CUnit;


class LuaMatTexSetRef;
class LuaMatUnifSetRef;


/******************************************************************************/
/******************************************************************************/

class LuaMatShader {
	public:
		enum Type {
			LUASHADER_NONE = 0,
			LUASHADER_GL   = 1,
			LUASHADER_3DO  = 2,
			LUASHADER_S3O  = 3
		};

	public:
		LuaMatShader() : type(LUASHADER_NONE), openglID(0) {}

		void Finalize();

		void Execute(const LuaMatShader& prev, bool deferredPass) const;

		void Print(const string& indent) const;

		static int Compare(const LuaMatShader& a, const LuaMatShader& b);
		bool operator <(const LuaMatShader& mt) const {
			return Compare(*this, mt)  < 0;
		}
		bool operator==(const LuaMatShader& mt) const {
			return Compare(*this, mt) == 0;
		}
		bool operator!=(const LuaMatShader& mt) const {
			return Compare(*this, mt) != 0;
		}

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
		LuaMaterial()
		: type(LuaMatType(-1)), // invalid
		  order(0), texCount(0),
		  preList(0), postList(0),
		  useCamera(true), culling(0),
		  cameraLoc(-1), cameraInvLoc(-1), cameraPosLoc(-1),
		  sunPosLoc(-1), shadowLoc(-1), shadowParamsLoc(-1)
		{}

		void Finalize();

		void Execute(const LuaMaterial& prev, bool deferredPass) const;

		void Print(const string& indent) const;

		static int Compare(const LuaMaterial& a, const LuaMaterial& b);
		bool operator <(const LuaMaterial& m) const {
			return Compare(*this, m)  < 0;
		}
		bool operator==(const LuaMaterial& m) const {
			return Compare(*this, m) == 0;
		}
		bool operator!=(const LuaMaterial& m) const {
			return Compare(*this, m) != 0;
		}

	public:
		LuaMatType type;
		int order; // for manually adjusting rendering order

		LuaMatShader standardShader;
		LuaMatShader deferredShader;

		int texCount;
		LuaMatTexture textures[LuaMatTexture::maxTexUnits];

		GLuint preList;
		GLuint postList;

		bool useCamera;
		GLenum culling;
		GLint cameraLoc;       // view matrix
		GLint cameraInvLoc;    // inverse view matrix
		GLint cameraPosLoc;
		GLint sunPosLoc;
		GLint shadowLoc;       // shadow matrix
		GLint shadowParamsLoc;

		static const LuaMaterial defMat;
};


/******************************************************************************/
/******************************************************************************/

class LuaMatBin : public LuaMaterial {

	friend class LuaMatHandler;

	public:
		void Clear() { units.clear(); }
		const std::vector<CUnit*>& GetUnits() const { return units; }

		void Ref();
		void UnRef();

		void AddUnit(CUnit* unit) { units.push_back(unit); }

		void Print(const string& indent) const;

	private:
		LuaMatBin(const LuaMaterial& _mat) : LuaMaterial(_mat), refCount(0) {}
		LuaMatBin(const LuaMatBin&);
		LuaMatBin& operator=(const LuaMatBin&);

	private:
		int refCount;
		std::vector<CUnit*> units;
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

		void ClearBins();
		void ClearBins(LuaMatType type);

		void FreeBin(LuaMatBin* bin);

		void PrintBins(const string& indent, LuaMatType type) const;
		void PrintAllBins(const string& indent) const;

	public:
		void (*setup3doShader)(bool);
		void (*reset3doShader)(bool);
		void (*setupS3oShader)(bool);
		void (*resetS3oShader)(bool);

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
