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


/******************************************************************************/
/******************************************************************************/
