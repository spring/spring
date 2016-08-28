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
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitDefImage.h"
#include "System/Matrix44f.h"
#include "System/Util.h"
#include "System/Log/ILog.h"
#include "System/Sync/HsiehHash.h"



// for "$info:los", etc
static std::unordered_map<size_t, LuaMatTexture> luaMatTextures;

static const std::unordered_map<std::string, LuaMatTexture::Type> luaMatTexTypeMap = {
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


	{"$info",         LuaMatTexture::LUATEX_INFOTEX_ACTIVE},
	{"$extra",        LuaMatTexture::LUATEX_INFOTEX_ACTIVE},

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

static const std::unordered_map<std::string, LuaMatrixType> luaMatrixTypeMap = {
	{"view",                  LUAMATRICES_VIEW                 },
	{"projection",            LUAMATRICES_PROJECTION           },
	{"viewprojection",        LUAMATRICES_VIEWPROJECTION       },
	{"viewinverse",           LUAMATRICES_VIEWINVERSE          },
	{"projectioninverse",     LUAMATRICES_PROJECTIONINVERSE    },
	{"viewprojectioninverse", LUAMATRICES_VIEWPROJECTIONINVERSE},
	{"billboard",             LUAMATRICES_BILLBOARD            },
	{"shadow",                LUAMATRICES_SHADOW               },

	// backward compability
	{"camera",    LUAMATRICES_VIEW             },
	{"camprj",    LUAMATRICES_PROJECTION       },
	{"caminv",    LUAMATRICES_VIEWINVERSE      },
	{"camprjinv", LUAMATRICES_PROJECTIONINVERSE},
};


/******************************************************************************/
/******************************************************************************/


void LuaOpenGLUtils::ResetState()
{
	// must be cleared, LuaMatTexture's contain pointers
	luaMatTextures.clear();
}



LuaMatTexture::Type LuaOpenGLUtils::GetLuaMatTextureType(const std::string& name)
{
	const auto it = luaMatTexTypeMap.find(name);

	if (it == luaMatTexTypeMap.end())
		return LuaMatTexture::LUATEX_NONE;

	return it->second;
}

LuaMatrixType LuaOpenGLUtils::GetLuaMatrixType(const std::string& name)
{
	const auto it = luaMatrixTypeMap.find(name);

	if (it == luaMatrixTypeMap.end())
		return LUAMATRICES_NONE;

	return it->second;
}

const CMatrix44f* LuaOpenGLUtils::GetNamedMatrix(const std::string& name)
{
	// skipped for performance reasons (this function gets called a lot)
	// StringToLowerInPlace(name);

	switch (GetLuaMatrixType(name)) {
		case LUAMATRICES_SHADOW:
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



/******************************************************************************/
/******************************************************************************/

S3DModel* ParseModel(int defID)
{
	const SolidObjectDef* objectDef = nullptr;

	if (defID < 0) {
		objectDef = featureDefHandler->GetFeatureDefByID(-defID);
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

	const int texType = model->textureType;

	if (texType == 0)
		return false;

	// note: textures are stored contiguously, so this pointer can not be leaked
	const CS3OTextureHandler::S3OTexMat* stex = texturehandlerS3O->GetTexture(texType);

	if (stex == nullptr)
		return false;

	if (texNum == '0') {
		texUnit.type = LuaMatTexture::LUATEX_UNITTEXTURE1;
		texUnit.data = reinterpret_cast<const void*>(texType);
		return true;
	}
	if (texNum == '1') {
		texUnit.type = LuaMatTexture::LUATEX_UNITTEXTURE2;
		texUnit.data = reinterpret_cast<const void*>(texType);
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



static bool ParseNamedSubTexture(LuaMatTexture& texUnit, const std::string& texName)
{
	const size_t texNameHash = HsiehHash(texName.c_str(), texName.size(), 0);
	const auto luaMatTexIt = luaMatTextures.find(texNameHash);

	// see if this texture has been requested before
	if (luaMatTexIt != luaMatTextures.end()) {
		texUnit.type = (luaMatTexIt->second).type;
		texUnit.data = (luaMatTexIt->second).data;
		return true;
	}

	// search for both types of separators (preserves BC)
	//
	// note: "$ssmf*" contains underscores in its prefix
	// that are not separators (unlike the old "$info_*"
	// and "$extra_*"), so do not use find_first_of
	const size_t sepIdx = texName.find_last_of(":_");

	if (sepIdx == std::string::npos)
		return false;


	const size_t prefixHash = HsiehHash(texName.c_str(), sepIdx, 0);

	// TODO: use constexpr hashes plus switch, also add $*_gbuffer_*
	static const size_t prefixHashes[] = {
		HsiehHash("$ssmf_splat_normals", sizeof("$ssmf_splat_normals") - 1, 0),
		HsiehHash(              "$info", sizeof(              "$info") - 1, 0),
		HsiehHash(             "$extra", sizeof(             "$extra") - 1, 0),
	};


	if (prefixHash == prefixHashes[0]) {
		// last character becomes the index, clamped in ReadMap
		// (data remains 0 if suffix is not ":X" with X a digit)
		texUnit.type = LuaMatTexture::LUATEX_SSMF_SNORMALS;
		texUnit.data = reinterpret_cast<const void*>(0);

		if ((sepIdx + 2) == texName.size() && std::isdigit(texName.back())) {
			texUnit.data = reinterpret_cast<const void*>(int(texName.back()) - int('0'));
		}

		luaMatTextures[texNameHash] = texUnit;
		return true;
	}

	if (prefixHash == prefixHashes[1] || prefixHash == prefixHashes[2]) {
		// try to find a matching info-texture based on suffix
		CInfoTexture* itex = infoTextureHandler->GetInfoTexture(texName.substr(sepIdx + 1));

		if (itex->GetName() != "dummy") {
			texUnit.type = LuaMatTexture::LUATEX_INFOTEX_SUFFIX;
			texUnit.data = itex; // fast, and barely preserves RTTI

			luaMatTextures[texNameHash] = texUnit;
			return true;
		}
	}

	return false;
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
				// name not found in table, check for special case overrides
				if (!ParseNamedSubTexture(texUnit, image)) {
					return false;
				}
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
						default: { return false; } break;
					}
				}
			} break;

			case LuaMatTexture::LUATEX_HEIGHTMAP: {
				if (heightMapTexture->GetTextureID() == 0) {
					// optional, return false when not available
					return false;
				}
			} break;

			case LuaMatTexture::LUATEX_SSMF_SNORMALS: {
				// suffix case (":X") is handled by ParseNamedSubTexture
				texUnit.data = reinterpret_cast<const void*>(int(0));
			} break;

			default: {
			} break;
		}
	}

	else {
		bool persist = CLuaHandle::GetHandle(L)->GetName() == "LuaMenu";
		const CNamedTextures::TexInfo* texInfo = CNamedTextures::GetInfo(image, true, persist);

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
			texID = texturehandlerS3O->GetTexture(*reinterpret_cast<const int*>(&data))->tex1;
		} break;
		case LUATEX_UNITTEXTURE2: {
			texID = texturehandlerS3O->GetTexture(*reinterpret_cast<const int*>(&data))->tex2;
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
				texID = shadowHandler->GetShadowTextureID();
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
		case LUATEX_SSMF_SKYREFL:
		case LUATEX_SSMF_EMISSION:
		case LUATEX_SSMF_PARALLAX: {
			if (readMap != nullptr) {
				// convert type=LUATEX_* to MAP_*
				texID = readMap->GetTexture(type - LUATEX_SMF_GRASS);
			}
		} break;

		case LUATEX_SSMF_SNORMALS: {
			if (readMap != nullptr) {
				texID = readMap->GetTexture((LUATEX_SSMF_SNORMALS - LUATEX_SMF_GRASS), *reinterpret_cast<const int*>(&data));
			}
		} break;


		// map info-overlay textures
		case LUATEX_INFOTEX_SUFFIX: if (infoTextureHandler != nullptr) {
			const CInfoTexture* cInfoTex = static_cast<const CInfoTexture*>(data);
			      CInfoTexture* mInfoTex = const_cast<CInfoTexture*>(cInfoTex);

			texID = mInfoTex->GetTexture();
		} break;

		case LUATEX_INFOTEX_ACTIVE: if (infoTextureHandler != nullptr) {
			texID = infoTextureHandler->GetCurrentInfoTexture();
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


		case LUATEX_INFOTEX_SUFFIX:
		case LUATEX_INFOTEX_ACTIVE: {
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

		// do not enable cubemap samplers here (not
		// needed for shaders, not wanted otherwise)
		if (enable) {
			switch (texType) {
				case GL_TEXTURE_2D:           {   glEnable(texType);   } break;
				case GL_TEXTURE_CUBE_MAP_ARB: { /*glEnable(texType);*/ } break;
				default:                      {                        } break;
			}
		}
	}

	else if (!enable) {
		switch (texType) {
			case GL_TEXTURE_2D:           {   glDisable(texType);   } break;
			case GL_TEXTURE_CUBE_MAP_ARB: { /*glDisable(texType);*/ } break;
			default:                      {                         } break;
		}
	}

	if (enableTexParams && type == LUATEX_SHADOWMAP)
		shadowHandler->SetupShadowTexSamplerRaw();

}


void LuaMatTexture::Unbind() const
{
	if (type == LUATEX_NONE)
		return;

	if (enableTexParams && type == LUATEX_SHADOWMAP)
		shadowHandler->ResetShadowTexSamplerRaw();

	if (!enable)
		return;

	switch (GetTextureTarget()) {
		case GL_TEXTURE_2D:           {   glDisable(GL_TEXTURE_2D);             } break;
		case GL_TEXTURE_CUBE_MAP_ARB: { /*glDisable(GL_TEXTURE_CUBE_MAP_ARB);*/ } break;
		default:                      {                                         } break;
	}
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
			const auto stex = texturehandlerS3O->GetTexture(*reinterpret_cast<const int*>(&data));
			return int2(stex->tex1SizeX, stex->tex1SizeY);
		} break;
		case LUATEX_UNITTEXTURE2: {
			const auto stex = texturehandlerS3O->GetTexture(*reinterpret_cast<const int*>(&data));
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
		case LUATEX_SSMF_SKYREFL:
		case LUATEX_SSMF_EMISSION:
		case LUATEX_SSMF_PARALLAX: {
			if (readMap != nullptr) {
				// convert type=LUATEX_* to MAP_*
				return (readMap->GetTextureSize(type - LUATEX_SMF_GRASS));
			}
		} break;

		case LUATEX_SSMF_SNORMALS: {
			if (readMap != nullptr) {
				return (readMap->GetTextureSize((LUATEX_SSMF_SNORMALS - LUATEX_SMF_GRASS), *reinterpret_cast<const int*>(&data)));
			}
		} break;


		case LUATEX_INFOTEX_SUFFIX: {
			if (infoTextureHandler != nullptr) {
				return ((static_cast<const CInfoTexture*>(data))->GetTexSize());
			}
		} break;

		case LUATEX_INFOTEX_ACTIVE: {
			if (infoTextureHandler != nullptr) {
				return infoTextureHandler->GetCurrentInfoTextureSize();
			}
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



void LuaMatTexture::Finalize()
{
	// enable &= (type != LUATEX_NONE);
	enableTexParams = true;
}


int LuaMatTexture::Compare(const LuaMatTexture& a, const LuaMatTexture& b)
{
	if (a.type != b.type)
		return (a.type < b.type) ? -1 : +1;

	if (a.data != b.data)
		return (a.data < b.data) ? -1 : +1;

	if (a.enable != b.enable)
		return a.enable ? -1 : +1;

	if (a.enableTexParams != b.enableTexParams)
		return a.enableTexParams ? -1 : +1;

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


		STRING_CASE(typeName, LUATEX_INFOTEX_SUFFIX);
		STRING_CASE(typeName, LUATEX_INFOTEX_ACTIVE);

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
