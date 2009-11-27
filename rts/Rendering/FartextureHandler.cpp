#include "StdAfx.h"
#include "mmgr.h"

#include "LogOutput.h"
#include "FartextureHandler.h"
#include "GlobalUnsynced.h"
#include "UnitModels/UnitDrawer.h"
#include "Textures/Bitmap.h"
#include "Rendering/UnitModels/3DModel.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Map/MapInfo.h"


CFartextureHandler* fartextureHandler = NULL;


CFartextureHandler::CFartextureHandler()
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


CFartextureHandler::~CFartextureHandler()
{
	glDeleteTextures(1, &farTexture);
}


/**
 * @brief Add the model to the queue of units waiting for their fartexture.
 * On the next CreateFarTextures() call the fartexture for this model will be
 * created.
 */
void CFartextureHandler::CreateFarTexture(S3DModel* model)
{
	GML_STDMUTEX_LOCK(tex); // CreateFarTexture

	pending.push_back(model);
}


/**
 * @brief Process the queue of pending fartexture creation requests.
 * This loops through the queue calling ReallyCreateFarTexture() on each entry,
 * and empties the queue afterwards.
 */
void CFartextureHandler::CreateFarTextures()
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
int2 CFartextureHandler::GetTextureCoordsInt(const int& farTextureNum, const int& orientation)
{
	const int texnum = (farTextureNum * numOrientations) + orientation;

	const int row = texnum / (texSizeX / iconSizeX);
	const int col = texnum - row * (texSizeX / iconSizeX);
	return int2(col,row);
}


/**
 * @brief Returns the TexCoords of a FarTexture in the TextureAtlas.
 */
float2 CFartextureHandler::GetTextureCoords(const int& farTextureNum, const int& orientation)
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
void CFartextureHandler::ReallyCreateFarTexture(S3DModel* model)
{
	model->farTextureNum = usedFarTextures;

	const int maxSprites = (texSizeX / iconSizeX)*(texSizeY / iconSizeY) - 1;
	if (usedFarTextures >= maxSprites) {
		//TODO resize texture atlas if possible
		//logOutput.Print("Out of fartextures");
		return;
	}

	if (!fbo.IsValid()) {
		//logOutput.Print("framebuffer not valid!");
		return;
	}

	fbo.Bind();

	glDisable(GL_BLEND);
	unitDrawer->SetupForUnitDrawing();
	if (model->type == MODELTYPE_3DO) {
		unitDrawer->SetupFor3DO();
	} else {
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
		glScalef(-1,1,1);

	glColor4f(1,1,1,1);
	glRotatef(45, 1, 0, 0);

	//! draw the model in 8 different orientations
	for (size_t orient=0; orient < numOrientations; ++orient) {
		//! setup viewport
		int2 pos = GetTextureCoordsInt(usedFarTextures, orient);
		glViewport(pos.x * iconSizeX, pos.y * iconSizeY, iconSizeX, iconSizeY);

		glPushMatrix();
		glTranslatef(0, -model->height*0.5f, 0);

		//! draw the model to a temporary buffer
		model->DrawStatic();

		glPopMatrix();

		//! rotate by 45 degrees for the next orientation
		glRotatef(360.0f/numOrientations, 0, 1, 0);
		glLightfv(GL_LIGHT1, GL_POSITION, mapInfo->light.sunDir);
	}

	if (model->type == MODELTYPE_3DO) {
		unitDrawer->CleanUp3DO();
	}
	unitDrawer->CleanUpUnitDrawing();

	glViewport(gu->viewPosX, 0, gu->viewSizeX, gu->viewSizeY);

	usedFarTextures++;

	fbo.Unbind();
}
