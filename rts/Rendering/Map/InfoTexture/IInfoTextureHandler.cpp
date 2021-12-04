/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IInfoTextureHandler.h"
#include "NullInfoTextureHandler.h"
#include "Modern/InfoTextureHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/FBO.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"


IInfoTextureHandler* infoTextureHandler = nullptr;


void IInfoTextureHandler::Create()
{
	try {
		infoTextureHandler = new CInfoTextureHandler();
	} catch (const opengl_error& glerr) {
		LOG("[IInfoTextureHandler::%s] exception \"%s\"", __func__, glerr.what());
		infoTextureHandler = new CNullInfoTextureHandler();
	}
}
