/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cctype>

#include "LuaOpenGLUtils.h"

#include "LuaHandle.h"
#include "LuaTextures.h"
#include "Game/Camera.h"
#include "Game/UI/MiniMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/HeightMapTexture.h"
#include "Map/ReadMap.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/IconHandler.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDefImage.h"
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
#include "System/Matrix44f.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"



// for "$info:los", etc
static spring::unsynced_map<size_t, LuaMatTexture> luaMatTextures;

/******************************************************************************/
/******************************************************************************/


void LuaOpenGLUtils::ResetState()
{
	// must be cleared, LuaMatTexture's contain pointers
	luaMatTextures.clear();
}



LuaMatTexture::Type LuaOpenGLUtils::GetLuaMatTextureType(const std::string& name)
{
	switch (hashString(name.c_str())) {
		// atlases
		case hashString("$units" ): { return LuaMatTexture::LUATEX_3DOTEXTURE; } break;
		case hashString("$units1"): { return LuaMatTexture::LUATEX_3DOTEXTURE; } break;
		case hashString("$units2"): { return LuaMatTexture::LUATEX_3DOTEXTURE; } break;

		// cubemaps
		case hashString(    "$reflection"): { return LuaMatTexture::LUATEX_MAP_REFLECTION; } break;
		case hashString("$map_reflection"): { return LuaMatTexture::LUATEX_MAP_REFLECTION; } break;
		case hashString("$sky_reflection"): { return LuaMatTexture::LUATEX_SKY_REFLECTION; } break;

		// specials
		case hashString("$shadow"   ): { return LuaMatTexture::LUATEX_SHADOWMAP; } break;
		case hashString("$heightmap"): { return LuaMatTexture::LUATEX_HEIGHTMAP; } break;

		// SMF-maps
		case hashString("$grass"  ): { return LuaMatTexture::LUATEX_SMF_GRASS; } break;
		case hashString("$detail" ): { return LuaMatTexture::LUATEX_SMF_DETAIL; } break;
		case hashString("$minimap"): { return LuaMatTexture::LUATEX_SMF_MINIMAP; } break;
		case hashString("$shading"): { return LuaMatTexture::LUATEX_SMF_SHADING; } break;
		case hashString("$normals"): { return LuaMatTexture::LUATEX_SMF_NORMALS; } break;
		// SSMF-maps
		case hashString("$ssmf_normals"      ): { return LuaMatTexture::LUATEX_SSMF_NORMALS ; } break;
		case hashString("$ssmf_specular"     ): { return LuaMatTexture::LUATEX_SSMF_SPECULAR; } break;
		case hashString("$ssmf_splat_distr"  ): { return LuaMatTexture::LUATEX_SSMF_SDISTRIB; } break;
		case hashString("$ssmf_splat_detail" ): { return LuaMatTexture::LUATEX_SSMF_SDETAIL ; } break;
		case hashString("$ssmf_splat_normals"): { return LuaMatTexture::LUATEX_SSMF_SNORMALS; } break;
		case hashString("$ssmf_sky_refl"     ): { return LuaMatTexture::LUATEX_SSMF_SKYREFL ; } break;
		case hashString("$ssmf_emission"     ): { return LuaMatTexture::LUATEX_SSMF_EMISSION; } break;
		case hashString("$ssmf_parallax"     ): { return LuaMatTexture::LUATEX_SSMF_PARALLAX; } break;


		case hashString("$info" ): { return LuaMatTexture::LUATEX_INFOTEX_ACTIVE; } break;
		case hashString("$extra"): { return LuaMatTexture::LUATEX_INFOTEX_ACTIVE; } break;

		case hashString("$map_gb_nt"): { return LuaMatTexture::LUATEX_MAP_GBUFFER_NORM; } break;
		case hashString("$map_gb_dt"): { return LuaMatTexture::LUATEX_MAP_GBUFFER_DIFF; } break;
		case hashString("$map_gb_st"): { return LuaMatTexture::LUATEX_MAP_GBUFFER_SPEC; } break;
		case hashString("$map_gb_et"): { return LuaMatTexture::LUATEX_MAP_GBUFFER_EMIT; } break;
		case hashString("$map_gb_mt"): { return LuaMatTexture::LUATEX_MAP_GBUFFER_MISC; } break;
		case hashString("$map_gb_zt"): { return LuaMatTexture::LUATEX_MAP_GBUFFER_ZVAL; } break;

		case hashString("$map_gbuffer_normtex"): { return LuaMatTexture::LUATEX_MAP_GBUFFER_NORM; } break;
		case hashString("$map_gbuffer_difftex"): { return LuaMatTexture::LUATEX_MAP_GBUFFER_DIFF; } break;
		case hashString("$map_gbuffer_spectex"): { return LuaMatTexture::LUATEX_MAP_GBUFFER_SPEC; } break;
		case hashString("$map_gbuffer_emittex"): { return LuaMatTexture::LUATEX_MAP_GBUFFER_EMIT; } break;
		case hashString("$map_gbuffer_misctex"): { return LuaMatTexture::LUATEX_MAP_GBUFFER_MISC; } break;
		case hashString("$map_gbuffer_zvaltex"): { return LuaMatTexture::LUATEX_MAP_GBUFFER_ZVAL; } break;

		case hashString("$mdl_gb_nt"): { return LuaMatTexture::LUATEX_MODEL_GBUFFER_NORM; } break;
		case hashString("$mdl_gb_dt"): { return LuaMatTexture::LUATEX_MODEL_GBUFFER_DIFF; } break;
		case hashString("$mdl_gb_st"): { return LuaMatTexture::LUATEX_MODEL_GBUFFER_SPEC; } break;
		case hashString("$mdl_gb_et"): { return LuaMatTexture::LUATEX_MODEL_GBUFFER_EMIT; } break;
		case hashString("$mdl_gb_mt"): { return LuaMatTexture::LUATEX_MODEL_GBUFFER_MISC; } break;
		case hashString("$mdl_gb_zt"): { return LuaMatTexture::LUATEX_MODEL_GBUFFER_ZVAL; } break;

		case hashString("$model_gbuffer_normtex"): { return LuaMatTexture::LUATEX_MODEL_GBUFFER_NORM; } break;
		case hashString("$model_gbuffer_difftex"): { return LuaMatTexture::LUATEX_MODEL_GBUFFER_DIFF; } break;
		case hashString("$model_gbuffer_spectex"): { return LuaMatTexture::LUATEX_MODEL_GBUFFER_SPEC; } break;
		case hashString("$model_gbuffer_emittex"): { return LuaMatTexture::LUATEX_MODEL_GBUFFER_EMIT; } break;
		case hashString("$model_gbuffer_misctex"): { return LuaMatTexture::LUATEX_MODEL_GBUFFER_MISC; } break;
		case hashString("$model_gbuffer_zvaltex"): { return LuaMatTexture::LUATEX_MODEL_GBUFFER_ZVAL; } break;

		case hashString("$font"     ): { return LuaMatTexture::LUATEX_FONT     ; } break;
		case hashString("$smallfont"): { return LuaMatTexture::LUATEX_FONTSMALL; } break;
		case hashString("$fontsmall"): { return LuaMatTexture::LUATEX_FONTSMALL; } break;

		default: {} break;
	}

	return LuaMatTexture::LUATEX_NONE;
}

LuaMatrixType LuaOpenGLUtils::GetLuaMatrixType(const char* name)
{
	switch (hashString(name)) {
		case hashString("view"                 ): { return LUAMATRICES_VIEW                 ; } break;
		case hashString(    "projection"       ): { return LUAMATRICES_PROJECTION           ; } break;
		case hashString("viewprojection"       ): { return LUAMATRICES_VIEWPROJECTION       ; } break;
		case hashString("viewinverse"          ): { return LUAMATRICES_VIEWINVERSE          ; } break;
		case hashString(    "projectioninverse"): { return LUAMATRICES_PROJECTIONINVERSE    ; } break;
		case hashString("viewprojectioninverse"): { return LUAMATRICES_VIEWPROJECTIONINVERSE; } break;
		case hashString(            "billboard"): { return LUAMATRICES_BILLBOARD            ; } break;
		case hashString(           "shadow"    ): // fall-through
		case hashString(           "shadowview"): { return LUAMATRICES_SHADOW_VIEW          ; } break;
		case hashString(           "shadowproj"): { return LUAMATRICES_SHADOW_PROJ          ; } break;

		case hashString(  "mmview"   ): { return LUAMATRICES_MINIMAP_DRAWVIEW    ; } break;
		case hashString(  "mmproj"   ): { return LUAMATRICES_MINIMAP_DRAWPROJ    ; } break;
		case hashString("dimmview"   ): { return LUAMATRICES_MINIMAP_DIMMVIEW    ; } break;
		case hashString("dimmproj"   ): { return LUAMATRICES_MINIMAP_DIMMPROJ    ; } break;
		case hashString("dimmviewlua"): { return LUAMATRICES_MINIMAP_DIMMVIEW_LUA; } break;
		case hashString("dimmprojlua"): { return LUAMATRICES_MINIMAP_DIMMPROJ_LUA; } break;

		// backward compatibility
		case hashString("camera"   ): { return LUAMATRICES_VIEW             ; } break;
		case hashString("camprj"   ): { return LUAMATRICES_PROJECTION       ; } break;
		case hashString("caminv"   ): { return LUAMATRICES_VIEWINVERSE      ; } break;
		case hashString("camprjinv"): { return LUAMATRICES_PROJECTIONINVERSE; } break;

		default: {} break;
	}

	return LUAMATRICES_NONE;
}

const CMatrix44f* LuaOpenGLUtils::GetNamedMatrix(const char* name)
{
	const LuaMatrixType matType = GetLuaMatrixType(name);

	switch (matType) {
		case LUAMATRICES_SHADOW_VIEW:
			return &shadowHandler.GetShadowViewMatrix();
		case LUAMATRICES_SHADOW_PROJ:
			return &shadowHandler.GetShadowProjMatrix();

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

		case LUAMATRICES_MINIMAP_DRAWVIEW    :
		case LUAMATRICES_MINIMAP_DIMMVIEW    :
		case LUAMATRICES_MINIMAP_DIMMVIEW_LUA: {
			if (minimap != nullptr)
				return &minimap->GetViewMat(matType - LUAMATRICES_MINIMAP_DRAWVIEW);
		} break;
		case LUAMATRICES_MINIMAP_DRAWPROJ    :
		case LUAMATRICES_MINIMAP_DIMMPROJ    :
		case LUAMATRICES_MINIMAP_DIMMPROJ_LUA: {
			if (minimap != nullptr)
				return &minimap->GetProjMat(matType - LUAMATRICES_MINIMAP_DRAWPROJ);
		} break;

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
	const CS3OTextureHandler::S3OTexMat* stex = textureHandlerS3O.GetTexture(texType);

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
	const size_t texNameHash = hashString(texName.c_str());
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


	// TODO: add $*_gbuffer_*
	switch (hashString(texName.c_str(), sepIdx)) {
		case hashString("$ssmf_splat_normals"): {
			// last character becomes the index, clamped in ReadMap
			// (data remains 0 if suffix is not ":X" with X a digit)
			texUnit.type = LuaMatTexture::LUATEX_SSMF_SNORMALS;
			texUnit.data = reinterpret_cast<const void*>(0);

			if ((sepIdx + 2) == texName.size() && std::isdigit(texName.back()))
				texUnit.data = reinterpret_cast<const void*>(int(texName.back()) - int('0'));

			luaMatTextures[texNameHash] = texUnit;
			return true;
		} break;

		case hashString("$info"):
		case hashString("$extra"): {
			// try to find a matching (non-dummy) info-texture based on suffix
			CInfoTexture* itex = infoTextureHandler->GetInfoTexture(texName.substr(sepIdx + 1));

			if (itex != nullptr && dynamic_cast<const CDummyInfoTexture*>(itex) == nullptr) {
				texUnit.type = LuaMatTexture::LUATEX_INFOTEX_SUFFIX;
				texUnit.data = itex; // fast, and barely preserves RTTI

				luaMatTextures[texNameHash] = texUnit;
				return true;
			}
		} break;

		default: {
		} break;
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
	// $reflection  --  reflection cube map
	// $heightmap   --  ground heightmap
	// ...          --  named textures
	//

	texUnit.type   = LuaMatTexture::LUATEX_NONE;
	texUnit.enable = false;
	texUnit.data   = nullptr;
	texUnit.state  = L; // can be NULL, but only for InfoTex shaders

	if (image.empty())
		return false;

	switch (image[0]) {
		case LuaTextures::prefix: {
			if (L == nullptr)
				return false;

			// dynamic texture
			const LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
			const LuaTextures::Texture* texInfo = textures.GetInfo(image);

			if (texInfo == nullptr)
				return false;

			texUnit.type = LuaMatTexture::LUATEX_LUATEXTURE;
			texUnit.data = reinterpret_cast<const void*>(textures.GetIdx(image));
		} break;

		case '%': {
			return ParseUnitTexture(texUnit, image);
		} break;

		case '#': {
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
		} break;

		case '^': {
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
		} break;

		case '$': {
			switch ((texUnit.type = GetLuaMatTextureType(image))) {
				case LuaMatTexture::LUATEX_NONE: {
					// name not found in table, check for special case overrides
					if (!ParseNamedSubTexture(texUnit, image))
						return false;
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
		} break;

		default: {
			const CLuaHandle* luaHandle = CLuaHandle::GetHandle(L);
			const CNamedTextures::TexInfo* texInfo = CNamedTextures::GetInfo(image, true, luaHandle->PersistOnReload());

			if (texInfo != nullptr) {
				texUnit.type = LuaMatTexture::LUATEX_NAMED;
				texUnit.data = reinterpret_cast<const void*>(CNamedTextures::GetInfoIndex(image));
			} else {
				LOG_L(L_WARNING, "Lua: Couldn't load texture named \"%s\"!", image.c_str());
				return false;
			}
		} break;
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
			texID = CNamedTextures::GetInfo(*reinterpret_cast<const size_t*>(&data))->id;
		} break;

		case LUATEX_LUATEXTURE: {
			assert(state != nullptr);

			const LuaTextures& luaTextures = CLuaHandle::GetActiveTextures(reinterpret_cast<lua_State*>(state));
			const LuaTextures::Texture* luaTexture = luaTextures.GetInfo(*reinterpret_cast<const size_t*>(&data));

			texID = luaTexture->id;
		} break;

		// object model-textures
		case LUATEX_UNITTEXTURE1: {
			texID = textureHandlerS3O.GetTexture(*reinterpret_cast<const int*>(&data))->tex1;
		} break;
		case LUATEX_UNITTEXTURE2: {
			texID = textureHandlerS3O.GetTexture(*reinterpret_cast<const int*>(&data))->tex2;
		} break;
		case LUATEX_3DOTEXTURE: {
			if (*reinterpret_cast<const int*>(&data) == 1) {
				texID = textureHandler3DO.GetAtlasTex1ID();
			} else {
				texID = textureHandler3DO.GetAtlasTex2ID();
			}
		} break;

		// object icon-textures
		case LUATEX_UNITBUILDPIC: {
			if (unitDefHandler != nullptr)
				texID = unitDrawer->GetUnitDefImage(reinterpret_cast<const UnitDef*>(data));
		} break;
		case LUATEX_UNITRADARICON: {
			texID = (reinterpret_cast<const UnitDef*>(data))->iconType->GetTextureID();
		} break;


		// cubemap textures
		case LUATEX_MAP_REFLECTION: {
			texID = cubeMapHandler.GetEnvReflectionTextureID();
		} break;
		case LUATEX_SKY_REFLECTION: {
			texID = cubeMapHandler.GetSkyReflectionTextureID();
		} break;


		case LUATEX_SHADOWMAP: {
			texID = shadowHandler.GetShadowTextureID();
		} break;
		case LUATEX_HEIGHTMAP: {
			if (heightMapTexture != nullptr)
				texID = heightMapTexture->GetTextureID();
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
			// convert type=LUATEX_* to MAP_*
			if (readMap != nullptr)
				texID = readMap->GetTexture(type - LUATEX_SMF_GRASS);
		} break;

		case LUATEX_SSMF_SNORMALS: {
			if (readMap != nullptr)
				texID = readMap->GetTexture((LUATEX_SSMF_SNORMALS - LUATEX_SMF_GRASS), *reinterpret_cast<const int*>(&data));
		} break;


		// map info-overlay textures
		case LUATEX_INFOTEX_SUFFIX: {
			if (infoTextureHandler != nullptr) {
				const CInfoTexture* cInfoTex = static_cast<const CInfoTexture*>(data);
				      CInfoTexture* mInfoTex = const_cast<CInfoTexture*>(cInfoTex);

				texID = mInfoTex->GetTexture();
			}
		} break;

		case LUATEX_INFOTEX_ACTIVE: {
			if (infoTextureHandler != nullptr)
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

	#define groundDrawer (readMap->GetGroundDrawer())
	#define gdGeomBuff (groundDrawer->GetGeometryBuffer())
	#define udGeomBuff (unitDrawer->GetGeometryBuffer())

	switch (type) {
		case LUATEX_NAMED: {
			texType = GL_TEXTURE_2D; //FIXME allow lua to load cubemaps!
		} break;
		case LUATEX_LUATEXTURE: {
			assert(state != nullptr);

			const LuaTextures& luaTextures = CLuaHandle::GetActiveTextures(reinterpret_cast<lua_State*>(state));
			const LuaTextures::Texture* luaTexture = luaTextures.GetInfo(*reinterpret_cast<const size_t*>(&data));

			texType = luaTexture->target;
		} break;

		case LUATEX_MAP_REFLECTION:
		case LUATEX_SKY_REFLECTION: {
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
			texType = gdGeomBuff->GetTextureTarget();
		} break;

		case LUATEX_MODEL_GBUFFER_NORM:
		case LUATEX_MODEL_GBUFFER_DIFF:
		case LUATEX_MODEL_GBUFFER_SPEC:
		case LUATEX_MODEL_GBUFFER_EMIT:
		case LUATEX_MODEL_GBUFFER_MISC:
		case LUATEX_MODEL_GBUFFER_ZVAL: {
			texType = udGeomBuff->GetTextureTarget();
		} break;

		case LUATEX_FONT:
		case LUATEX_FONTSMALL: {
			texType = GL_TEXTURE_2D;
		} break;

		default:
			assert(false);
	}

	#undef udGeomBuff
	#undef gdGeomBuff
	#undef groundDrawer

	return texType;
}


void LuaMatTexture::Bind() const
{
	const GLuint texID = GetTextureID();
	const GLuint texType = GetTextureTarget();

	if (texID != 0)
		glBindTexture(texType, texID);

	if (type == LUATEX_SHADOWMAP)
		shadowHandler.SetupShadowTexSamplerRaw();
}


void LuaMatTexture::Unbind() const
{
	if (type == LUATEX_SHADOWMAP)
		shadowHandler.ResetShadowTexSamplerRaw();
}


int2 LuaMatTexture::GetSize() const
{
	#define groundDrawer (readMap->GetGroundDrawer())
	#define gdGeomBuff (groundDrawer->GetGeometryBuffer())
	#define udGeomBuff (unitDrawer->GetGeometryBuffer())
	#define sqint2(x) int2(x, x)

	switch (type) {
		case LUATEX_NAMED: {
			const CNamedTextures::TexInfo* namedTexInfo = CNamedTextures::GetInfo(*reinterpret_cast<const size_t*>(&data));

			return {namedTexInfo->xsize, namedTexInfo->ysize};
		} break;

		case LUATEX_LUATEXTURE: {
			assert(state != nullptr);

			const LuaTextures& luaTextures = CLuaHandle::GetActiveTextures(reinterpret_cast<lua_State*>(state));
			const LuaTextures::Texture* luaTexture = luaTextures.GetInfo(*reinterpret_cast<const size_t*>(&data));

			return {luaTexture->xsize, luaTexture->ysize};
		} break;


		case LUATEX_UNITTEXTURE1: {
			const CS3OTextureHandler::S3OTexMat* texMat = textureHandlerS3O.GetTexture(*reinterpret_cast<const int*>(&data));
			return {static_cast<int>(texMat->tex1SizeX), static_cast<int>(texMat->tex1SizeY)};
		} break;
		case LUATEX_UNITTEXTURE2: {
			const CS3OTextureHandler::S3OTexMat* texMat = textureHandlerS3O.GetTexture(*reinterpret_cast<const int*>(&data));
			return {static_cast<int>(texMat->tex2SizeX), static_cast<int>(texMat->tex2SizeY)};
		} break;
		case LUATEX_3DOTEXTURE: {
			return {static_cast<int>(textureHandler3DO.GetAtlasTexSizeX()), static_cast<int>(textureHandler3DO.GetAtlasTexSizeY())};
		} break;


		case LUATEX_UNITBUILDPIC: {
			if (unitDefHandler != nullptr) {
				const UnitDef* ud = reinterpret_cast<const UnitDef*>(data);
				unitDrawer->GetUnitDefImage(ud); // forced existance
				const UnitDefImage* bp = ud->buildPic;
				return {bp->imageSizeX, bp->imageSizeY};
			}
		} break;
		case LUATEX_UNITRADARICON: {
			const UnitDef* ud = reinterpret_cast<const UnitDef*>(data);
			const icon::CIcon& it = ud->iconType;
			return {it->GetSizeX(), it->GetSizeY()};
		} break;


		case LUATEX_MAP_REFLECTION: {
			return sqint2(cubeMapHandler.GetReflectionTextureSize());
		} break;
		case LUATEX_SKY_REFLECTION: {
			// note: same size as regular refltex
			return sqint2(cubeMapHandler.GetReflectionTextureSize());
		} break;


		case LUATEX_SHADOWMAP: {
			return sqint2(shadowHandler.shadowMapSize);
		} break;
		case LUATEX_HEIGHTMAP: {
			if (heightMapTexture != nullptr)
				return {heightMapTexture->GetSizeX(), heightMapTexture->GetSizeY()};
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
			// convert type=LUATEX_* to MAP_*
			if (readMap != nullptr)
				return (readMap->GetTextureSize(type - LUATEX_SMF_GRASS));
		} break;

		case LUATEX_SSMF_SNORMALS: {
			if (readMap != nullptr)
				return (readMap->GetTextureSize((LUATEX_SSMF_SNORMALS - LUATEX_SMF_GRASS), *reinterpret_cast<const int*>(&data)));
		} break;


		case LUATEX_INFOTEX_SUFFIX: {
			if (infoTextureHandler != nullptr)
				return ((static_cast<const CInfoTexture*>(data))->GetTexSize());
		} break;

		case LUATEX_INFOTEX_ACTIVE: {
			if (infoTextureHandler != nullptr)
				return infoTextureHandler->GetCurrentInfoTextureSize();
		} break;


		case LUATEX_MAP_GBUFFER_NORM:
		case LUATEX_MAP_GBUFFER_DIFF:
		case LUATEX_MAP_GBUFFER_SPEC:
		case LUATEX_MAP_GBUFFER_EMIT:
		case LUATEX_MAP_GBUFFER_MISC:
		case LUATEX_MAP_GBUFFER_ZVAL: {
			if (readMap != nullptr)
				return (gdGeomBuff->GetWantedSize(readMap->GetGroundDrawer()->DrawDeferred()));
		} break;

		case LUATEX_MODEL_GBUFFER_NORM:
		case LUATEX_MODEL_GBUFFER_DIFF:
		case LUATEX_MODEL_GBUFFER_SPEC:
		case LUATEX_MODEL_GBUFFER_EMIT:
		case LUATEX_MODEL_GBUFFER_MISC:
		case LUATEX_MODEL_GBUFFER_ZVAL: {
			if (unitDrawer != nullptr)
				return (udGeomBuff->GetWantedSize(unitDrawer->DrawDeferred()));
		} break;


		case LUATEX_FONT:
			return {font->GetTextureWidth(), font->GetTextureHeight()};
		case LUATEX_FONTSMALL:
			return {smallFont->GetTextureWidth(), smallFont->GetTextureHeight()};


		case LUATEX_NONE:
		default: break;
	}

	#undef groundDrawer
	#undef gdGeomBuff
	#undef udGeomBuff

	return int2(0, 0);
}



int LuaMatTexture::Compare(const LuaMatTexture& a, const LuaMatTexture& b)
{
	if (a.type != b.type)
		return (a.type < b.type) ? -1 : +1;

	if (a.data != b.data)
		return (a.data < b.data) ? -1 : +1;

	if (a.enable != b.enable)
		return a.enable ? -1 : +1;

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
