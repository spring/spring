/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <map>
#include <set>

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

static const std::unordered_map<std::string, LuaMatTexture::Type> texNameToTypeMap = {
	// atlases
	{"$units",  LuaMatTexture::LUATEX_3DOTEXTURE},
	{"$units1", LuaMatTexture::LUATEX_3DOTEXTURE},
	{"$units2", LuaMatTexture::LUATEX_3DOTEXTURE},

	// cubemaps
	{"$specular", LuaMatTexture::LUATEX_SPECULAR},
	{"$reflection", LuaMatTexture::LUATEX_MAP_REFLECTION},
	{"$map_reflection", LuaMatTexture::LUATEX_MAP_REFLECTION},
	{"$sky_reflection", LuaMatTexture::LUATEX_SKY_REFLECTION},

	// specials
	{"$shadow", LuaMatTexture::LUATEX_SHADOWMAP},
	{"$heightmap", LuaMatTexture::LUATEX_HEIGHTMAP},

	// SMF-maps
	{"$grass", LuaMatTexture::LUATEX_SMF_GRASS},
	{"$detail", LuaMatTexture::LUATEX_SMF_DETAIL},
	{"$minimap", LuaMatTexture::LUATEX_SMF_MINIMAP},
	{"$shading", LuaMatTexture::LUATEX_SMF_SHADING},
	{"$normals", LuaMatTexture::LUATEX_SMF_NORMALS},
	// SSMF-maps
	{"$ssmf_normals",       LuaMatTexture::LUATEX_SSMF_NORMALS },
	{"$ssmf_specular",      LuaMatTexture::LUATEX_SSMF_SPECULAR},
	{"$ssmf_splat_distr",   LuaMatTexture::LUATEX_SSMF_SDISTRIB},
	{"$ssmf_splat_detail",  LuaMatTexture::LUATEX_SSMF_SDETAIL },
	{"$ssmf_splat_normals", LuaMatTexture::LUATEX_SSMF_SNORMALS},
	{"$ssmf_sky_refl",      LuaMatTexture::LUATEX_SSMF_SKYREFL },
	{"$ssmf_emission",      LuaMatTexture::LUATEX_SSMF_EMISSION},
	{"$ssmf_parallax",      LuaMatTexture::LUATEX_SSMF_PARALLAX},


	{"$info",        LuaMatTexture::LUATEX_INFOTEX_ACTIVE},
	{"$info_losmap", LuaMatTexture::LUATEX_INFOTEX_LOSMAP},
	{"$info_mtlmap", LuaMatTexture::LUATEX_INFOTEX_MTLMAP},
	{"$info_hgtmap", LuaMatTexture::LUATEX_INFOTEX_HGTMAP},
	{"$info_blkmap", LuaMatTexture::LUATEX_INFOTEX_BLKMAP},

	{"$extra",        LuaMatTexture::LUATEX_INFOTEX_ACTIVE},
	{"$extra_losmap", LuaMatTexture::LUATEX_INFOTEX_LOSMAP},
	{"$extra_mtlmap", LuaMatTexture::LUATEX_INFOTEX_MTLMAP},
	{"$extra_hgtmap", LuaMatTexture::LUATEX_INFOTEX_HGTMAP},
	{"$extra_blkmap", LuaMatTexture::LUATEX_INFOTEX_BLKMAP},

	{"$map_gb_nt", LuaMatTexture::LUATEX_MAP_GBUFFER_NORM},
	{"$map_gb_dt", LuaMatTexture::LUATEX_MAP_GBUFFER_DIFF},
	{"$map_gb_st", LuaMatTexture::LUATEX_MAP_GBUFFER_SPEC},
	{"$map_gb_et", LuaMatTexture::LUATEX_MAP_GBUFFER_EMIT},
	{"$map_gb_mt", LuaMatTexture::LUATEX_MAP_GBUFFER_MISC},
	{"$map_gb_zt", LuaMatTexture::LUATEX_MAP_GBUFFER_ZVAL},

	{"$map_gbuffer_normtex", LuaMatTexture::LUATEX_MAP_GBUFFER_NORM},
	{"$map_gbuffer_difftex", LuaMatTexture::LUATEX_MAP_GBUFFER_DIFF},
	{"$map_gbuffer_spectex", LuaMatTexture::LUATEX_MAP_GBUFFER_SPEC},
	{"$map_gbuffer_emittex", LuaMatTexture::LUATEX_MAP_GBUFFER_EMIT},
	{"$map_gbuffer_misctex", LuaMatTexture::LUATEX_MAP_GBUFFER_MISC},
	{"$map_gbuffer_zvaltex", LuaMatTexture::LUATEX_MAP_GBUFFER_ZVAL},

	{"$mdl_gb_nt", LuaMatTexture::LUATEX_MODEL_GBUFFER_NORM},
	{"$mdl_gb_dt", LuaMatTexture::LUATEX_MODEL_GBUFFER_DIFF},
	{"$mdl_gb_st", LuaMatTexture::LUATEX_MODEL_GBUFFER_SPEC},
	{"$mdl_gb_et", LuaMatTexture::LUATEX_MODEL_GBUFFER_EMIT},
	{"$mdl_gb_mt", LuaMatTexture::LUATEX_MODEL_GBUFFER_MISC},
	{"$mdl_gb_zt", LuaMatTexture::LUATEX_MODEL_GBUFFER_ZVAL},

	{"$model_gbuffer_normtex", LuaMatTexture::LUATEX_MODEL_GBUFFER_NORM},
	{"$model_gbuffer_difftex", LuaMatTexture::LUATEX_MODEL_GBUFFER_DIFF},
	{"$model_gbuffer_spectex", LuaMatTexture::LUATEX_MODEL_GBUFFER_SPEC},
	{"$model_gbuffer_emittex", LuaMatTexture::LUATEX_MODEL_GBUFFER_EMIT},
	{"$model_gbuffer_misctex", LuaMatTexture::LUATEX_MODEL_GBUFFER_MISC},
	{"$model_gbuffer_zvaltex", LuaMatTexture::LUATEX_MODEL_GBUFFER_ZVAL},

	{"$font"     , LuaMatTexture::LUATEX_FONT},
	{"$smallfont", LuaMatTexture::LUATEX_FONTSMALL},
	{"$fontsmall", LuaMatTexture::LUATEX_FONTSMALL},

};

static const std::unordered_map<std::string, LuaMatrixType> matrixNameToTypeMap = {
	{"shadow", LUAMATRICES_SHADOW},
	{"view", LUAMATRICES_VIEW},
	{"viewinverse", LUAMATRICES_VIEWINVERSE},
	{"projection", LUAMATRICES_PROJECTION},
	{"projectioninverse", LUAMATRICES_PROJECTIONINVERSE},
	{"viewprojection", LUAMATRICES_VIEWPROJECTION},
	{"viewprojectioninverse", LUAMATRICES_VIEWPROJECTIONINVERSE},
	{"billboard", LUAMATRICES_BILLBOARD},
	// backward compability
	{"camera", LUAMATRICES_VIEW},
	{"caminv", LUAMATRICES_VIEWINVERSE},
	{"camprj", LUAMATRICES_PROJECTION},
	{"camprjinv", LUAMATRICES_PROJECTIONINVERSE},
};


inline static LuaMatTexture::Type GetLuaMatTextureType(const std::string& name)
{
	const auto it = texNameToTypeMap.find(name);

	if (it == texNameToTypeMap.end())
		return LuaMatTexture::LUATEX_NONE;

	return it->second;
}

inline static LuaMatrixType GetLuaMatrixType(const std::string& name)
{
	const auto it = matrixNameToTypeMap.find(name);

	if (it == matrixNameToTypeMap.end())
		return LUAMATRICES_NONE;

	return it->second;
}

/******************************************************************************/
/******************************************************************************/

const CMatrix44f* LuaOpenGLUtils::GetNamedMatrix(const std::string& name)
{
	//! don't do for performance reasons (this function gets called a lot)
	//StringToLowerInPlace(name);

	switch (GetLuaMatrixType(name)) {
		case LUAMATRICES_SHADOW:
			if (shadowHandler == nullptr)
				break;

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

	return nullptr;
}


S3DModel* ParseModel(int defID)
{
	const SolidObjectDef* objectDef = nullptr;

	if (defID < 0) {
		objectDef = featureHandler->GetFeatureDefByID(-defID);
	} else {
		objectDef = unitDefHandler->GetUnitDefByID(defID);
	}

	if (objectDef == nullptr)
		return nullptr;

	return (objectDef->LoadModel());
}

bool ParseTexture(const S3DModel* model, LuaMatTexture& texUnit, char texNum)
{
	if (model == nullptr)
		return false;

	const unsigned int texType = model->textureType;

	if (texType == 0)
		return false;

	const CS3OTextureHandler::S3oTex* stex = texturehandlerS3O->GetS3oTex(texType);

	if (stex == nullptr)
		return false;

	if (texNum == '0') {
		texUnit.type = LuaMatTexture::LUATEX_UNITTEXTURE1;
		texUnit.data = reinterpret_cast<const void*>(stex);
		return true;
	}
	if (texNum == '1') {
		texUnit.type = LuaMatTexture::LUATEX_UNITTEXTURE2;
		texUnit.data = reinterpret_cast<const void*>(stex);
		return true;
	}

	return false;
}

bool ParseUnitTexture(LuaMatTexture& texUnit, const std::string& texture)
{
	if (texture.length() < 4)
		return false;

	char* endPtr;
	const char* startPtr = texture.c_str() + 1; // skip the '%'
	const int id = (int)strtol(startPtr, &endPtr, 10);

	if ((endPtr == startPtr) || (*endPtr != ':'))
		return false;

	endPtr++; // skip the ':'

	if ((startPtr - 1) + texture.length() <= endPtr) {
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

	return (ParseTexture(ParseModel(id), texUnit, *endPtr));
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

	if (image.empty())
		return false;

	if (image[0] == LuaTextures::prefix) {
		if (L == nullptr)
			return false;

		// dynamic texture
		const LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
		const LuaTextures::Texture* texInfo = textures.GetInfo(image);

		if (texInfo == nullptr)
			return false;

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

		if (endPtr == startPtr)
			return false;

		const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);

		if (ud == nullptr)
			return false;

		texUnit.type = LuaMatTexture::LUATEX_UNITBUILDPIC;
		texUnit.data = reinterpret_cast<const void*>(ud);
	}
	else if (image[0] == '^') {
		// unit icon
		char* endPtr;
		const char* startPtr = image.c_str() + 1; // skip the '^'
		const int unitDefID = (int)strtol(startPtr, &endPtr, 10);

		if (endPtr == startPtr)
			return false;

		const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);

		if (ud == nullptr)
			return false;

		texUnit.type = LuaMatTexture::LUATEX_UNITRADARICON;
		texUnit.data = reinterpret_cast<const void*>(ud);
	}

	else if (image[0] == '$') {
		switch ((texUnit.type = GetLuaMatTextureType(image))) {
			case LuaMatTexture::LUATEX_NONE: {
				// name not found in table, check for special case override
				if (image.find("$info:") != 0)
					return false;

				const std::string& infoTexName = image.substr(6);
				const CInfoTexture* itex = infoTextureHandler->GetInfoTextureConst(infoTexName);

				if (itex->GetName() == "dummy")
					return false;

				infoTexNames.insert(infoTexName);
				texUnit.type = LuaMatTexture::LUATEX_INFOTEX;
				texUnit.data = &*infoTexNames.find(infoTexName);
			} break;

			case LuaMatTexture::LUATEX_3DOTEXTURE: {
				if (image.size() == 5) {
					// "$units"
					texUnit.data = reinterpret_cast<const void*>(int(1));
				} else {
					// "$units1" or "$units2"
					switch (image[6]) {
						case '1': { texUnit.data = reinterpret_cast<const void*>(int(1)); } break;
						case '2': { texUnit.data = reinterpret_cast<const void*>(int(2)); } break;
					}
				}
			} break;

			case LuaMatTexture::LUATEX_HEIGHTMAP: {
				if (heightMapTexture->GetTextureID() == 0) {
					// optional, return false when not available
					return false;
				}
			} break;

			default: {
			} break;
		}
	}
	else {
		const CNamedTextures::TexInfo* texInfo = CNamedTextures::GetInfo(image, true);

		if (texInfo != nullptr) {
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

		// object model-textures
		case LUATEX_UNITTEXTURE1: {
			texID = (reinterpret_cast<const CS3OTextureHandler::S3oTex*>(data))->tex1;
		} break;
		case LUATEX_UNITTEXTURE2: {
			texID = (reinterpret_cast<const CS3OTextureHandler::S3oTex*>(data))->tex2;
		} break;
		case LUATEX_3DOTEXTURE: {
			if (texturehandler3DO != nullptr) {
				if (*reinterpret_cast<const int*>(&data) == 1) {
					texID = texturehandler3DO->GetAtlasTex1ID();
				} else {
					texID = texturehandler3DO->GetAtlasTex2ID();
				}
			}
		} break;

		// object icon-textures
		case LUATEX_UNITBUILDPIC: if (unitDefHandler != nullptr) {
			texID = unitDefHandler->GetUnitDefImage(reinterpret_cast<const UnitDef*>(data));
		} break;
		case LUATEX_UNITRADARICON: {
			texID = (reinterpret_cast<const UnitDef*>(data))->iconType->GetTextureID();
		} break;


		// cubemap textures
		case LUATEX_MAP_REFLECTION: {
			if (cubeMapHandler != nullptr) {
				texID = cubeMapHandler->GetEnvReflectionTextureID();
			}
		} break;
		case LUATEX_SKY_REFLECTION: {
			if (cubeMapHandler != nullptr) {
				texID = cubeMapHandler->GetSkyReflectionTextureID();
			}
		} break;
		case LUATEX_SPECULAR: {
			if (cubeMapHandler != nullptr) {
				texID = cubeMapHandler->GetSpecularTextureID();
			}
		} break;


		case LUATEX_SHADOWMAP: {
			if (shadowHandler != nullptr) {
				texID = shadowHandler->shadowTexture;
			}
		} break;
		case LUATEX_HEIGHTMAP: {
			if (heightMapTexture != nullptr) {
				texID = heightMapTexture->GetTextureID();
			}
		} break;


		// map shading textures (TODO: diffuse? needs coors and {G,S}etMapSquareTexture rework)
		case LUATEX_SMF_GRASS:
		case LUATEX_SMF_DETAIL:
		case LUATEX_SMF_MINIMAP:
		case LUATEX_SMF_SHADING:
		case LUATEX_SMF_NORMALS:

		case LUATEX_SSMF_NORMALS:
		case LUATEX_SSMF_SPECULAR:
		case LUATEX_SSMF_SDISTRIB:
		case LUATEX_SSMF_SDETAIL:
		case LUATEX_SSMF_SNORMALS:
		case LUATEX_SSMF_SKYREFL:
		case LUATEX_SSMF_EMISSION:
		case LUATEX_SSMF_PARALLAX: {
			if (readMap != nullptr) {
				// convert type=LUATEX_* to MAP_* (FIXME: MAP_SSMF_SPLAT_NORMAL_TEX needs a num)
				texID = readMap->GetTexture(type - LUATEX_SMF_GRASS);
			}
		} break;


		// map info-overlay textures
		case LUATEX_INFOTEX: if (infoTextureHandler != nullptr) {
			const std::string* name = static_cast<const std::string*>(data);
			const CInfoTexture* itex = infoTextureHandler->GetInfoTextureConst(*name);

			texID = itex->GetTexture();
		} break;

		case LUATEX_INFOTEX_ACTIVE: if (infoTextureHandler != nullptr) {
			texID = infoTextureHandler->GetCurrentInfoTexture();
		} break;
		case LUATEX_INFOTEX_LOSMAP: if (infoTextureHandler != nullptr) {
			texID = (infoTextureHandler->GetInfoTextureConst("los"))->GetTexture();
		} break;
		case LUATEX_INFOTEX_MTLMAP: if (infoTextureHandler != nullptr) {
			texID = (infoTextureHandler->GetInfoTextureConst("metal"))->GetTexture();
		} break;
		case LUATEX_INFOTEX_HGTMAP: if (infoTextureHandler != nullptr) {
			texID = (infoTextureHandler->GetInfoTextureConst("height"))->GetTexture();
		} break;
		case LUATEX_INFOTEX_BLKMAP: if (infoTextureHandler != nullptr) {
			texID = (infoTextureHandler->GetInfoTextureConst("path"))->GetTexture();
		} break;


		// g-buffer textures
		case LUATEX_MAP_GBUFFER_NORM: { texID = gdGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_NORMTEX); } break;
		case LUATEX_MAP_GBUFFER_DIFF: { texID = gdGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_DIFFTEX); } break;
		case LUATEX_MAP_GBUFFER_SPEC: { texID = gdGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_SPECTEX); } break;
		case LUATEX_MAP_GBUFFER_EMIT: { texID = gdGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_EMITTEX); } break;
		case LUATEX_MAP_GBUFFER_MISC: { texID = gdGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_MISCTEX); } break;
		case LUATEX_MAP_GBUFFER_ZVAL: { texID = gdGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_ZVALTEX); } break;

		case LUATEX_MODEL_GBUFFER_NORM: { texID = udGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_NORMTEX); } break;
		case LUATEX_MODEL_GBUFFER_DIFF: { texID = udGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_DIFFTEX); } break;
		case LUATEX_MODEL_GBUFFER_SPEC: { texID = udGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_SPECTEX); } break;
		case LUATEX_MODEL_GBUFFER_EMIT: { texID = udGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_EMITTEX); } break;
		case LUATEX_MODEL_GBUFFER_MISC: { texID = udGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_MISCTEX); } break;
		case LUATEX_MODEL_GBUFFER_ZVAL: { texID = udGeomBuff->GetBufferTexture(GL::GeometryBuffer::ATTACHMENT_ZVALTEX); } break;


		// font textures
		case LUATEX_FONT: {
			texID = font->GetTexture();
		} break;
		case LUATEX_FONTSMALL: {
			texID = smallFont->GetTexture();
		} break;

		default: {
			assert(false);
		} break;
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

		case LUATEX_MAP_REFLECTION:
		case LUATEX_SKY_REFLECTION:
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


		case LUATEX_SMF_GRASS:
		case LUATEX_SMF_DETAIL:
		case LUATEX_SMF_MINIMAP:
		case LUATEX_SMF_SHADING:
		case LUATEX_SMF_NORMALS:

		case LUATEX_SSMF_NORMALS:
		case LUATEX_SSMF_SPECULAR:
		case LUATEX_SSMF_SDISTRIB:
		case LUATEX_SSMF_SDETAIL:
		case LUATEX_SSMF_SNORMALS:
		case LUATEX_SSMF_SKYREFL:
		case LUATEX_SSMF_EMISSION:
		case LUATEX_SSMF_PARALLAX:


		case LUATEX_INFOTEX:
		case LUATEX_INFOTEX_ACTIVE:
		case LUATEX_INFOTEX_LOSMAP:
		case LUATEX_INFOTEX_MTLMAP:
		case LUATEX_INFOTEX_HGTMAP:
		case LUATEX_INFOTEX_BLKMAP: {
			texType = GL_TEXTURE_2D;
		} break;

		case LUATEX_MAP_GBUFFER_NORM:
		case LUATEX_MAP_GBUFFER_DIFF:
		case LUATEX_MAP_GBUFFER_SPEC:
		case LUATEX_MAP_GBUFFER_EMIT:
		case LUATEX_MAP_GBUFFER_MISC:
		case LUATEX_MAP_GBUFFER_ZVAL: {
			texType = GL_TEXTURE_2D;
		} break;

		case LUATEX_MODEL_GBUFFER_NORM:
		case LUATEX_MODEL_GBUFFER_DIFF:
		case LUATEX_MODEL_GBUFFER_SPEC:
		case LUATEX_MODEL_GBUFFER_EMIT:
		case LUATEX_MODEL_GBUFFER_MISC:
		case LUATEX_MODEL_GBUFFER_ZVAL: {
			texType = GL_TEXTURE_2D;
		} break;

		case LUATEX_FONT:
		case LUATEX_FONTSMALL: {
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
	#define groundDrawer (readMap->GetGroundDrawer())
	#define gdGeomBuff (groundDrawer->GetGeometryBuffer())
	#define udGeomBuff (unitDrawer->GetGeometryBuffer())
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
		case LUATEX_3DOTEXTURE: {
			if (texturehandler3DO != nullptr) {
				return int2(texturehandler3DO->GetAtlasTexSizeX(), texturehandler3DO->GetAtlasTexSizeY());
			}
		} break;


		case LUATEX_UNITBUILDPIC: {
			if (unitDefHandler != nullptr) {
				const auto ud = reinterpret_cast<const UnitDef*>(data);
				unitDefHandler->GetUnitDefImage(ud); // forced existance
				const auto bp = ud->buildPic;
				return int2(bp->imageSizeX, bp->imageSizeY);
			}
		} break;
		case LUATEX_UNITRADARICON: {
			const auto ud = reinterpret_cast<const UnitDef*>(data);
			const auto it = ud->iconType;
			return int2(it->GetSizeX(), it->GetSizeY());
		} break;


		case LUATEX_MAP_REFLECTION: {
			if (cubeMapHandler != nullptr) {
				return sqint2(cubeMapHandler->GetReflectionTextureSize());
			}
		} break;
		case LUATEX_SKY_REFLECTION: {
			if (cubeMapHandler != nullptr) {
				// note: same size as regular refltex
				return sqint2(cubeMapHandler->GetReflectionTextureSize());
			}
		} break;
		case LUATEX_SPECULAR: {
			if (cubeMapHandler != nullptr) {
				return sqint2(cubeMapHandler->GetSpecularTextureSize());
			}
		} break;


		case LUATEX_SHADOWMAP: {
			if (shadowHandler != nullptr) {
				return sqint2(shadowHandler->shadowMapSize);
			}
		} break;
		case LUATEX_HEIGHTMAP: {
			if (heightMapTexture != nullptr) {
				return int2(heightMapTexture->GetSizeX(), heightMapTexture->GetSizeY());
			}
		} break;


		case LUATEX_SMF_GRASS:
		case LUATEX_SMF_DETAIL:
		case LUATEX_SMF_MINIMAP:
		case LUATEX_SMF_SHADING:
		case LUATEX_SMF_NORMALS:

		case LUATEX_SSMF_NORMALS:
		case LUATEX_SSMF_SPECULAR:
		case LUATEX_SSMF_SDISTRIB:
		case LUATEX_SSMF_SDETAIL:
		case LUATEX_SSMF_SNORMALS:
		case LUATEX_SSMF_SKYREFL:
		case LUATEX_SSMF_EMISSION:
		case LUATEX_SSMF_PARALLAX: {
			if (readMap != nullptr) {
				// convert type=LUATEX_* to MAP_*
				return (readMap->GetTextureSize(type - LUATEX_SMF_GRASS));
			}
		} break;


		case LUATEX_INFOTEX_ACTIVE: {
			if (infoTextureHandler != nullptr) {
				return infoTextureHandler->GetCurrentInfoTextureSize();
			}
		} break;

		case LUATEX_INFOTEX: {
			if (infoTextureHandler != nullptr) {
				const std::string* name = static_cast<const std::string*>(data);
				const CInfoTexture* itex = infoTextureHandler->GetInfoTextureConst(*name);

				return (itex->GetTexSize());
			}
		} break;

		case LUATEX_INFOTEX_LOSMAP: if (infoTextureHandler != nullptr) {
			return ((infoTextureHandler->GetInfoTextureConst("los"))->GetTexSize());
		} break;
		case LUATEX_INFOTEX_MTLMAP: if (infoTextureHandler != nullptr) {
			return ((infoTextureHandler->GetInfoTextureConst("metal"))->GetTexSize());
		} break;
		case LUATEX_INFOTEX_HGTMAP: if (infoTextureHandler != nullptr) {
			return ((infoTextureHandler->GetInfoTextureConst("height"))->GetTexSize());
		} break;
		case LUATEX_INFOTEX_BLKMAP: if (infoTextureHandler != nullptr) {
			return ((infoTextureHandler->GetInfoTextureConst("path"))->GetTexSize());
		} break;


		case LUATEX_MAP_GBUFFER_NORM:
		case LUATEX_MAP_GBUFFER_DIFF:
		case LUATEX_MAP_GBUFFER_SPEC:
		case LUATEX_MAP_GBUFFER_EMIT:
		case LUATEX_MAP_GBUFFER_MISC:
		case LUATEX_MAP_GBUFFER_ZVAL: {
			if (readMap != nullptr) {
				return (gdGeomBuff->GetWantedSize(readMap->GetGroundDrawer()->DrawDeferred()));
			}
		} break;

		case LUATEX_MODEL_GBUFFER_NORM:
		case LUATEX_MODEL_GBUFFER_DIFF:
		case LUATEX_MODEL_GBUFFER_SPEC:
		case LUATEX_MODEL_GBUFFER_EMIT:
		case LUATEX_MODEL_GBUFFER_MISC:
		case LUATEX_MODEL_GBUFFER_ZVAL: {
			if (unitDrawer != nullptr) {
				return (udGeomBuff->GetWantedSize(unitDrawer->DrawDeferred()));
			}
		} break;


		case LUATEX_FONT:
			return int2(font->GetTextureWidth(), font->GetTextureHeight());
		case LUATEX_FONTSMALL:
			return int2(smallFont->GetTextureWidth(), smallFont->GetTextureHeight());


		case LUATEX_NONE:
		default: break;
	}

	#undef groundDrawer
	#undef gdGeomBuff
	#undef udGeomBuff

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

		STRING_CASE(typeName, LUATEX_MAP_REFLECTION);
		STRING_CASE(typeName, LUATEX_SKY_REFLECTION);
		STRING_CASE(typeName, LUATEX_SPECULAR);

		STRING_CASE(typeName, LUATEX_SHADOWMAP);
		STRING_CASE(typeName, LUATEX_HEIGHTMAP);

		STRING_CASE(typeName, LUATEX_SMF_GRASS);
		STRING_CASE(typeName, LUATEX_SMF_DETAIL);
		STRING_CASE(typeName, LUATEX_SMF_MINIMAP);
		STRING_CASE(typeName, LUATEX_SMF_SHADING);
		STRING_CASE(typeName, LUATEX_SMF_NORMALS);

		STRING_CASE(typeName, LUATEX_SSMF_NORMALS);
		STRING_CASE(typeName, LUATEX_SSMF_SPECULAR);
		STRING_CASE(typeName, LUATEX_SSMF_SDISTRIB);
		STRING_CASE(typeName, LUATEX_SSMF_SDETAIL);
		STRING_CASE(typeName, LUATEX_SSMF_SNORMALS);
		STRING_CASE(typeName, LUATEX_SSMF_SKYREFL);
		STRING_CASE(typeName, LUATEX_SSMF_EMISSION);
		STRING_CASE(typeName, LUATEX_SSMF_PARALLAX);


		STRING_CASE(typeName, LUATEX_INFOTEX);
		STRING_CASE(typeName, LUATEX_INFOTEX_ACTIVE);
		STRING_CASE(typeName, LUATEX_INFOTEX_LOSMAP);
		STRING_CASE(typeName, LUATEX_INFOTEX_MTLMAP);
		STRING_CASE(typeName, LUATEX_INFOTEX_HGTMAP);
		STRING_CASE(typeName, LUATEX_INFOTEX_BLKMAP);

		STRING_CASE(typeName, LUATEX_MAP_GBUFFER_NORM);
		STRING_CASE(typeName, LUATEX_MAP_GBUFFER_DIFF);
		STRING_CASE(typeName, LUATEX_MAP_GBUFFER_SPEC);
		STRING_CASE(typeName, LUATEX_MAP_GBUFFER_EMIT);
		STRING_CASE(typeName, LUATEX_MAP_GBUFFER_MISC);
		STRING_CASE(typeName, LUATEX_MAP_GBUFFER_ZVAL);

		STRING_CASE(typeName, LUATEX_MODEL_GBUFFER_NORM);
		STRING_CASE(typeName, LUATEX_MODEL_GBUFFER_DIFF);
		STRING_CASE(typeName, LUATEX_MODEL_GBUFFER_SPEC);
		STRING_CASE(typeName, LUATEX_MODEL_GBUFFER_EMIT);
		STRING_CASE(typeName, LUATEX_MODEL_GBUFFER_MISC);
		STRING_CASE(typeName, LUATEX_MODEL_GBUFFER_ZVAL);


		STRING_CASE(typeName, LUATEX_FONT);
		STRING_CASE(typeName, LUATEX_FONTSMALL);

		#undef STRING_CASE
	}

	LOG("%s%s %i %s", indent.c_str(), typeName, *reinterpret_cast<const GLuint*>(&data), (enable ? "true" : "false"));
}


/******************************************************************************/
/******************************************************************************/
