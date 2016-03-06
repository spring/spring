/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BaseGroundDrawer.h"

#include "Game/Camera.h"
#include "ReadMap.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "System/Config/ConfigHandler.h"


CONFIG(float, GroundLODScaleReflection).defaultValue(1.0f).headlessValue(0.0f);
CONFIG(float, GroundLODScaleRefraction).defaultValue(1.0f).headlessValue(0.0f);
CONFIG(float, GroundLODScaleTerrainReflection).defaultValue(1.0f);


CBaseGroundDrawer::CBaseGroundDrawer()
{
	LODScaleReflection = configHandler->GetFloat("GroundLODScaleReflection");
	LODScaleRefraction = configHandler->GetFloat("GroundLODScaleRefraction");
	LODScaleTerrainReflection = configHandler->GetFloat("GroundLODScaleTerrainReflection");

	drawForward = true;
	drawDeferred = false;
	drawMapEdges = false;

	wireframe = false;
	advShading = false;

	jamColor[0] = (int)(losColorScale * 0.1f);
	jamColor[1] = (int)(losColorScale * 0.0f);
	jamColor[2] = (int)(losColorScale * 0.0f);

	losColor[0] = (int)(losColorScale * 0.3f);
	losColor[1] = (int)(losColorScale * 0.3f);
	losColor[2] = (int)(losColorScale * 0.3f);

	radarColor[0] = (int)(losColorScale * 0.0f);
	radarColor[1] = (int)(losColorScale * 0.0f);
	radarColor[2] = (int)(losColorScale * 1.0f);

	alwaysColor[0] = (int)(losColorScale * 0.2f);
	alwaysColor[1] = (int)(losColorScale * 0.2f);
	alwaysColor[2] = (int)(losColorScale * 0.2f);

	radarColor2[0] = (int)(losColorScale * 0.0f);
	radarColor2[1] = (int)(losColorScale * 1.0f);
	radarColor2[2] = (int)(losColorScale * 0.0f);
	
	groundTextures = NULL;
}


CBaseGroundDrawer::~CBaseGroundDrawer()
{
}



void CBaseGroundDrawer::DrawTrees(bool drawReflection) const
{
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.005f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (treeDrawer->drawTrees) {
		// NOTE:
		//   the info-texture now contains an alpha-component
		//   so binding it here means trees will be invisible
		//   when shadows are disabled
		if (infoTextureHandler->IsEnabled()) {
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
			glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());
			SetTexGen(1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE), 0, 0);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}

		treeDrawer->Draw(drawReflection);

		if (infoTextureHandler->IsEnabled()) {
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
	}

	glPopAttrib();
}

