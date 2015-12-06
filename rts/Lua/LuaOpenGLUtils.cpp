/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <map>
#include <set>
#include <boost/assign/list_of.hpp>

#include "LuaOpenGLUtils.h"

#include "LuaHandle.h"
#include "LuaTextures.h"
#include "Game/Camera.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/HeightMapTexture.h"
#include "Map/ReadMap.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/IconHandler.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/GeometryBuffer.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Map/InfoTexture/InfoTexture.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitDefImage.h"
#include "System/Matrix44f.h"
#include "System/Util.h"
#include "System/Log/ILog.h"



/******************************************************************************/
/******************************************************************************/

static std::set<std::string> infoTexNames;

/******************************************************************************/
/******************************************************************************/

enum LUAMATRICES {
	LUAMATRICES_SHADOW,
	LUAMATRICES_VIEW,
	LUAMATRICES_VIEWINVERSE,
	LUAMATRICES_PROJECTION,
	LUAMATRICES_PROJECTIONINVERSE,
	LUAMATRICES_VIEWPROJECTION,
	LUAMATRICES_VIEWPROJECTIONINVERSE,
	LUAMATRICES_BILLBOARD,
	LUAMATRICES_NONE
};

static const std::map<std::string, LUAMATRICES> matrixNameToId = boost::assign::map_list_of
	("shadow", LUAMATRICES_SHADOW)
	("view", LUAMATRICES_VIEW)
	("viewinverse", LUAMATRICES_VIEWINVERSE)
	("projection", LUAMATRICES_PROJECTION)
	("projectioninverse", LUAMATRICES_PROJECTIONINVERSE)
	("viewprojection", LUAMATRICES_VIEWPROJECTION)
	("viewprojectioninverse", LUAMATRICES_VIEWPROJECTIONINVERSE)
	("billboard", LUAMATRICES_BILLBOARD)
	// backward compability
	("camera", LUAMATRICES_VIEW)
	("caminv", LUAMATRICES_VIEWINVERSE)
	("camprj", LUAMATRICES_PROJECTION)
	("camprjinv", LUAMATRICES_PROJECTIONINVERSE)
;

inline static LUAMATRICES MatrixGetId(const std::string& name)
{
	const std::map<std::string, LUAMATRICES>::const_iterator it = matrixNameToId.find(name);
	if (it == matrixNameToId.end()) {
		return LUAMATRICES_NONE;
	}
	return it->second;
}

/******************************************************************************/
/******************************************************************************/

const CMatrix44f* LuaOpenGLUtils::GetNamedMatrix(const std::string& name)
{
	//! don't do for performance reasons (this function gets called a lot)
	//StringToLowerInPlace(name);

	const LUAMATRICES mat = MatrixGetId(name);

	switch (mat) {
		case LUAMATRICES_SHADOW:
			if (shadowHandler == NULL) {
				break;
			}
			return &shadowHandler->GetShadowMatrix();
		case LUAMATRICES_VIEW:
			return &camera->GetViewMatrix();
		case LUAMATRICES_VIEWINVERSE:
			return &camera->GetViewMatrixInverse();
		case LUAMATRICES_PROJECTION:
			return &camera->GetProjectionMatrix();
		case LUAMATRICES_PROJECTIONINVERSE:
			return &camera->GetProjectionMatrixInverse();
		case LUAMATRICES_VIEWPROJECTION:
			return &camera->GetViewProjectionMatrix();
		case LUAMATRICES_VIEWPROJECTIONINVERSE:
			return &camera->GetViewProjectionMatrixInverse();
		case LUAMATRICES_BILLBOARD:
			return &camera->GetBillBoardMatrix();
		case LUAMATRICES_NONE:
			break;
	}

	return NULL;
}


bool ParseUnitTexture(LuaMatTexture& texUnit, const std::string& texture)
{
	if (texture.length()<4) {
		return false;
	}

	char* endPtr;
	const char* startPtr = texture.c_str() + 1; // skip the '%'
	const int id = (int)strtol(startPtr, &endPtr, 10);
	if ((endPtr == startPtr) || (*endPtr != ':')) {
		return false;
	}

	endPtr++; // skip the ':'
	if ( (startPtr-1)+texture.length() <= endPtr ) {
		return false; // ':' is end of string, but we expect '%num:0'
	}

	if (id == 0) {
		texUnit.type = LuaMatTexture::LUATEX_3DOTEXTURE;
		if (*endPtr == '0') {
			texUnit.data = reinterpret_cast<const void*>(int(1));
		}
		else if (*endPtr == '1') {
			texUnit.data = reinterpret_cast<const void*>(int(2));
		}
		return true;
	}

	S3DModel* model;

	if (id < 0) {
		const FeatureDef* fd = featureHandler->GetFeatureDefByID(-id);
		if (fd == NULL) {
			return false;
		}
		model = fd->LoadModel();
	} else {
		const UnitDef* ud = unitDefHandler->GetUnitDefByID(id);
		if (ud == NULL) {
			return false;
		}
		model = ud->LoadModel();
	}

	if (model == NULL) {
		return false;
	}

	const unsigned int texType = model->textureType;
	if (texType == 0) {
		return false;
	}

	const CS3OTextureHandler::S3oTex* stex = texturehandlerS3O->GetS3oTex(texType);
	if (stex == NULL) {
		return false;
	}

	if (*endPtr == '0') {
		texUnit.type = LuaMatTexture::LUATEX_UNITTEXTURE1;
		texUnit.data = reinterpret_cast<const void*>(stex);
	}
	else if (*endPtr == '1') {
		texUnit.type = LuaMatTexture::LUATEX_UNITTEXTURE2;
		texUnit.data = reinterpret_cast<const void*>(stex);
	}

	return true;
}


bool LuaOpenGLUtils::ParseTextureImage(lua_State* L, LuaMatTexture& texUnit, const std::string& image)
{
	// NOTE: current formats:
	//
	// #12          --  unitDef 12 buildpic
	// %34:0        --  unitDef 34 s3o tex1
	// %-34:1       --  featureDef 34 s3o tex2
	// !56          --  lua generated texture 56
	// $shadow      --  shadowmap
	// $specular    --  specular cube map
	// $reflection  --  reflection cube map
	// $heightmap   --  ground heightmap
	// ...          --  named textures
	//

	texUnit.type   = LuaMatTexture::LUATEX_NONE;
	texUnit.enable = false;
	texUnit.data   = 0;

	if (image.empty()) {
		return false;
	}

	if (image[0] == LuaTextures::prefix) {
		if (L == nullptr)
			return false;
		// dynamic texture
		LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
		const LuaTextures::Texture* texInfo = textures.GetInfo(image);
		if (texInfo == NULL) {
			return false;
		}
		texUnit.type = LuaMatTexture::LUATEX_LUATEXTURE;
		texUnit.data = reinterpret_cast<const void*>(texInfo);
	}
	else if (image[0] == '%') {
		return ParseUnitTexture(texUnit, image);
	}
	else if (image[0] == '#') {
		// unit build picture
		char* endPtr;
		const char* startPtr = image.c_str() + 1; // skip the '#'
		const int unitDefID = (int)strtol(startPtr, &endPtr, 10);
		if (endPtr == startPtr) {
			return false;
		}
		const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
		if (ud == NULL) {
			return false;
		}
		texUnit.type = LuaMatTexture::LUATEX_UNITBUILDPIC;
		texUnit.data = reinterpret_cast<const void*>(ud);
	}
	else if (image[0] == '^') {
		// unit icon
		char* endPtr;
		const char* startPtr = image.c_str() + 1; // skip the '^'
		const int unitDefID = (int)strtol(startPtr, &endPtr, 10);
		if (endPtr == startPtr) {
			return false;
		}
		const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
		if (ud == NULL) {
			return false;
		}
		texUnit.type = LuaMatTexture::LUATEX_UNITRADARICON;
		texUnit.data = reinterpret_cast<const void*>(ud);
	}
	else if (image[0] == '$') {
		if (image == "$units" || image == "$units1") {
			texUnit.type = LuaMatTexture::LUATEX_3DOTEXTURE;
			texUnit.data = reinterpret_cast<const void*>(int(1));
		}
		else if (image == "$units2") {
			texUnit.type = LuaMatTexture::LUATEX_3DOTEXTURE;
			texUnit.data = reinterpret_cast<const void*>(int(2));
		}
		else if (image == "$shadow") {
			texUnit.type = LuaMatTexture::LUATEX_SHADOWMAP;
		}
		else if (image == "$specular") {
			texUnit.type = LuaMatTexture::LUATEX_SPECULAR;
		}
		else if (image == "$reflection") {
			texUnit.type = LuaMatTexture::LUATEX_REFLECTION;
		}
		else if (image == "$heightmap") {
			texUnit.type = LuaMatTexture::LUATEX_HEIGHTMAP;

			if (heightMapTexture->GetTextureID() == 0) {
				// it's optional, return false when not available
				return false;
			}
		}
		else if (image == "$shading") {
			texUnit.type = LuaMatTexture::LUATEX_SHADING;
		}
		else if (image == "$grass") {
			texUnit.type = LuaMatTexture::LUATEX_GRASS;
		}
		else if (image == "$minimap") {
			texUnit.type = LuaMatTexture::LUATEX_MINIMAP;
		}

		else if (image.find("$info:") == 0) {
			const std::string infoTexName = image.substr(6);
			CInfoTexture* itex = infoTextureHandler->GetInfoTexture(infoTexName);
			if (!itex)
				return false;

			infoTexNames.insert(infoTexName);
			texUnit.type = LuaMatTexture::LUATEX_INFOTEX;
			texUnit.data = &*infoTexNames.find(infoTexName);
		}

		else if (image == "$info"        || image == "$extra"       ) { texUnit.type = LuaMatTexture::LUATEX_INFOTEX_ACTIVE; }
		else if (image == "$info_losmap" || image == "$extra_losmap") { texUnit.type = LuaMatTexture::LUATEX_INFOTEX_LOSMAP; }
		else if (image == "$info_mtlmap" || image == "$extra_mtlmap") { texUnit.type = LuaMatTexture::LUATEX_INFOTEX_MTLMAP; }
		else if (image == "$info_hgtmap" || image == "$extra_hgtmap") { texUnit.type = LuaMatTexture::LUATEX_INFOTEX_HGTMAP; }
		else if (image == "$info_blkmap" || image == "$extra_blkmap") { texUnit.type = LuaMatTexture::LUATEX_INFOTEX_BLKMAP; }

		else if (image == "$map_gb_nt" || image == "$map_gbuffer_normtex") { texUnit.type = LuaMatTexture::LUATEX_MAP_GBUFFER_NORMTEX; }
		else if (image == "$map_gb_dt" || image == "$map_gbuffer_difftex") { texUnit.type = LuaMatTexture::LUATEX_MAP_GBUFFER_DIFFTEX; }
		else if (image == "$map_gb_st" || image == "$map_gbuffer_spectex") { texUnit.type = LuaMatTexture::LUATEX_MAP_GBUFFER_SPECTEX; }
		else if (image == "$map_gb_et" || image == "$map_gbuffer_emittex") { texUnit.type = LuaMatTexture::LUATEX_MAP_GBUFFER_EMITTEX; }
		else if (image == "$map_gb_mt" || image == "$map_gbuffer_misctex") { texUnit.type = LuaMatTexture::LUATEX_MAP_GBUFFER_MISCTEX; }
		else if (image == "$map_gb_zt" || image == "$map_gbuffer_zvaltex") { texUnit.type = LuaMatTexture::LUATEX_MAP_GBUFFER_ZVALTEX; }

		else if (image == "$mdl_gb_nt" || image == "$model_gbuffer_normtex") { texUnit.type = LuaMatTexture::LUATEX_MODEL_GBUFFER_NORMTEX; }
		else if (image == "$mdl_gb_dt" || image == "$model_gbuffer_difftex") { texUnit.type = LuaMatTexture::LUATEX_MODEL_GBUFFER_DIFFTEX; }
		else if (image == "$mdl_gb_st" || image == "$model_gbuffer_spectex") { texUnit.type = LuaMatTexture::LUATEX_MODEL_GBUFFER_SPECTEX; }
		else if (image == "$mdl_gb_et" || image == "$model_gbuffer_emittex") { texUnit.type = LuaMatTexture::LUATEX_MODEL_GBUFFER_EMITTEX; }
		else if (image == "$mdl_gb_mt" || image == "$model_gbuffer_misctex") { texUnit.type = LuaMatTexture::LUATEX_MODEL_GBUFFER_MISCTEX; }
		else if (image == "$mdl_gb_zt" || image == "$model_gbuffer_zvaltex") { texUnit.type = LuaMatTexture::LUATEX_MODEL_GBUFFER_ZVALTEX; }

		else if (image == "$font") {
			texUnit.type = LuaMatTexture::LUATEX_FONT;
		}
		else if (image == "$smallfont" || image == "$fontsmall") {
			texUnit.type = LuaMatTexture::LUATEX_FONTSMALL;
		}
		else {
			return false;
		}
	}
	else {
		const CNamedTextures::TexInfo* texInfo = CNamedTextures::GetInfo(image, true);
		if (texInfo != NULL) {
			texUnit.type = LuaMatTexture::LUATEX_NAMED;
			texUnit.data = reinterpret_cast<const void*>(texInfo);
		} else {
			LOG_L(L_WARNING, "Lua: Couldn't load texture named \"%s\"!", image.c_str());
			return false;
		}
	}

	return true;
}


GLuint LuaMatTexture::GetTextureID() const
{
	GLuint texID = 0;

	#define groundDrawer (readMap->GetGroundDrawer())
	#define gdGeomBuff (groundDrawer->GetGeometryBuffer())
	#define udGeomBuff (unitDrawer->GetGeometryBuffer())

	switch (type) {
		case LUATEX_NONE: {
		} break;
		case LUATEX_NAMED: {
			texID = reinterpret_cast<const CNamedTextures::TexInfo*>(data)->id;
		} break;
		case LUATEX_LUATEXTURE: {
			texID = reinterpret_cast<const LuaTextures::Texture*>(data)->id;
		} break;
		case LUATEX_UNITTEXTURE1: {
			auto stex = reinterpret_cast<const CS3OTextureHandler::S3oTex*>(data);
			texID = stex->tex1;
		} break;
		case LUATEX_UNITTEXTURE2: {
			auto stex = reinterpret_cast<const CS3OTextureHandler::S3oTex*>(data);
			texID = stex->tex2;
		} break;
		case LUATEX_3DOTEXTURE: if (texturehandler3DO != nullptr) {
			if (*reinterpret_cast<const int*>(&data) == 1) {
				texID = texturehandler3DO->GetAtlasTex1ID();
			} else {
				texID = texturehandler3DO->GetAtlasTex2ID();
			}
		} break;
		case LUATEX_UNITBUILDPIC: if (unitDefHandler != nullptr) {
			auto ud = reinterpret_cast<const UnitDef*>(data);
			texID = unitDefHandler->GetUnitDefImage(ud);
		} break;
		case LUATEX_UNITRADARICON: {
			auto ud = reinterpret_cast<const UnitDef*>(data);
			texID = ud->iconType->GetTextureID();
		} break;
		case LUATEX_SHADOWMAP: if (shadowHandler != nullptr) {
			texID = shadowHandler->shadowTexture;
		} break;
		case LUATEX_REFLECTION: if (cubeMapHandler != nullptr) {
			texID = cubeMapHandler->GetEnvReflectionTextureID();
		} break;
		case LUATEX_SPECULAR: if (cubeMapHandler != nullptr) {
			texID = cubeMapHandler->GetSpecularTextureID();
		} break;
		case LUATEX_HEIGHTMAP: if (heightMapTexture) {
			texID = heightMapTexture->GetTextureID();
		} break;
		case LUATEX_SHADING: {
			texID = (readMap != NULL)? readMap->GetShadingTexture(): 0;
		} break;
		case LUATEX_GRASS: {
			texID = (readMap != NULL)? readMap->GetGrassShadingTexture(): 0;
		} break;
		case LUATEX_FONT: {
			texID = font->GetTexture();
		} break;
		case LUATEX_FONTSMALL: {
			texID = smallFont->GetTexture();
		} break;
		case LUATEX_MINIMAP: {
			texID = (readMap != NULL)? readMap->GetMiniMapTexture(): 0;
		} break;

		case LUATEX_INFOTEX: if (infoTextureHandler != nullptr) {
			CInfoTexture* itex = infoTextureHandler->GetInfoTexture(*static_cast<const std::string*>(data));
			texID = (itex) ? itex->GetTexture() : 0;
		} break;

		case LUATEX_INFOTEX_ACTIVE: if (infoTextureHandler != nullptr) {
			texID = infoTextureHandler->GetCurrentInfoTexture();
		} break;
		case LUATEX_INFOTEX_LOSMAP: if (infoTextureHandler != nullptr) {
			CInfoTexture* itex = infoTextureHandler->GetInfoTexture("los");
			texID = (itex) ? itex->GetTexture() : 0;
		} break;
		case LUATEX_INFOTEX_MTLMAP: if (infoTextureHandler != nullptr) {
			CInfoTexture* itex = infoTextureHandler->GetInfoTexture("metal");
			texID = (itex) ? itex->GetTexture() : 0;
		} break;
		case LUATEX_INFOTEX_HGTMAP: if (infoTextureHandler != nullptr) {
			CInfoTexture* itex = infoTextureHandler->GetInfoTexture("height");
			texID = (itex) ? itex->GetTexture() : 0;
		} break;
		case LUATEX_INFOTEX_BLKMAP: if (infoTextureHandler != nullptr) {
			CInfoTexture* itex = infoTextureHandler->GetInfoTexture("path");
			texID = (itex) ? itex->GetTexture() : 0;
		} break;

		case LUATEX_MAP_GBUFFER_NORMTEX: { texID = gdGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_NORMTEX); } break;
		case LUATEX_MAP_GBUFFER_DIFFTEX: { texID = gdGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_DIFFTEX); } break;
		case LUATEX_MAP_GBUFFER_SPECTEX: { texID = gdGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_SPECTEX); } break;
		case LUATEX_MAP_GBUFFER_EMITTEX: { texID = gdGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_EMITTEX); } break;
		case LUATEX_MAP_GBUFFER_MISCTEX: { texID = gdGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_MISCTEX); } break;
		case LUATEX_MAP_GBUFFER_ZVALTEX: { texID = gdGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_ZVALTEX); } break;

		case LUATEX_MODEL_GBUFFER_NORMTEX: { texID = udGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_NORMTEX); } break;
		case LUATEX_MODEL_GBUFFER_DIFFTEX: { texID = udGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_DIFFTEX); } break;
		case LUATEX_MODEL_GBUFFER_SPECTEX: { texID = udGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_SPECTEX); } break;
		case LUATEX_MODEL_GBUFFER_EMITTEX: { texID = udGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_EMITTEX); } break;
		case LUATEX_MODEL_GBUFFER_MISCTEX: { texID = udGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_MISCTEX); } break;
		case LUATEX_MODEL_GBUFFER_ZVALTEX: { texID = udGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_ZVALTEX); } break;

		default:
			assert(false);
	}

	#undef udGeomBuff
	#undef gdGeomBuff
	#undef groundDrawer

	return texID;
}


GLuint LuaMatTexture::GetTextureTarget() const
{
	GLuint texType = GL_TEXTURE_2D;

	switch (type) {
		case LUATEX_NAMED: {
			texType = GL_TEXTURE_2D; //FIXME allow lua to load cubemaps!
		} break;
		case LUATEX_LUATEXTURE: {
			texType = reinterpret_cast<const LuaTextures::Texture*>(data)->target;
		} break;
		case LUATEX_REFLECTION:
		case LUATEX_SPECULAR: {
			texType = GL_TEXTURE_CUBE_MAP_ARB;
		} break;

		case LUATEX_NONE:
		case LUATEX_UNITTEXTURE1:
		case LUATEX_UNITTEXTURE2:
		case LUATEX_3DOTEXTURE:
		case LUATEX_UNITBUILDPIC:
		case LUATEX_UNITRADARICON:
		case LUATEX_SHADOWMAP:
		case LUATEX_HEIGHTMAP:
		case LUATEX_SHADING:
		case LUATEX_GRASS:
		case LUATEX_FONT:
		case LUATEX_FONTSMALL:
		case LUATEX_MINIMAP:

		case LUATEX_INFOTEX:
		case LUATEX_INFOTEX_ACTIVE:
		case LUATEX_INFOTEX_LOSMAP:
		case LUATEX_INFOTEX_MTLMAP:
		case LUATEX_INFOTEX_HGTMAP:
		case LUATEX_INFOTEX_BLKMAP: {
			texType = GL_TEXTURE_2D;
		} break;

		case LUATEX_MAP_GBUFFER_NORMTEX:
		case LUATEX_MAP_GBUFFER_DIFFTEX:
		case LUATEX_MAP_GBUFFER_SPECTEX:
		case LUATEX_MAP_GBUFFER_EMITTEX:
		case LUATEX_MAP_GBUFFER_MISCTEX:
		case LUATEX_MAP_GBUFFER_ZVALTEX: {
			texType = GL_TEXTURE_2D;
		} break;

		case LUATEX_MODEL_GBUFFER_NORMTEX:
		case LUATEX_MODEL_GBUFFER_DIFFTEX:
		case LUATEX_MODEL_GBUFFER_SPECTEX:
		case LUATEX_MODEL_GBUFFER_EMITTEX:
		case LUATEX_MODEL_GBUFFER_MISCTEX:
		case LUATEX_MODEL_GBUFFER_ZVALTEX: {
			texType = GL_TEXTURE_2D;
		} break;

		default:
			assert(false);
	}

	return texType;
}


void LuaMatTexture::Bind() const
{
	const GLuint texID = GetTextureID();
	const GLuint texType = GetTextureTarget();

	if (texID != 0) {
		glBindTexture(texType, texID);
		if (enable && (texType == GL_TEXTURE_2D)) {
			glEnable(GL_TEXTURE_2D);
		}
	} else if (!enable && (texType == GL_TEXTURE_2D)) {
		glDisable(GL_TEXTURE_2D);
	}

	if (enableTexParams && type == LUATEX_SHADOWMAP) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
	}
}


void LuaMatTexture::Unbind() const
{
	if (type == LUATEX_NONE)
		return;

	if (enableTexParams && type == LUATEX_SHADOWMAP) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
	}

	if (!enable)
		return;

	const GLuint texType = GetTextureTarget();
	if (texType == GL_TEXTURE_2D)
		glDisable(GL_TEXTURE_2D);
}


int2 LuaMatTexture::GetSize() const
{
	#define sqint2(x) int2(x, x)
	switch (type) {
		case LUATEX_NAMED: {
			const auto namedTex = reinterpret_cast<const CNamedTextures::TexInfo*>(data);
			return int2(namedTex->xsize, namedTex->ysize);
		} break;
		case LUATEX_LUATEXTURE: {
			const auto luaTex = reinterpret_cast<const LuaTextures::Texture*>(data);
			return int2(luaTex->xsize, luaTex->ysize);
		} break;
		case LUATEX_UNITTEXTURE1: {
			const auto stex = reinterpret_cast<const CS3OTextureHandler::S3oTex*>(data);
			return int2(stex->tex1SizeX, stex->tex1SizeY);
		} break;
		case LUATEX_UNITTEXTURE2: {
			const auto stex = reinterpret_cast<const CS3OTextureHandler::S3oTex*>(data);
			return int2(stex->tex2SizeX, stex->tex2SizeY);
		} break;
		case LUATEX_3DOTEXTURE: if (texturehandler3DO != nullptr) {
			return int2(texturehandler3DO->GetAtlasTexSizeX(), texturehandler3DO->GetAtlasTexSizeY());
		} break;
		case LUATEX_UNITBUILDPIC: if (unitDefHandler != nullptr) {
			const auto ud = reinterpret_cast<const UnitDef*>(data);
			unitDefHandler->GetUnitDefImage(ud); // forced existance
			return int2(ud->buildPic->imageSizeX, ud->buildPic->imageSizeY);
		} break;
		case LUATEX_UNITRADARICON: {
			const auto ud = reinterpret_cast<const UnitDef*>(data);
			return int2(ud->iconType->GetSizeX(), ud->iconType->GetSizeY());
		} break;
		case LUATEX_SHADOWMAP: if (shadowHandler != nullptr) {
			return sqint2(shadowHandler->shadowMapSize);
		} break;
		case LUATEX_REFLECTION: if (cubeMapHandler != nullptr) {
			return sqint2(cubeMapHandler->GetReflectionTextureSize());
		} break;
		case LUATEX_SPECULAR: if (cubeMapHandler != nullptr) {
			return sqint2(cubeMapHandler->GetSpecularTextureSize());
		} break;
		case LUATEX_SHADING: if (gs != nullptr) {
			return int2(mapDims.pwr2mapx, mapDims.pwr2mapy);
		} break;
		case LUATEX_GRASS:
			return int2(1024, 1024);
		case LUATEX_FONT:
			return int2(font->GetTextureWidth(), font->GetTextureHeight());
		case LUATEX_FONTSMALL:
			return int2(smallFont->GetTextureWidth(), smallFont->GetTextureHeight());
		case LUATEX_MINIMAP: if (readMap != NULL) {
			return readMap->GetMiniMapTextureSize();
		} break;

		case LUATEX_INFOTEX: if (infoTextureHandler != nullptr) {
			CInfoTexture* itex = infoTextureHandler->GetInfoTexture(*static_cast<const std::string*>(data));
			if (itex) {
				return itex->GetTexSize();
			}
		} break;
		case LUATEX_INFOTEX_ACTIVE: if (infoTextureHandler != nullptr) {
			return infoTextureHandler->GetCurrentInfoTextureSize();
		} break;
		case LUATEX_INFOTEX_LOSMAP: if (infoTextureHandler != nullptr) {
			CInfoTexture* itex = infoTextureHandler->GetInfoTexture("los");
			if (itex) {
				return itex->GetTexSize();
			}
		} break;
		case LUATEX_INFOTEX_MTLMAP: if (infoTextureHandler != nullptr) {
			CInfoTexture* itex = infoTextureHandler->GetInfoTexture("metal");
			if (itex) {
				return itex->GetTexSize();
			}
		} break;
		case LUATEX_INFOTEX_HGTMAP: if (infoTextureHandler != nullptr) {
			CInfoTexture* itex = infoTextureHandler->GetInfoTexture("height");
			if (itex) {
				return itex->GetTexSize();
			}
		} break;
		case LUATEX_INFOTEX_BLKMAP: if (infoTextureHandler != nullptr) {
			CInfoTexture* itex = infoTextureHandler->GetInfoTexture("path");
			if (itex) {
				return itex->GetTexSize();
			}
		} break;

		case LUATEX_MAP_GBUFFER_NORMTEX:
		case LUATEX_MAP_GBUFFER_DIFFTEX:
		case LUATEX_MAP_GBUFFER_SPECTEX:
		case LUATEX_MAP_GBUFFER_EMITTEX:
		case LUATEX_MAP_GBUFFER_MISCTEX:
		case LUATEX_MAP_GBUFFER_ZVALTEX: {
			if (readMap != NULL) {
				return (readMap->GetGroundDrawer()->GetGeometryBuffer()->GetWantedSize(readMap->GetGroundDrawer()->DrawDeferred()));
			}
		} break;

		case LUATEX_MODEL_GBUFFER_NORMTEX:
		case LUATEX_MODEL_GBUFFER_DIFFTEX:
		case LUATEX_MODEL_GBUFFER_SPECTEX:
		case LUATEX_MODEL_GBUFFER_EMITTEX:
		case LUATEX_MODEL_GBUFFER_MISCTEX:
		case LUATEX_MODEL_GBUFFER_ZVALTEX: {
			if (unitDrawer != NULL) {
				return (unitDrawer->GetGeometryBuffer()->GetWantedSize(unitDrawer->DrawDeferred()));
			}
		} break;

		case LUATEX_HEIGHTMAP: {
			if (heightMapTexture != NULL) {
				return int2(heightMapTexture->GetSizeX(), heightMapTexture->GetSizeY());
			}
		} break;

		case LUATEX_NONE:
		default: break;
	}

	return int2(0, 0);
}



/******************************************************************************/
/******************************************************************************/
//
//  LuaMatTexture
//

void LuaMatTexture::Finalize()
{
	//if (type == LUATEX_NONE) {
	//	enable = false;
	//}
}


int LuaMatTexture::Compare(const LuaMatTexture& a, const LuaMatTexture& b)
{
	if (a.type != b.type) {
		return (a.type < b.type) ? -1 : +1;
	}
	if (a.data != b.data) {
		return (a.data < b.data) ? -1 : +1;
	}
	if (a.enable != b.enable) {
		return a.enable ? -1 : +1;
	}
	return 0;
}


void LuaMatTexture::Print(const string& indent) const
{
	const char* typeName = "Unknown";

	switch (type) {
		#define STRING_CASE(ptr, x) case x: ptr = #x; break;
		STRING_CASE(typeName, LUATEX_NONE);
		STRING_CASE(typeName, LUATEX_NAMED);
		STRING_CASE(typeName, LUATEX_LUATEXTURE);
		STRING_CASE(typeName, LUATEX_UNITTEXTURE1);
		STRING_CASE(typeName, LUATEX_UNITTEXTURE2);
		STRING_CASE(typeName, LUATEX_3DOTEXTURE);
		STRING_CASE(typeName, LUATEX_UNITBUILDPIC);
		STRING_CASE(typeName, LUATEX_UNITRADARICON);
		STRING_CASE(typeName, LUATEX_SHADOWMAP);
		STRING_CASE(typeName, LUATEX_REFLECTION);
		STRING_CASE(typeName, LUATEX_SPECULAR);
		STRING_CASE(typeName, LUATEX_HEIGHTMAP);
		STRING_CASE(typeName, LUATEX_SHADING);
		STRING_CASE(typeName, LUATEX_GRASS);
		STRING_CASE(typeName, LUATEX_FONT);
		STRING_CASE(typeName, LUATEX_FONTSMALL);
		STRING_CASE(typeName, LUATEX_MINIMAP);

		STRING_CASE(typeName, LUATEX_INFOTEX);
		STRING_CASE(typeName, LUATEX_INFOTEX_ACTIVE);
		STRING_CASE(typeName, LUATEX_INFOTEX_LOSMAP);
		STRING_CASE(typeName, LUATEX_INFOTEX_MTLMAP);
		STRING_CASE(typeName, LUATEX_INFOTEX_HGTMAP);
		STRING_CASE(typeName, LUATEX_INFOTEX_BLKMAP);

		STRING_CASE(typeName, LUATEX_MAP_GBUFFER_NORMTEX);
		STRING_CASE(typeName, LUATEX_MAP_GBUFFER_DIFFTEX);
		STRING_CASE(typeName, LUATEX_MAP_GBUFFER_SPECTEX);
		STRING_CASE(typeName, LUATEX_MAP_GBUFFER_EMITTEX);
		STRING_CASE(typeName, LUATEX_MAP_GBUFFER_MISCTEX);
		STRING_CASE(typeName, LUATEX_MAP_GBUFFER_ZVALTEX);

		STRING_CASE(typeName, LUATEX_MODEL_GBUFFER_NORMTEX);
		STRING_CASE(typeName, LUATEX_MODEL_GBUFFER_DIFFTEX);
		STRING_CASE(typeName, LUATEX_MODEL_GBUFFER_SPECTEX);
		STRING_CASE(typeName, LUATEX_MODEL_GBUFFER_EMITTEX);
		STRING_CASE(typeName, LUATEX_MODEL_GBUFFER_MISCTEX);
		STRING_CASE(typeName, LUATEX_MODEL_GBUFFER_ZVALTEX);
		#undef STRING_CASE
	}

	LOG("%s%s %i %s", indent.c_str(), typeName, *reinterpret_cast<const GLuint*>(&data), (enable ? "true" : "false"));
}


/******************************************************************************/
/******************************************************************************/
