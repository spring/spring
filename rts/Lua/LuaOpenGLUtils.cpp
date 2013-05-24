/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <map>
#include <boost/assign/list_of.hpp>

#include "LuaOpenGLUtils.h"

#include "LuaHandle.h"
#include "LuaTextures.h"
#include "Game/Camera.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/HeightMapTexture.h"
#include "Map/ReadMap.h"
#include "Rendering/glFont.h"
#include "Rendering/IconHandler.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "System/Matrix44f.h"
#include "System/Util.h"
#include "System/Log/ILog.h"


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
			if (!shadowHandler) { break; }
			return &shadowHandler->shadowMatrix;
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


const GLuint LuaOpenGLUtils::ParseUnitTexture(const std::string& texture)
{
	if (texture.length()<4) {
		return 0;
	}

	char* endPtr;
	const char* startPtr = texture.c_str() + 1; // skip the '%'
	const int id = (int)strtol(startPtr, &endPtr, 10);
	if ((endPtr == startPtr) || (*endPtr != ':')) {
		return 0;
	}

	endPtr++; // skip the ':'
	if ( (startPtr-1)+texture.length() <= endPtr ) {
		return 0; // ':' is end of string, but we expect '%num:0'
	}

	if (id == 0) {
		if (*endPtr == '0') {
			return texturehandler3DO->GetAtlasTex1ID();
		}
		else if (*endPtr == '1') {
			return texturehandler3DO->GetAtlasTex2ID();
		}
		return 0;
	}

	S3DModel* model;

	if (id < 0) {
		const FeatureDef* fd = featureHandler->GetFeatureDefByID(-id);
		if (fd == NULL) {
			return 0;
		}
		model = fd->LoadModel();
	} else {
		const UnitDef* ud = unitDefHandler->GetUnitDefByID(id);
		if (ud == NULL) {
			return 0;
		}
		model = ud->LoadModel();
	}

	if (model == NULL) {
		return 0;
	}

	const unsigned int texType = model->textureType;
	if (texType == 0) {
		return 0;
	}

	const CS3OTextureHandler::S3oTex* stex = texturehandlerS3O->GetS3oTex(texType);
	if (stex == NULL) {
		return 0;
	}

	if (*endPtr == '0') {
		return stex->tex1;
	}
	else if (*endPtr == '1') {
		return stex->tex2;
	}

	return 0;
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

	texUnit.type     = LuaMatTexture::LUATEX_NONE;
	texUnit.enable   = false;
	texUnit.openglID = 0;

	if (image.empty()) {
		return false;
	}

	if (image[0] == LuaTextures::prefix) {
		// dynamic texture
		LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
		const LuaTextures::Texture* texInfo = textures.GetInfo(image);
		if (texInfo == NULL) {
			return false;
		}
		texUnit.type = LuaMatTexture::LUATEX_GL;
		texUnit.openglID = texInfo->id;
	}
	else if (image[0] == '%') {
		texUnit.type = LuaMatTexture::LUATEX_GL;
		texUnit.openglID = ParseUnitTexture(image);
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
		texUnit.type = LuaMatTexture::LUATEX_GL;
		texUnit.openglID = unitDefHandler->GetUnitDefImage(ud);
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
		texUnit.type = LuaMatTexture::LUATEX_GL;
		texUnit.openglID = ud->iconType->GetTextureID();
	}
	else if (image[0] == '$') {
		if (image == "$units" || image == "$units1") {
			texUnit.type = LuaMatTexture::LUATEX_GL;
			texUnit.openglID = texturehandler3DO->GetAtlasTex1ID();
		}
		else if (image == "$units2") {
			texUnit.type = LuaMatTexture::LUATEX_GL;
			texUnit.openglID = texturehandler3DO->GetAtlasTex2ID();
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
		else if (image == "$extra") {
			texUnit.type = LuaMatTexture::LUATEX_EXTRA;
		}
		else if (image == "$heightmap") {
			texUnit.type = LuaMatTexture::LUATEX_HEIGHTMAP;
			const GLuint texID = heightMapTexture->GetTextureID();
			if (texID == 0) {
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
		else if (image == "$info") {
			texUnit.type = LuaMatTexture::LUATEX_INFOTEX;
		}
		else if (image == "$font") {
			texUnit.type = LuaMatTexture::LUATEX_FONT;
		}
		else if (image == "$smallfont" || image == "$fontsmall") {
			texUnit.type = LuaMatTexture::LUATEX_FONTSMALL;
		}
	}
	else {
		const CNamedTextures::TexInfo* texInfo = CNamedTextures::GetInfo(image, true);
		if (texInfo != NULL) {
			//FIXME save GL_TEXTURE_2D etc. somewhere (so we can even bind cubemaps)
			texUnit.type = LuaMatTexture::LUATEX_GL;
			texUnit.openglID = texInfo->id;
		} else {
			LOG_L(L_WARNING, "Lua: Couldn't load texture named \"%s\"!", image.c_str());
			return false;
		}
	}

	return true;
}


void LuaMatTexture::Bind() const
{
	GLuint texID = 0;
	GLuint texType = GL_TEXTURE_2D;

	switch (type) {
		case LUATEX_NONE: {
			if (!enable) {
				glDisable(GL_TEXTURE_2D); //FIXME diff GL_TEXTURE_2D?
			}
		} break;
		case LUATEX_GL: {
			texID = openglID;
		} break;
		case LUATEX_SHADOWMAP: {
			glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE); //FIXME
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
			if (enable) {
				glEnable(GL_TEXTURE_2D);
			}
		} break;
		case LUATEX_REFLECTION: {
			texID = cubeMapHandler->GetEnvReflectionTextureID();
			texType = GL_TEXTURE_CUBE_MAP_ARB;
		} break;
		case LUATEX_SPECULAR: {
			texID = cubeMapHandler->GetSpecularTextureID();
			texType = GL_TEXTURE_CUBE_MAP_ARB;
		} break;
		case LUATEX_EXTRA: {
			const CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
			texID = gd->infoTex;
		} break;
		case LUATEX_HEIGHTMAP: {
			texID = heightMapTexture->GetTextureID();
		} break;
		case LUATEX_SHADING: {
			texID = readmap->GetShadingTexture();
		} break;
		case LUATEX_GRASS: {
			texID = readmap->GetGrassShadingTexture();
		} break;
		case LUATEX_FONT: {
			texID = font->GetTexture();
		} break;
		case LUATEX_FONTSMALL: {
			texID = smallFont->GetTexture();
		} break;
		case LUATEX_MINIMAP: {
			texID = readmap->GetMiniMapTexture();
		} break;
		case LUATEX_INFOTEX: {
			texID = readmap->GetGroundDrawer()->infoTex;
		} break;
		default:
			assert(false);
	}

	if (texID != 0) {
		glBindTexture(texType, texID);
		if (enable) {
			glEnable(texType);
		}
	}
}


void LuaMatTexture::Unbind() const
{
	if (!enable && type != LUATEX_SHADOWMAP)
		return;

	switch (type) {
		case LUATEX_NONE: {
			//glDisable(GL_TEXTURE_2D);
		} break;
		case LUATEX_GL: {
			glDisable(GL_TEXTURE_2D);
		} break;
		case LUATEX_SHADOWMAP: {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
			if (enable) glDisable(GL_TEXTURE_2D);
		} break;
		case LUATEX_REFLECTION:
		case LUATEX_SPECULAR: {
			glDisable(GL_TEXTURE_CUBE_MAP_ARB);
		} break;
		case LUATEX_EXTRA:
		case LUATEX_HEIGHTMAP:
		case LUATEX_SHADING:
		case LUATEX_GRASS:
		case LUATEX_FONT:
		case LUATEX_FONTSMALL:
		case LUATEX_MINIMAP:
		case LUATEX_INFOTEX: {
			glDisable(GL_TEXTURE_2D);
		} break;
		default:
			assert(false);
	}
}



/******************************************************************************/
/******************************************************************************/
//
//  LuaMatTexture
//

void LuaMatTexture::Finalize()
{
	for (int t = 0; t < maxTexUnits; t++) {
		if ((type == LUATEX_GL) && (openglID == 0)) {
			type = LUATEX_NONE;
		}
		if (type == LUATEX_NONE) {
			enable = false;
		}
		if (type != LUATEX_GL) {
			openglID = 0;
		}
	}
}


int LuaMatTexture::Compare(const LuaMatTexture& a, const LuaMatTexture& b)
{
	if (a.type != b.type) {
		return (a.type < b.type) ? -1 : +1;
	}
	if (a.type == LUATEX_GL) {
		if (a.openglID != b.openglID) {
			return (a.openglID < b.openglID) ? -1 : +1;
		}
	}
	if (a.enable != b.enable) {
		return a.enable ? -1 : +1;
	}
	return 0; // LUATEX_NONE and LUATEX_SHADOWMAP ignore openglID
}


void LuaMatTexture::Print(const string& indent) const
{
	const char* typeName = "Unknown";
	switch (type) {
		#define STRING_CASE(ptr, x) case x: ptr = #x; break;
		STRING_CASE(typeName, LUATEX_NONE);
		STRING_CASE(typeName, LUATEX_GL);
		STRING_CASE(typeName, LUATEX_SHADOWMAP);
		STRING_CASE(typeName, LUATEX_REFLECTION);
		STRING_CASE(typeName, LUATEX_SPECULAR);
		STRING_CASE(typeName, LUATEX_EXTRA);
		STRING_CASE(typeName, LUATEX_HEIGHTMAP);
		STRING_CASE(typeName, LUATEX_SHADING);
		STRING_CASE(typeName, LUATEX_GRASS);
		STRING_CASE(typeName, LUATEX_FONT);
		STRING_CASE(typeName, LUATEX_FONTSMALL);
		STRING_CASE(typeName, LUATEX_MINIMAP);
		STRING_CASE(typeName, LUATEX_INFOTEX);
	}
	LOG("%s%s %i %s", indent.c_str(),
			typeName, openglID, (enable ? "true" : "false"));
}


/******************************************************************************/
/******************************************************************************/
