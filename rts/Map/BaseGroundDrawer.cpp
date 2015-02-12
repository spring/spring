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

	drawMapEdges = false;
	drawDeferred = false;
	wireframe = false;
	advShading = false;

	jamColor[0] = (int)(losColorScale * 0.30f);
	jamColor[1] = (int)(losColorScale * 0.00f);
	jamColor[2] = (int)(losColorScale * 0.00f);

	losColor[0] = (int)(losColorScale * 0.15f);
	losColor[1] = (int)(losColorScale * 0.15f);
	losColor[2] = (int)(losColorScale * 0.15f);

	radarColor[0] = (int)(losColorScale * 0.0f);
	radarColor[1] = (int)(losColorScale * 0.0f);
	radarColor[2] = (int)(losColorScale * 1.0f);

	alwaysColor[0] = (int)(losColorScale * 0.4f);
	alwaysColor[1] = (int)(losColorScale * 0.4f);
	alwaysColor[2] = (int)(losColorScale * 0.4f);

	groundTextures = NULL;
}


CBaseGroundDrawer::~CBaseGroundDrawer()
{
}



void CBaseGroundDrawer::DrawTrees(bool drawReflection) const
{
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.005f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	if (treeDrawer->drawTrees) {
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

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
}


void CBaseGroundDrawer::UpdateCamRestraints(CCamera* cam)
{
	// add restraints for camera sides
	cam->GetFrustumSides(readMap->GetCurrMinHeight() - 100.0f, readMap->GetCurrMaxHeight() + 30.0f,  SQUARE_SIZE);

	// CAMERA DISTANCE IS ALREADY CHECKED IN CGroundDrawer::GridVisibility()!
/*
	// add restraint for maximum view distance (use flat z-dir as side)
	// this is supposed to prevent (far) terrain from first being drawn
	// and then immediately z-clipped away
	const float3& camDir3D = cam->forward;

	// prevent colinearity in top-down view
	if (math::fabs(camDir3D.dot(UpVector)) < 0.95f) {
		float3 camDir2D  = float3(camDir3D.x, 0.0f, camDir3D.z).SafeANormalize();
		float3 camOffset = camDir2D * globalRendering->viewRange * 1.05f;

		// FIXME magic constants
		static const float miny = 0.0f;
		static const float maxy = 255.0f / 3.5f;
		cam->GetFrustumSide(camDir2D, camOffset, miny, maxy, SQUARE_SIZE, (camDir3D.y > 0.0f), false);
	}
*/
}
