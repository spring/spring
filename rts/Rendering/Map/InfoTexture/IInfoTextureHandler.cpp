/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IInfoTextureHandler.h"
#include "Legacy/LegacyInfoTextureHandler.h"
#include "Modern/InfoTextureHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/FBO.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"


IInfoTextureHandler* infoTextureHandler = nullptr;


void IInfoTextureHandler::Create()
{
	if (
		globalRendering->haveGLSL &&
		globalRendering->supportNonPowerOfTwoTex &&
		globalRendering->supportTextureQueryLOD &&
		FBO::IsSupported() &&
		glewIsSupported("GL_VERSION_3_0")
	) {
		try {
			infoTextureHandler = new CInfoTextureHandler();
		} catch (const opengl_error& glerr) {
			infoTextureHandler = nullptr;
		}
	}

	if (infoTextureHandler == nullptr)
		infoTextureHandler = new CLegacyInfoTextureHandler();

	if (dynamic_cast<CInfoTextureHandler*>(infoTextureHandler) != nullptr) {
		LOG("InfoTexture: shaders");
	} else {
		LOG("InfoTexture: legacy");
	}
}
