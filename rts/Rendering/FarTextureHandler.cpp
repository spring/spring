/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "Rendering/FarTextureHandler.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/UnitModels/3DModel.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/UnitModels/WorldObjectModelRenderer.h"
#include "Map/MapInfo.h"
#include "Game/Camera.h"
#include "System/GlobalUnsynced.h"
#include "System/myMath.h"
#include "System/LogOutput.h"

CFarTextureHandler* farTextureHandler = NULL;


CFarTextureHandler::CFarTextureHandler()
{
	usedFarTextures = 0;

	farTexture = 0;

	texSizeX = 1024;
	texSizeY = 1024;

	if (!fbo.IsValid()) {
		logOutput.Print("framebuffer not valid!");
		return;
	}

	glGenTextures(1,&farTexture);
	glBindTexture(GL_TEXTURE_2D, farTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texSizeX, texSizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	fbo.Bind();
	fbo.AttachTexture(farTexture);
	fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, GL_DEPTH_COMPONENT16, texSizeX, texSizeY); //TODO hmmm perhaps just create it on-demand to save gfx-ram?
	bool status = fbo.CheckStatus("FARTEXTURE");
	if (status) {
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	fbo.Unbind();

	fbo.reloadOnAltTab = true;

	if (!status) {
		//do something
	}
}


CFarTextureHandler::~CFarTextureHandler()
{
	glDeleteTextures(1, &farTexture);
}


/**
 * @brief Add the model to the queue of units waiting for their farTexture.
 * On the next CreateFarTextures() call the farTexture for this model will be
 * created.
 */
void CFarTextureHandler::CreateFarTexture(S3DModel* model)
{
	GML_STDMUTEX_LOCK(tex); // CreateFarTexture

	pending.push_back(model);
}


/**
 * @brief Process the queue of pending farTexture creation requests.
 * This loops through the queue calling ReallyCreateFarTexture() on each entry,
 * and empties the queue afterwards.
 */
void CFarTextureHandler::CreateFarTextures()
{
	GML_STDMUTEX_LOCK(tex); // CreateFarTextures

	for(std::vector<S3DModel*>::const_iterator it = pending.begin(); it != pending.end(); ++it) {
		ReallyCreateFarTexture(*it);
	}
	pending.clear();
}


/**
 * @brief Returns the (row, column) pair of a FarTexture in the TextureAtlas.
 */
int2 CFarTextureHandler::GetTextureCoordsInt(const int& farTextureNum, const int& orientation)
{
	const int texnum = (farTextureNum * numOrientations) + orientation;

	const int row = texnum / (texSizeX / iconSizeX);
	const int col = texnum - row * (texSizeX / iconSizeX);
	return int2(col, row);
}


/**
 * @brief Returns the TexCoords of a FarTexture in the TextureAtlas.
 */
float2 CFarTextureHandler::GetTextureCoords(const int& farTextureNum, const int& orientation)
{
	float2 texcoords;

	const int texnum = (farTextureNum * numOrientations) + orientation;

	const int row = texnum / (texSizeX / iconSizeX);
	const int col = texnum - row * (texSizeX / iconSizeX);

	texcoords.x = (float(iconSizeX) / texSizeX) * col;
	texcoords.y = (float(iconSizeY) / texSizeY) * row;

	return texcoords;
}


/**
 * @brief Really create the far texture for the given model.
 */
void CFarTextureHandler::ReallyCreateFarTexture(S3DModel* model)
{
	model->farTextureNum = usedFarTextures;

	const int maxSprites = (texSizeX / iconSizeX)*(texSizeY / iconSizeY) - 1;
	if (usedFarTextures >= maxSprites) {
		//TODO resize texture atlas if possible
		//logOutput.Print("Out of farTextures");
		return;
	}

	if (!fbo.IsValid()) {
		//logOutput.Print("framebuffer not valid!");
		return;
	}

	fbo.Bind();

	glDisable(GL_BLEND);
	unitDrawer->SetupForUnitDrawing();
	unitDrawer->GetOpaqueModelRenderer(model->type)->PushRenderState();

	if (model->type == MODELTYPE_S3O) {
		//FIXME for some strange reason we need to invert the culling, why?
		glCullFace(GL_FRONT);
		texturehandlerS3O->SetS3oTexture(model->textureType);
	}

	unitDrawer->SetTeamColour(0);

	glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-model->radius, model->radius, -model->radius, model->radius, -model->radius*1.5f, model->radius*1.5f);
	glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glScalef(-1.0f, 1.0f, 1.0f);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glRotatef(45.0f, 1.0f, 0.0f, 0.0f);

	//! draw the model in 8 different orientations
	for (size_t orient = 0; orient < numOrientations; ++orient) {
		//! setup viewport
		int2 pos = GetTextureCoordsInt(usedFarTextures, orient);
		glViewport(pos.x * iconSizeX, pos.y * iconSizeY, iconSizeX, iconSizeY);

		glPushMatrix();
		glTranslatef(0, -model->height * 0.5f, 0);

		//! draw the model to a temporary buffer
		model->DrawStatic();

		glPopMatrix();

		//! rotate by 45 degrees for the next orientation
		glRotatef(-360.0f / numOrientations, 0, 1, 0);
		glLightfv(GL_LIGHT1, GL_POSITION, mapInfo->light.sunDir);
	}

	unitDrawer->GetOpaqueModelRenderer(model->type)->PopRenderState();
	unitDrawer->CleanUpUnitDrawing();

	glViewport(gu->viewPosX, 0, gu->viewSizeX, gu->viewSizeY);

	usedFarTextures++;

	fbo.Unbind();
}



void CFarTextureHandler::DrawFarTexture(const CCamera* cam, const S3DModel* mdl, const float3& pos, float radius, short heading, CVertexArray* va) {
	const float3 interPos = pos + UpVector * mdl->height * 0.5f;

	//! indicates the orientation to draw
	static const int USHRT_MAX_ = (1 << 16);
	const int orient_step = USHRT_MAX_ / numOrientations;

	int orient = GetHeadingFromVector(-cam->forward.x, -cam->forward.z) - heading;
		orient += USHRT_MAX_;          //! make it positive only
		orient += (orient_step >> 1);  //! we want that frontdir is from -orient_step/2 upto orient_step/2
		orient %= USHRT_MAX_;          //! we have an angle so it's periodical
		orient /= orient_step;         //! get the final direction index

	const float iconSizeX = float(this->iconSizeX) / texSizeX;
	const float iconSizeY = float(this->iconSizeY) / texSizeY;
	const float2 texcoords = GetTextureCoords(mdl->farTextureNum, orient);

	const float3 curad = camera->up *    radius;
	const float3 crrad = camera->right * radius;

	va->AddVertexQT(interPos - curad + crrad, texcoords.x, texcoords.y );
	va->AddVertexQT(interPos + curad + crrad, texcoords.x, texcoords.y + iconSizeY);
	va->AddVertexQT(interPos + curad - crrad, texcoords.x + iconSizeX, texcoords.y + iconSizeY);
	va->AddVertexQT(interPos - curad - crrad, texcoords.x + iconSizeX, texcoords.y );
}
