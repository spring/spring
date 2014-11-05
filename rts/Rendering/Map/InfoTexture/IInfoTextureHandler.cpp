/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IInfoTextureHandler.h"
#include "Legacy/LegacyInfoTextureHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/FBO.h"

IInfoTextureHandler* infoTextureHandler = nullptr;


void IInfoTextureHandler::Create()
{

	if (infoTextureHandler == nullptr) {
		infoTextureHandler = new CLegacyInfoTextureHandler();
	}
}
